/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/cpp.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <endian.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fat_fs.h"
#include "fat_priv.h"

#define LOCAL_TRACE 0

// TODO: clean up this routine to not need to allocate and build the LFN in place

__MALLOC __WARN_UNUSED_RESULT static char *fat_dir_get_filename(uint8_t *dir, off_t offset, int lfn_sequences) {
    int result_len = 1 + (lfn_sequences == 0 ? 12 : (lfn_sequences * 26));
    int j = 0;

    char *result = (char *)calloc(1, result_len);

    if (lfn_sequences == 0) {
        // Ignore trailing spaces in filename and/or extension
        int fn_len=8, ext_len=3;
        for (int i=7; i>=0; i--) {
            if (dir[offset + i] == 0x20) {
                fn_len--;
            } else {
                break;
            }
        }
        for (int i=10; i>=8; i--) {
            if (dir[offset + i] == 0x20) {
                ext_len--;
            } else {
                break;
            }
        }

        for (int i=0; i<fn_len; i++) {
            result[j++] = dir[offset + i];
        }
        if (ext_len > 0) {
            result[j++] = '.';
            for (int i=0; i<ext_len; i++) {
                result[j++] = dir[offset + 8 + i];
            }
        }
    } else {
        // XXX: not unicode aware.
        for (int sequence=1; sequence<=lfn_sequences; sequence++) {
            for (int i=1; i<DIR_ENTRY_LENGTH; i++) {
                int char_offset = (offset - (sequence * DIR_ENTRY_LENGTH)) + i;
                if (dir[char_offset] != 0x00 && dir[char_offset] != 0xff) {
                    result[j++] = dir[char_offset];
                }

                if (i == 9) {
                    i = 13;
                } else if (i == 25) {
                    i = 27;
                }
            }
        }
    }
    return result;
}

status_t fat_walk(fat_fs_t *fat, const char *path, dir_entry *out_entry) {
    LTRACEF("path %s\n", path);

    DEBUG_ASSERT(fat->lock.is_held());

    // routine to push the path element ahead one bump
    // will leave path pointing at the next element, and path_element_size
    // in characters for the next element (or 0 if finished).
    size_t path_element_size = 0;
    auto path_increment = [&path, &path_element_size]() {
        path += path_element_size;
        path_element_size = 0;

        // we're at the end of the string
        if (*path == 0) {
            return;
        }

        // push path past the next /
        while (*path == '/' && path != 0) {
            path++;
        }

        // count the size of the next element
        const char *ptr = path;
        while (*ptr != '/' && *ptr != 0) {
            ptr++;
            path_element_size++;
        }
    };

    // increment it once past leading / and establish an initial path_element_size
    path_increment();
    LTRACEF("first path '%s' path_element_size %zu\n", path, path_element_size);

    // set up our dir structure
    dir_info dir;
    if (fat->root_cluster) {
        dir.is_linear_root_dir = false;
        dir.starting_cluster = fat->root_cluster;
    } else {
        dir.is_linear_root_dir = true; // inline root dir that fat 12/16s have
    }

    // output entry
    dir_entry entry {};

    // walk the directory structure
    for (;;) {
        char name_element[256];
        strlcpy(name_element, path, MIN(sizeof(name_element), path_element_size + 1));

        LTRACEF("searching for element %s\n", name_element);

        auto status = fat_find_file_in_dir(fat, dir, name_element, &entry);
        if (status < 0) {
            return ERR_NOT_FOUND;
        }

        // we found something
        LTRACEF("found dir entry attributes %#hhx length %u start_cluster %u\n",
                (uint8_t)entry.attributes, entry.length, entry.start_cluster);

        // push the name element forward one notch so we can see if we're at the end (or iterate once again)
        path_increment();
        if (path_element_size > 0) {
            // did we find a directory on an inner node of the path? we can continue iterating
            if (entry.attributes == fat_attribute::directory) {
                dir.is_linear_root_dir = false;
                dir.starting_cluster = entry.start_cluster;
            } else {
                LTRACEF("found a non dir at a non terminal path entry, exit\n");
                return ERR_NOT_FOUND;
            }
        } else {
            // we got a hit at the terminal entry of the path, pass it out to the caller as a success
            *out_entry = entry;
            return NO_ERROR;
        }
    }
}

