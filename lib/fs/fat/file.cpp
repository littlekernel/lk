/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <lib/bio.h>
#include <lib/fs.h>
#include <lk/trace.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <lk/debug.h>

#include "fat_fs.h"
#include "fat_priv.h"

#define DIR_ENTRY_LENGTH 32

#define LOCAL_TRACE 1

__MALLOC __WARN_UNUSED_RESULT static char *fat_dir_get_filename(uint8_t *dir, off_t offset, int lfn_sequences) {
    int result_len = 1 + (lfn_sequences == 0 ? 12 : (lfn_sequences * 26));
    int j = 0;

    char *result = (char *)malloc(result_len);
    memset(result, 0x00, result_len);

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

status_t fat_open_file(fscookie *cookie, const char *path, filecookie **fcookie) {
    fat_fs_t *fat = (fat_fs_t *)cookie;
    status_t result = ERR_GENERIC;

    LTRACEF("fscookie %p path %s fcookie %p\n", cookie, path, fcookie);

    uint8_t *dir = (uint8_t *)malloc(fat->bytes_per_cluster);
    uint32_t dir_cluster = fat->root_cluster; // TODO: handle fat16/12 root dirs
    fat_file_t *file = NULL;

    const char *ptr;
    /* chew up leading slashes */
    ptr = &path[0];
    while (*ptr == '/') {
        ptr++;
    }

    bool done = false;
    do {
        // XXX: use the cache!
        auto ret =fat_read_cluster(fat, dir, dir_cluster);
        if (ret != fat->bytes_per_cluster) {
            if (ret < 0) {
                result = ret;
            } else {
                result = ERR_GENERIC;
            }
            goto out;
        }

        if (LOCAL_TRACE) {
            LTRACEF("dir cluster:\n");
            hexdump8(dir, 512);
        }

        char *next_sep = strchr(ptr, '/');
        if (next_sep) {
            /* terminate the next component, giving us a substring */
            *next_sep = 0;
        } else {
            /* this is the last component */
            done = true;
        }

        uint32_t offset = 0;
        uint32_t lfn_sequences = 0;
        bool matched = false;
        while (dir[offset] != 0x00 && offset < fat->bytes_per_cluster) {
            if ( dir[offset] == 0xE5 /*deleted*/) {
                offset += DIR_ENTRY_LENGTH;
                continue;
            } else if ((dir[offset + 0x0B] & 0x08)) {
                if (dir[offset + 0x0B] == 0x0f) {
                    lfn_sequences++;
                }

                offset += DIR_ENTRY_LENGTH;
                continue;
            }

            char *filename = fat_dir_get_filename(dir, offset, lfn_sequences);
            lfn_sequences = 0;

            TRACEF("found filename '%s'\n", filename);

            matched = (strnicmp(ptr, filename, strlen(filename)) == 0);
            free(filename);

            if (matched) {
                uint16_t target_cluster = fat_read16(dir, offset + 0x1a);
                if (done == true) {
                    file = (fat_file_t *)malloc(sizeof(fat_file_t));
                    file->fat_fs = fat;
                    file->start_cluster = target_cluster;
                    file->length = fat_read32(dir, offset + 0x1c);
                    file->attributes = (fat_attribute)dir[0x0B + offset];
                    result = NO_ERROR;
                } else {
                    dir_cluster = target_cluster;
                }
                break;
            }

            offset += DIR_ENTRY_LENGTH;
        }

        if (matched == true) {
            if (done == true) {
                break;
            } else {
                /* move to the next separator */
                ptr = next_sep + 1;

                /* consume multiple separators */
                while (*ptr == '/') {
                    ptr++;
                }
            }
        } else {
            // XXX: untested!!!
            dir_cluster = fat_next_cluster_in_chain(fat, dir_cluster);
            if (is_eof_cluster(dir_cluster)) {
                // no more clusters in the chain
                break;
            }
        }
    } while (true);

out:
    *fcookie = (filecookie *)file;
    free(dir);
    return result;
}

ssize_t fat_read_file(filecookie *fcookie, void *_buf, off_t offset, size_t len) {
    fat_file_t *file = (fat_file_t *)fcookie;
    fat_fs_t *fat = file->fat_fs;
    bdev_t *dev = fat->dev;

    LTRACEF("fcookie %p buf %p offset %lld len %zu\n", fcookie, _buf, offset, len);

    // negative offsets are invalid
    if (offset < 0) {
        return ERR_INVALID_ARGS;
    }

    // trim the read to the file
    if (offset >= file->length) {
        return 0;
    }
    if (offset + len > file->length) {
        len = file->length - offset;
    }

    LTRACEF("trimmed offset %lld len %zu\n", offset, len);

    uint8_t *buf = (uint8_t *)_buf;
    ssize_t amount_read = 0;

    uint32_t cluster = file_offset_to_cluster(fat, file->start_cluster, offset);

    LTRACEF("starting cluster at %#x\n", cluster);

    while (len > 0) {
        size_t to_read = MIN(fat->bytes_per_cluster, len);
        size_t cluster_offset = offset % fat->bytes_per_cluster;

        LTRACEF("top of loop: len remaining %zu offset %llu to_read %zu cluster offset %zu\n", len, offset, to_read, cluster_offset);

#if 0
        // TODO: finish logic to handle reading sectors out of the block cache
        uint8_t *cache_ptr;
        bcache_get_block(fat->cache, (void **)&cache_ptr, fat_offset_for_cluster(fat, cluster) * fat->sectors_per_cluster);
#else
        ssize_t err = bio_read(dev, buf, fat_sector_for_cluster(fat, cluster) * fat->bytes_per_sector + cluster_offset, to_read);
        if (err != (ssize_t)to_read) {
            TRACEF("short read or error %ld from bio_read\n", err);
            break;
        }
#endif

        offset += to_read;
        len -= to_read;
        amount_read += to_read;

        // Find the next cluster in the file. Start with the last looked up cluster with
        // a cluster-size offset to get it to look up the next one in the list.
        if (len > 0) {
            cluster = file_offset_to_cluster(fat, cluster, fat->bytes_per_cluster);
            if (is_eof_cluster(cluster)) {
                TRACEF("hit EOF too early\n");
                break;
            }
        }
    }

out:
    return amount_read;
}

status_t fat_close_file(filecookie *fcookie) {
    fat_file_t *file = (fat_file_t *)fcookie;
    free(file);
    return NO_ERROR;
}

status_t fat_stat_file(filecookie *fcookie, struct file_stat *stat) {
    fat_file_t *file = (fat_file_t *)fcookie;
    stat->size = file->length;
    stat->is_dir = (file->attributes == fat_attribute::directory);
    return NO_ERROR;
}
