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

#include "file_iterator.h"

#define LOCAL_TRACE 0

struct fat_file {
    fat_fs_t *fat_fs;

    uint32_t start_cluster;
    uint32_t length;

    fat_attribute attributes;

    bool is_dir() const { return attributes == fat_attribute::directory; }
};

status_t fat_open_file(fscookie *cookie, const char *path, filecookie **fcookie) {
    fat_fs_t *fat = (fat_fs_t *)cookie;

    LTRACEF("fscookie %p path '%s' fcookie %p\n", cookie, path, fcookie);

    fat_file *file = NULL;

    AutoLock guard(fat->lock);

    dir_entry entry;
    status_t err = fat_walk(fat, path, &entry);
    if (err != NO_ERROR) {
        return err;
    }

    // did we get a file?
    if (entry.attributes != fat_attribute::directory) {
        // XXX better attribute testing
        file = new fat_file;
        file->fat_fs = fat;
        file->start_cluster = entry.start_cluster;
        file->length = entry.length;
        file->attributes = entry.attributes;
        *fcookie = (filecookie *)file;
        return NO_ERROR;
    } else if (entry.attributes == fat_attribute::directory) {
        // we can open directories, but just not do anything with it except stat
        file = new fat_file;
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

ssize_t fat_read_file(filecookie *fcookie, void *_buf, const off_t offset, size_t len) {
    fat_file *file = (fat_file *)fcookie;
    fat_fs_t *fat = file->fat_fs;
    uint8_t *buf = (uint8_t *)_buf;

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

    // above test should ensure offset < 32bit because file->length is 32bit unsigned
    DEBUG_ASSERT(offset <= UINT32_MAX);

    if (offset + len > file->length) {
        len = file->length - offset;
    }

    LTRACEF("trimmed offset %lld len %zu\n", offset, len);

    // create a file block iterator and push it forward to the starting point
    uint32_t logical_cluster = offset / fat->bytes_per_cluster;
    uint32_t sector_within_cluster = (offset % fat->bytes_per_cluster) / fat->bytes_per_sector;
    uint32_t offset_within_sector = offset % fat->bytes_per_sector;

    LTRACEF("starting off logical cluster %u, sector within %u, offset within %u\n",
            logical_cluster, sector_within_cluster, offset_within_sector);

    file_block_iterator fbi(fat, file->start_cluster);

    // move it forward to our index point
    // also loads the buffer
    status_t err = fbi.next_sectors(logical_cluster * fat->sectors_per_cluster + sector_within_cluster);
    if (err < 0) {
        LTRACEF("error moving up to starting point!\n");
        return err;
    }

    ssize_t amount_read = 0;
    size_t buf_offset = 0; // offset into the output buffer
    while (buf_offset < len) {
        size_t to_read = MIN(fat->bytes_per_sector - offset_within_sector, len - buf_offset);

        LTRACEF("top of loop: sector offset %u, buf offset %zu, remaining len %zu, to_read %zu\n",
                offset_within_sector, buf_offset, len, to_read);

        // grab a pointer directly to the block cache
        const uint8_t *ptr;
        ptr = fbi.get_bcache_ptr(offset_within_sector);
        if (!ptr) {
            LTRACEF("error getting pointer to file in cache\n");
            return ERR_IO;
        }

        // make sure we dont do something silly
        DEBUG_ASSERT(buf_offset + to_read <= len);
        DEBUG_ASSERT(buf_offset % fat->bytes_per_sector + to_read <= fat->bytes_per_sector);

        // copy out to the buffer
        memcpy(buf + buf_offset, ptr, to_read);

        // next iteration of the loop
        amount_read += to_read;
        buf_offset += to_read;

        // go to the next sector
        offset_within_sector += to_read;
        DEBUG_ASSERT(offset_within_sector <= fat->bytes_per_sector);
        if (offset_within_sector == fat->bytes_per_sector) {
            offset_within_sector = 0;
            // push the iterator forward to next sector
            err = fbi.next_sector();
            if (err < 0) {
                return err;
            }
        }
    }

    return amount_read;
}

status_t fat_stat_file(filecookie *fcookie, struct file_stat *stat) {
    fat_file *file = (fat_file *)fcookie;
    fat_fs_t *fat = file->fat_fs;

    AutoLock guard(fat->lock);

    stat->size = file->length;
    stat->is_dir = (file->attributes == fat_attribute::directory);
    return NO_ERROR;
}

status_t fat_close_file(filecookie *fcookie) {
    fat_file *file = (fat_file *)fcookie;
    fat_fs_t *fat = file->fat_fs;

    AutoLock guard(fat->lock);

    // TODO: keep a list of open files to keep from getting duplicated

    delete file;

    return NO_ERROR;
}
