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
#include <stdlib.h>
#include <string.h>
#include <lk/debug.h>

#include "fat_fs.h"
#include "fat_priv.h"

#define LOCAL_TRACE 0

status_t fat_open_file(fscookie *cookie, const char *path, filecookie **fcookie) {
    fat_fs_t *fat = (fat_fs_t *)cookie;

    LTRACEF("fscookie %p path '%s' fcookie %p\n", cookie, path, fcookie);

    fat_file_t *file = NULL;

    AutoLock guard(fat->lock);

    dir_entry entry;
    status_t err = fat_walk(fat, path, &entry);
    if (err != NO_ERROR) {
        return err;
    }

    // did we get a file?
    if (entry.attributes != fat_attribute::directory) {
        // XXX better attribute testing
        file = new fat_file_t;
        file->fat_fs = fat;
        file->start_cluster = entry.start_cluster;
        file->length = entry.length;
        file->attributes = entry.attributes;
        *fcookie = (filecookie *)file;
        return NO_ERROR;
    } else if (entry.attributes == fat_attribute::directory) {
        // we can open directories, but just not do anything with it except stat
        file = new fat_file_t;
        file->fat_fs = fat;
        file->start_cluster = entry.start_cluster;
        file->length = 0;
        file->attributes = entry.attributes;
        *fcookie = (filecookie *)file;
        return NO_ERROR;
    } else {
        return ERR_NOT_VALID;
    }
}

ssize_t fat_read_file(filecookie *fcookie, void *_buf, off_t offset, size_t len) {
    fat_file_t *file = (fat_file_t *)fcookie;
    fat_fs_t *fat = file->fat_fs;
    bdev_t *dev = fat->dev;

    LTRACEF("fcookie %p buf %p offset %lld len %zu\n", fcookie, _buf, offset, len);

    if (file->is_dir()) {
        return ERR_NOT_FILE;
    }

    // negative offsets are invalid
    if (offset < 0) {
        return ERR_INVALID_ARGS;
    }

    AutoLock guard(fat->lock);

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

status_t fat_stat_file(filecookie *fcookie, struct file_stat *stat) {
    fat_file_t *file = (fat_file_t *)fcookie;
    fat_fs_t *fat = file->fat_fs;

    AutoLock guard(fat->lock);

    stat->size = file->length;
    stat->is_dir = (file->attributes == fat_attribute::directory);
    return NO_ERROR;
}

status_t fat_close_file(filecookie *fcookie) {
    fat_file_t *file = (fat_file_t *)fcookie;
    fat_fs_t *fat = file->fat_fs;

    AutoLock guard(fat->lock);

    // TODO: keep a list of open files to keep from getting duplicated

    delete file;

    return NO_ERROR;
}
