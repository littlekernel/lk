/*
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/bio.h>
#include <lk/cpp.h>
#include <lk/debug.h>
#include <stdint.h>
#include <sys/types.h>

#include "fat_fs.h"

// Local object used to track the state of walking through a file or directory
// one sector at a time using the block cache. Holds a reference to the open
// block cache block and intelligently returns and gets the next one when necessary.
class file_block_iterator {
public:
    // initialize with the starting cluster of the file or directory.
    // special case of starting_cluster == 0 will cause it to track
    // the root directory of a fat 12 or 16 volume, which handle
    // root directories differently.
    file_block_iterator(fat_fs *_fat, uint32_t starting_cluster);
    ~file_block_iterator();

    DISALLOW_COPY_ASSIGN_AND_MOVE(file_block_iterator);

    const uint8_t *get_bcache_ptr(size_t offset) {
        DEBUG_ASSERT(offset < fat->info().bytes_per_sector);
        DEBUG_ASSERT(bcache_buf);
        return (const uint8_t *)bcache_buf + offset;
    }

    // move N sectors ahead in the file, walking the FAT cluster chain as necessary.
    // sectors == 0 will ensure the current block is loaded.
    status_t next_sectors(uint32_t sectors);

    // move one sector
    status_t next_sector() { return next_sectors(1); }

    // return the number of sectors this has moved forward during its lifetime
    uint32_t get_sector_inc_count() const { return sector_inc_count; }
    void reset_sector_inc_count() { sector_inc_count = 0; }

private:
    void put_bcache_block();
    status_t load_current_bcache_block();
    status_t load_bcache_block(bnum_t bnum);

    fat_fs *fat;
    uint32_t cluster;           // current cluster we're on
    uint32_t sector_offset;     // sector number within cluster
    uint32_t sector_inc_count = 0; // number of sectors we have moved forward in the lifetime of this

    void *bcache_buf = nullptr; // current pointer to the bcache, if held
    bnum_t bcache_bnum;         // current block number of the bcache_buf, if valid
};

