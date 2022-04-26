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
#include "file_iterator.h"

#define LOCAL_TRACE 0

// structure that represents an open dir, may have multiple cookies in its list
// at any point in time,
struct fat_dir {
    struct list_node node;
    struct list_node cookies = LIST_INITIAL_VALUE(cookies);

    fat_fs_t *fat;

    // id of the directory is keyed off the starting cluster (or 0 for the root dir)
    uint32_t start_cluster;
};

// structure that represents an open dir handle. holds the offset into the directory
// that is being walked.
struct fat_dir_cookie {
    struct list_node node;
    struct fat_dir *dir;

    // next directory index offset in bytes, 0xffffffff for EOD
    uint32_t index;
    static const uint32_t index_eod = 0xffffffff;
};

namespace {

// walk one entry into the dir, starting at byte offset into the directory block iterator.
// both dbi and offset will be modified during the call.
// filles out the entry and returns a pointer into the passed in buffer in out_filename.
// NOTE: *must* pass at least a MAX_FILE_NAME_LEN byte char pointer in the filename_buffer slot.
status_t fat_find_next_entry(fat_fs_t *fat, file_block_iterator &dbi, uint32_t &offset, dir_entry *entry,
        char filename_buffer[MAX_FILE_NAME_LEN], char **out_filename) {

    DEBUG_ASSERT(entry && filename_buffer && out_filename);
    DEBUG_ASSERT(offset <= fat->bytes_per_sector); // passing offset == bytes_per_sector is okay

    // lfn parsing state
    struct lfn_parse_state {
        size_t  pos = 0;
        uint8_t last_sequence = 0xff; // 0xff means we haven't seen anything
                                      // since last reset
        uint8_t checksum = 0;

        void reset() {
            last_sequence = 0xff;
        }
    } lfn_state;

    for (;;) {
        if (LOCAL_TRACE >= 2) {
            LTRACEF("dir sector:\n");
            hexdump8_ex(dbi.get_bcache_ptr(0), fat->bytes_per_sector, 0);
        }

        // walk within a sector
        while (offset < fat->bytes_per_sector) {
            LTRACEF_LEVEL(2, "looking at offset %u\n", offset);
            const uint8_t *ent = dbi.get_bcache_ptr(offset);
            if (ent[0] == 0) { // no more entries
                // we're completely done
                LTRACEF("completely done\n");
                return ERR_NOT_FOUND;
            } else if (ent[0] == 0xE5) { // deleted entry
                LTRACEF("deleted entry\n");
                lfn_state.reset();
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else if (ent[0x0B] == (uint8_t)fat_attribute::volume_id) {
                // skip volume ids
                LTRACEF("skipping volume id\n");
                lfn_state.reset();
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else if (ent[0x0B] == (uint8_t)fat_attribute::lfn) {
                // part of a LFN sequence
                uint8_t sequence = ent[0] & ~0x40;
                if (ent[0] & 0x40) {
                    // end sequence, start a new backwards fill of the lfn name
                    lfn_state.pos = MAX_FILE_NAME_LEN;
                    filename_buffer[--lfn_state.pos] = 0;
                    lfn_state.last_sequence = sequence;
                    lfn_state.checksum = ent[0x0d];
                    LTRACEF_LEVEL(2, "start of new LFN entry, sequence %u\n", sequence);
                } else {
                    if (lfn_state.last_sequence != sequence + 1) {
                        // our entry is out of sequence? drop it and start over
                        LTRACEF("ent out of sequence %u (last sequence %u)\n", sequence, lfn_state.last_sequence);
                        lfn_state.reset();
                        offset += DIR_ENTRY_LENGTH;
                        continue;
                    }
                    if (lfn_state.checksum != ent[0x0d]) {
                        // all of the long sequences need to match the checksum
                        LTRACEF("ent mismatches previous checksum\n");
                        lfn_state.reset();
                        offset += DIR_ENTRY_LENGTH;
                        continue;
                    }
                    lfn_state.last_sequence = sequence;
                }

                // walk backwards through the entry, picking out unicode characters
                // table of unicode character offsets:
                const size_t table[] = { 30, 28, 24, 22, 20, 18, 16, 14, 9, 7, 5, 3, 1 };
                for (auto off : table) {
                    uint16_t c = fat_read16(ent, off);
                    if (c != 0xffff && c != 0x0) {
                        // TODO: properly deal with unicode -> utf8
                        filename_buffer[--lfn_state.pos] = c & 0xff;
                    }
                }

                LTRACEF_LEVEL(2, "lfn filename thus far: '%s' sequence %hhu\n", filename_buffer + lfn_state.pos, sequence);

                // iterate one more entry, since we need to at least need to find the corresponding SFN
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else {
                // regular entry, extract the short file name
                char short_filename[8 + 1 + 3 + 1]; // max short name (8 . 3 NULL)
                size_t fname_pos = 0;

                // Ignore trailing spaces in filename and/or extension
                int fn_len = 8, ext_len = 3;
                for (int i = 7; i >= 0; i--) {
                    if (ent[i] == 0x20) {
                        fn_len--;
                    } else {
                        break;
                    }
                }
                for (size_t i = 10; i >= 8; i--) {
                    if (ent[i] == 0x20) {
                        ext_len--;
                    } else {
                        break;
                    }
                }

                for (int i = 0; i < fn_len; i++) {
                    short_filename[fname_pos++] = ent[i];
                }
                if (ext_len > 0) {
                    short_filename[fname_pos++] = '.';
                    for (int i=0; i < ext_len; i++) {
                        short_filename[fname_pos++] = ent[8 + i];
                    }
                }
                short_filename[fname_pos++] = 0;
                DEBUG_ASSERT(fname_pos <= sizeof(short_filename));

                // now we have the SFN, see if we just got finished parsing a corresponding LFN
                // in the previous entries
                if (lfn_state.last_sequence == 1) {
                    // TODO: compute checksum and make sure it matches
                    *out_filename = filename_buffer + lfn_state.pos;
                } else {
                    // copy the parsed short file name into the out buffer
                    strlcpy(filename_buffer, short_filename, sizeof(short_filename));
                    *out_filename = filename_buffer;
                }

                lfn_state.reset();
                offset += DIR_ENTRY_LENGTH;

                // fall through, we've found a file entry
            }

            LTRACEF("found filename '%s'\n", *out_filename);

            // fill out the passed in dir entry and exit
            uint16_t target_cluster = fat_read16(ent, 0x1a);
            entry->length = fat_read32(ent, 0x1c);
            entry->attributes = (fat_attribute)ent[0x0B];
            entry->start_cluster = target_cluster;
            return NO_ERROR;
        }

        DEBUG_ASSERT(offset <= fat->bytes_per_sector);

        // move to the next sector
        status_t err = dbi.next_sector();
        if (err < 0) {
            break;
        }
        // starting over at offset 0 in the new sector
        offset = 0;
    }

    // we're out of entries
    return ERR_NOT_FOUND;
}

status_t fat_find_file_in_dir(fat_fs_t *fat, uint32_t starting_cluster, const char *name, dir_entry *entry) {
    LTRACEF("start_cluster %u, name '%s', out entry %p\n", starting_cluster, name, entry);

    DEBUG_ASSERT(fat->lock.is_held());

    // cache the length of the string we're matching against
    const size_t namelen = strlen(name);

    // kick start our directory sector iterator
    file_block_iterator dbi(fat, starting_cluster);
    status_t err = dbi.next_sectors(0);
    if (err < 0) {
        return err;
    }

    uint32_t offset = 0;
    for (;;) {
        char filename_buffer[MAX_FILE_NAME_LEN]; // max fat file name length
        char *filename;

        // step forward one entry and see if we got something
        err = fat_find_next_entry(fat, dbi, offset, entry, filename_buffer, &filename);
        if (err < 0) {
            return err;
        }

        const size_t filenamelen = strlen(filename);

        // see if we've matched an entry
        if (filenamelen == namelen && !strnicmp(name, filename, filenamelen)) {
            // we have, return with a good status
            return NO_ERROR;
        }
    }
}

} // namespace

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

    // set up the starting cluster to search
    uint32_t dir_start_cluster;
    if (fat->root_cluster) {
        dir_start_cluster = fat->root_cluster;
    } else {
        // fat 12/16 has a linear root dir, cluster 0 is a special case to fat_find_file_in_dir below
        dir_start_cluster = 0;
    }

    // output entry
    dir_entry entry {};

    // walk the directory structure
    for (;;) {
        char name_element[MAX_FILE_NAME_LEN];
        strlcpy(name_element, path, MIN(sizeof(name_element), path_element_size + 1));

        LTRACEF("searching for element %s\n", name_element);

        auto status = fat_find_file_in_dir(fat, dir_start_cluster, name_element, &entry);
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
                dir_start_cluster = entry.start_cluster;
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

status_t fat_opendir(fscookie *cookie, const char *name, dircookie **dcookie) {
    auto fat = (fat_fs_t *)cookie;

    LTRACEF("cookie %p name '%s' dircookie %p\n", cookie, name, dcookie);

    AutoLock guard(fat->lock);

    dir_entry entry;

    // special case for /
    if (name[0] == 0 || !strcmp(name, "/")) {
        entry.attributes = fat_attribute::directory;
        entry.length = 0;
        if (fat->fat_bits == 32) {
            entry.start_cluster = fat->root_cluster;
        } else {
            entry.start_cluster = 0;
        }
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

    // make sure the cookie makes sense
    DEBUG_ASSERT((cookie->index % DIR_ENTRY_LENGTH) == 0);

    char filename_buffer[MAX_FILE_NAME_LEN];
    char *filename;
    dir_entry entry;

    {
        AutoLock guard(fat->lock);

        // kick start our directory sector iterator
        LTRACEF("start cluster %u\n", cookie->dir->start_cluster);
        file_block_iterator dbi(fat, cookie->dir->start_cluster);

        // move it forward to our index point
        // also loads the buffer
        status_t err = dbi.next_sectors(cookie->index / fat->bytes_per_sector);
        if (err < 0) {
            return err;
        }

        // reset how many sectors the dbi has pushed forward so we can account properly for index shifts later
        dbi.reset_sector_inc_count();

        // pass the index in units of sector offset
        uint32_t offset = cookie->index % fat->bytes_per_sector;
        err = fat_find_next_entry(fat, dbi, offset, &entry, filename_buffer, &filename);
        if (err < 0) {
            return err;
        }

        // bump the index forward by extracting how much the sector iterator pushed things forward
        uint32_t index_inc = offset - (cookie->index % fat->bytes_per_sector);
        index_inc += dbi.get_sector_inc_count() * fat->bytes_per_sector;
        LTRACEF("calculated index increment %u (old index %u, offset %u, sector_inc_count %u)\n",
                index_inc, cookie->index, offset, dbi.get_sector_inc_count());
        cookie->index += index_inc;
    }

    // copy the info into the fs layer's entry
    strlcpy(ent->name, filename, MIN(sizeof(ent->name), MAX_FILE_NAME_LEN));

    return NO_ERROR;
}


status_t fat_closedir(dircookie *dcookie) {
    auto cookie = (fat_dir_cookie *)dcookie;
    auto dir = cookie->dir;
    auto fat = dir->fat;

    LTRACEF("dircookie %p\n", dcookie);

    AutoLock guard(fat->lock);

    // free the dircookie
    list_delete(&cookie->node);

    // delete the dir node from the fat list if this was the last cookie
    // TODO: use actual refs
    if (list_is_empty(&dir->cookies)) {
        list_delete(&dir->node);
        delete dir;
    }

    delete cookie;

    return NO_ERROR;
}

