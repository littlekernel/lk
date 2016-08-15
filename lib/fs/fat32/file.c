/*
 * Copyright (c) 2015 Steve White
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <err.h>
#include <lib/bio.h>
#include <lib/fs.h>
#include <trace.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <debug.h>

#include "fat_fs.h"
#include "fat32_priv.h"

#define DIR_ENTRY_LENGTH 32
#define USE_CACHE 1

uint32_t fat32_next_cluster_in_chain(fat_fs_t *fat, uint32_t cluster)
{
    uint32_t fat_sector = (cluster) >> 7;
    uint32_t fat_index = (cluster ) & 127;

    uint32_t bnum = (fat->lba_start / fat->bytes_per_sector) + (fat->reserved_sectors + fat_sector);
    uint32_t next_cluster = 0x0fffffff;

#if USE_CACHE
    void *cache_ptr;
    int err = bcache_get_block(fat->cache, &cache_ptr, bnum);
    if (err < 0) {
        printf("bcache_get_block returned: %i\n", err);
    } else {
        if (fat->fat_bits == 32) {
            uint32_t *table = (uint32_t *)cache_ptr;
            next_cluster = table[fat_index];
            LE32SWAP(next_cluster);
        } else if (fat->fat_bits == 16) {
            uint16_t *table = (uint16_t *)cache_ptr;
            next_cluster = table[fat_index];
            LE16SWAP(next_cluster);
            if (next_cluster > 0xfff0) {
                next_cluster |= 0x0fff0000;
            }
        }

        bcache_put_block(fat->cache, bnum);
    }
#else
    uint32_t offset = (bnum * fat->bytes_per_sector) + (fat_index * (fat->fat_bits / 8));
    bio_read(fat->dev, &next_cluster, offset, 4);
    LE32SWAP(next_cluster);
#endif
    return next_cluster;
}

static inline off_t fat32_offset_for_cluster(fat_fs_t *fat, uint32_t cluster)
{
    off_t cluster_begin_lba = fat->reserved_sectors + (fat->fat_count * fat->sectors_per_fat);
    return fat->lba_start + (cluster_begin_lba + (cluster - 2) * fat->sectors_per_cluster) * fat->bytes_per_sector;
}

char *fat32_dir_get_filename(uint8_t *dir, off_t offset, int lfn_sequences)
{
    int result_len = 1 + (lfn_sequences == 0 ? 12 : (lfn_sequences * 26));
    char *result = malloc(result_len);
    int j = 0;
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

status_t fat32_open_file(fscookie *cookie, const char *path, filecookie **fcookie)
{
    fat_fs_t *fat = (fat_fs_t *)cookie;
    status_t result = ERR_GENERIC;

    uint8_t *dir = malloc(fat->bytes_per_cluster);
    uint32_t dir_cluster = fat->root_cluster;
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
        bio_read(fat->dev, dir, fat32_offset_for_cluster(fat, dir_cluster), fat->bytes_per_cluster);

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

            char *filename = fat32_dir_get_filename(dir, offset, lfn_sequences);
            lfn_sequences = 0;

            matched = (strnicmp(ptr, filename, strlen(filename)) == 0);
            free(filename);

            if (matched) {
                uint16_t target_cluster = fat_read16(dir, offset + 0x1a);
                if (done == true) {
                    file = malloc(sizeof(fat_file_t));
                    file->fat_fs = fat;
                    file->start_cluster = target_cluster;
                    file->length = fat_read32(dir, offset + 0x1c);
                    file->attributes = dir[0x0B + offset];
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
            dir_cluster = fat32_next_cluster_in_chain(fat, dir_cluster);
            if (dir_cluster == 0x0fffffff) {
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

ssize_t fat32_read_file(filecookie *fcookie, void *buf, off_t offset, size_t len)
{
    fat_file_t *file = (fat_file_t *)fcookie;
    fat_fs_t *fat = file->fat_fs;
    bdev_t *dev = fat->dev;

    uint32_t cluster = 0;
    if (offset <= fat->bytes_per_cluster) {
        cluster = file->start_cluster;
    } else {
        // XXX: support non-0 offsets
        TRACE;
        return -1;
    }

    uint32_t length = file->length;
    uint32_t amount_read = 0;

    do {
        off_t lba_addr = fat32_offset_for_cluster(fat, cluster);

        uint32_t to_read = fat->bytes_per_cluster;
        uint32_t next_cluster = 0;
        while ((next_cluster = fat32_next_cluster_in_chain(fat, cluster)) == cluster + 1) {
            cluster = next_cluster;
            to_read += fat->bytes_per_cluster;
        }
        cluster = next_cluster;

        to_read = MIN(length - amount_read, to_read);
        // XXX: support non-0 offsets
        int err = bio_read(dev, buf+amount_read, lba_addr, to_read);
        if (err < 0) {
            return err;
        }

        amount_read += to_read;

        if (amount_read < length) {
            if (cluster == 0x0fffffff) {
                printf("no more clusters, amount_read=%i, to_read=%i\n", amount_read, to_read);
                break;
            }
        }
    } while (amount_read < length);

    return amount_read;
}

status_t fat32_close_file(filecookie *fcookie)
{
    fat_file_t *file = (fat_file_t *)fcookie;
    free(file);
    return NO_ERROR;
}

status_t fat32_stat_file(filecookie *fcookie, struct file_stat *stat)
{
    fat_file_t *file = (fat_file_t *)fcookie;
    stat->size = file->length;
    stat->is_dir = (file->attributes == fat_attribute_directory);
    return NO_ERROR;
}