// TODO: add FAT 12/16 support for non cluster based root dir
status_t fat_find_file_in_dir(fat_fs_t *fat, const dir_info &dir, const char *name, dir_entry *entry) {
    LTRACEF("dir (is_linear_root_dir %d, start_cluster %u), name '%s', out entry %p\n", dir.is_linear_root_dir, dir.starting_cluster, name, entry);

    DEBUG_ASSERT(fat->lock.is_held());

    const size_t namelen = strlen(name);

    // TODO: remove this allocation and use the bcache directly
    uint8_t *buffer = (uint8_t *)malloc(fat->bytes_per_cluster);
    auto free_buffer = lk::make_auto_call([buffer]() { free(buffer); });

    // bootstrap our sector/cluster walker based on if we're the root dir or not
    uint32_t dir_sector;
    uint32_t dir_cluster;
    dir_sector = dir_cluster = 0;
    if (dir.is_linear_root_dir) {
        dir_sector = fat->root_start_sector;
    } else {
        dir_cluster = dir.starting_cluster;
    }

    for (;;) {
        if (dir_sector) {
            PANIC_UNIMPLEMENTED;
        } else {
            // XXX: use the cache!
            auto ret = fat_read_cluster(fat, buffer, dir_cluster);
            if (ret < 0) {
                return ret;
            } else if (ret != (ssize_t)fat->bytes_per_cluster) {
                return ERR_GENERIC;
            }
        }

        if (LOCAL_TRACE) {
            LTRACEF("dir cluster:\n");
            hexdump8(buffer, 512);
        }

        // walk within a cluster
        uint32_t offset = 0;
        uint32_t lfn_sequences = 0;
        bool matched;
        while (buffer[offset] != 0x00 && offset < fat->bytes_per_cluster) {
            LTRACEF_LEVEL(2, "looking at offset %u\n", offset);
            if (buffer[offset] == 0xE5 /*deleted*/) {
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else if ((buffer[offset + 0x0B] & 0x08)) {
                if (buffer[offset + 0x0B] == 0x0f) {
                    lfn_sequences++;
                }

                offset += DIR_ENTRY_LENGTH;
                continue;
            }

            char *filename = fat_dir_get_filename(buffer, offset, lfn_sequences);
            lfn_sequences = 0;

            LTRACEF("found filename '%s'\n", filename);
            const size_t filenamelen = strlen(filename);

            matched = false;
            if (filenamelen == namelen && !strnicmp(name, filename, filenamelen)) {
                matched = true;
            }

            // we found an entry, fill it in and exit
            if (matched) {
                uint16_t target_cluster = fat_read16(buffer, offset + 0x1a);
                entry->length = fat_read32(buffer, offset + 0x1c);
                entry->attributes = (fat_attribute)buffer[0x0B + offset];
                entry->start_cluster = target_cluster;
                free(filename);
                return NO_ERROR;
            }

            free(filename);

            offset += DIR_ENTRY_LENGTH;
        }

        if (dir_cluster) {
            dir_cluster = fat_next_cluster_in_chain(fat, dir_cluster);
            if (is_eof_cluster(dir_cluster)) {
                // no more clusters in the chain
                break;
            }
        } else {
            // sector based
            PANIC_UNIMPLEMENTED;
        }
    }

    return ERR_NOT_FOUND;
}

struct fat_dir_cookie {
    struct list_node node;
    struct fat_dir *dir;

    // next directory index offset (in units of 0x20), 0xffffffff for EOD
    uint32_t index;
    static const uint32_t index_eod = 0xffffffff;
};

struct fat_dir {
    struct list_node node;
    struct list_node cookies = LIST_INITIAL_VALUE(cookies);

    fat_fs_t *fat;

    // id of the directory is keyed off the starting cluster (or 0 for the root dir)
    uint32_t start_cluster;
};

status_t fat_opendir(fscookie *cookie, const char *name, dircookie **dcookie) {
    auto fat = (fat_fs_t *)cookie;

    LTRACEF("cookie %p name '%s' dircookie %p\n", cookie, name, dcookie);

    AutoLock guard(fat->lock);

    dir_entry entry;

    // special case for /
    if (name[0] == 0 || !strcmp(name, "/")) {
        entry.attributes = fat_attribute::directory;
        entry.length = 0;
        entry.start_cluster = 0;
    } else {
        status_t err = fat_walk(fat, name, &entry);
        if (err != NO_ERROR) {
            return err;
        }
    }

    // if we walked and found a proper directory, it's a hit
    if (entry.attributes == fat_attribute::directory) {
        // TODO: see if the dir is already in the list
        auto dir = new fat_dir;
        dir->fat = fat;
        dir->start_cluster = entry.start_cluster;
        list_add_head(&fat->dir_list, &dir->node);

        // create a dir cookie
        auto dir_cookie = new fat_dir_cookie;
        dir_cookie->dir = dir;
        dir_cookie->index = 0;

        // add it to the dir object
        list_add_tail(&dir->cookies, &dir_cookie->node);

        *dcookie = (dircookie *)dir_cookie;
        return NO_ERROR;
    } else {
        return ERR_NOT_FILE;
    }

    return ERR_NOT_IMPLEMENTED;
};

status_t fat_readdir(dircookie *dcookie, struct dirent *ent) {
    auto cookie = (fat_dir_cookie *)dcookie;
    auto fat = cookie->dir->fat;

    LTRACEF("dircookie %p ent %p, current index %u\n", dcookie, ent, cookie->index);

    if (!ent)
        return ERR_INVALID_ARGS;

    AutoLock guard(fat->lock);

    // TODO: actually walk the dir
    cookie->index = fat_dir_cookie::index_eod;
    return ERR_NOT_FOUND;
}


status_t fat_closedir(dircookie *dcookie) {
    auto cookie = (fat_dir_cookie *)dcookie;
    auto fat = cookie->dir->fat;

    LTRACEF("dircookie %p\n", dcookie);

    AutoLock guard(fat->lock);

    // free the dircookie
    //mutex_acquire(&dcookie->fs->lock);
    list_delete(&cookie->node);
    //mutex_release(&dcookie->fs->lock);

    free(dcookie);

    return NO_ERROR;
}

