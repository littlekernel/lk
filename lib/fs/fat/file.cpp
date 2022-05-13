/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "file.h"

#include <lk/err.h>
#include <lib/bio.h>
#include <lib/fs.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <string.h>
#include <lk/debug.h>

#include "fat_fs.h"
#include "fat_priv.h"
#include "dir.h"

#include "file_iterator.h"

#define LOCAL_TRACE FAT_GLOBAL_TRACE(0)

fat_file::fat_file(fat_fs *f) : fs_(f) {}
fat_file::~fat_file() = default;

void fat_file::inc_ref() {
    ref_++;
    LTRACEF_LEVEL(2, "file %p (%u:%u): ref now %i\n", this, dir_loc_.starting_dir_cluster, dir_loc_.dir_offset, ref_);
    if (ref_ == 1) {
        DEBUG_ASSERT(!list_in_list(&node_));
        fs_->add_to_file_list(this);
    }
    DEBUG_ASSERT(list_in_list(&node_));
}

bool fat_file::dec_ref() {
    ref_--;
    LTRACEF_LEVEL(2, "file %p (%u:%u): ref now %i\n", this, dir_loc_.starting_dir_cluster, dir_loc_.dir_offset, ref_);
    if (ref_ == 0) {
        DEBUG_ASSERT(list_in_list(&node_));
        list_delete(&node_);
        return true;
    }

    return false;
}

status_t fat_file::open_file_priv(const dir_entry &entry, const dir_entry_location &loc) {
    DEBUG_ASSERT(fs_->lock.is_held());

    LTRACEF("found file at location %u:%u\n", loc.starting_dir_cluster, loc.dir_offset);

    // move this out to the wrapper function so we can properly deal with dirs
    //
    // did we get a file?
    if (entry.attributes != fat_attribute::directory) {
        // XXX better attribute testing
        start_cluster_ = entry.start_cluster;
        length_ = entry.length;
        attributes_ = entry.attributes;
        dir_loc_ = loc;
        inc_ref();
        return NO_ERROR;
    } else if (entry.attributes == fat_attribute::directory) {
        // we can open directories, but just not do anything with it except stat
        start_cluster_ = entry.start_cluster;
        length_ = 0;
        attributes_ = entry.attributes;
        dir_loc_ = loc;
        inc_ref();
        return NO_ERROR;
    } else {
        return ERR_NOT_VALID;
    }
}

status_t fat_file::open_file(fscookie *cookie, const char *path, filecookie **fcookie) {
    fat_fs *fs = (fat_fs *)cookie;

    LTRACEF("fscookie %p path '%s' fcookie %p\n", cookie, path, fcookie);

    AutoLock guard(fs->lock);

    // look for the file in the fs
    dir_entry entry;
    dir_entry_location loc;
    status_t err = fat_walk(fs, path, &entry, &loc);
    if (err != NO_ERROR) {
        return err;
    }

    // we found it, see if there's an existing file object
    fat_file *file = fs->lookup_file(loc);
    if (!file) {
        // didn't find an existing one, create a new object
        file = new fat_file(fs);
    }
    DEBUG_ASSERT(file);

    // perform file object private open
    err = file->open_file_priv(entry, loc);
    if (err < 0) {
        delete file;
        return err;
    }

    *fcookie = (filecookie *)file;

    return err;
}

ssize_t fat_file::read_file_priv(void *_buf, const off_t offset, size_t len) {
    uint8_t *buf = (uint8_t *)_buf;

    LTRACEF("file %p buf %p offset %lld len %zu\n", this, _buf, offset, len);

    if (is_dir()) {
        return ERR_NOT_FILE;
    }

    // negative offsets are invalid
    if (offset < 0) {
        return ERR_INVALID_ARGS;
    }

    AutoLock guard(fs_->lock);

    // trim the read to the file
    if (offset >= length_) {
        return 0;
    }

    // above test should ensure offset < 32bit because file->length is 32bit unsigned
    DEBUG_ASSERT(offset <= UINT32_MAX);

    if (offset + len > length_) {
        len = length_ - offset;
    }

    LTRACEF("trimmed offset %lld len %zu\n", offset, len);

    // create a file block iterator and push it forward to the starting point
    uint32_t logical_cluster = offset / fs_->info().bytes_per_cluster;
    uint32_t sector_within_cluster = (offset % fs_->info().bytes_per_cluster) / fs_->info().bytes_per_sector;
    uint32_t offset_within_sector = offset % fs_->info().bytes_per_sector;

    LTRACEF("starting off logical cluster %u, sector within %u, offset within %u\n",
            logical_cluster, sector_within_cluster, offset_within_sector);

    file_block_iterator fbi(fs_, start_cluster_);

    // move it forward to our index point
    // also loads the buffer
    status_t err = fbi.next_sectors(logical_cluster * fs_->info().sectors_per_cluster + sector_within_cluster);
    if (err < 0) {
        LTRACEF("error moving up to starting point!\n");
        return err;
    }

    ssize_t amount_read = 0;
    size_t buf_offset = 0; // offset into the output buffer
    while (buf_offset < len) {
        size_t to_read = MIN(fs_->info().bytes_per_sector - offset_within_sector, len - buf_offset);

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
        DEBUG_ASSERT(offset_within_sector + to_read <= fs_->info().bytes_per_sector);

        // copy out to the buffer
        memcpy(buf + buf_offset, ptr, to_read);

        // next iteration of the loop
        amount_read += to_read;
        buf_offset += to_read;

        // go to the next sector
        offset_within_sector += to_read;
        DEBUG_ASSERT(offset_within_sector <= fs_->info().bytes_per_sector);
        if (offset_within_sector == fs_->info().bytes_per_sector) {
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

ssize_t fat_file::read_file(filecookie *fcookie, void *_buf, const off_t offset, size_t len) {
    fat_file *file = (fat_file *)fcookie;

    return file->read_file_priv(_buf, offset, len);
}

status_t fat_file::stat_file_priv(struct file_stat *stat) {
    AutoLock guard(fs_->lock);

    stat->size = length_;
    stat->is_dir = is_dir();
    return NO_ERROR;
}

status_t fat_file::stat_file(filecookie *fcookie, struct file_stat *stat) {
    fat_file *file = (fat_file *)fcookie;

    return file->stat_file_priv(stat);
}

status_t fat_file::close_file_priv(bool *last_ref) {
    AutoLock guard(fs_->lock);

    // drop a ref to it, which may remove from the global list
    // and return whether or not it was the last ref
    *last_ref = dec_ref();

    return NO_ERROR;
}

status_t fat_file::close_file(filecookie *fcookie) {
    fat_file *file = (fat_file *)fcookie;

    bool last_ref;
    status_t err = file->close_file_priv(&last_ref);
    if (err < 0) {
        return err;
    }

    // if this was the last ref, delete the file
    if (last_ref) {
        LTRACEF("last ref, deleting %p (%u:%u)\n", file, file->dir_loc().starting_dir_cluster, file->dir_loc().dir_offset);
        delete file;
    }

    return NO_ERROR;
}
