/*
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "file_iterator.h"

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

#define LOCAL_TRACE FAT_GLOBAL_TRACE(0)

file_block_iterator::file_block_iterator(fat_fs *_fat, uint32_t starting_cluster) : fat(_fat) {
    if (starting_cluster == 0) {
        // special case on fat12/16 to represent the root dir.
        // load 0 into cluster and use sector_offset as relative to the
        // start of the volume.
        DEBUG_ASSERT(fat->info().fat_bits == 12 || fat->info().fat_bits == 16);
        DEBUG_ASSERT(fat->info().root_start_sector != 0);
        cluster = 0; // from now on out, cluster 0 will represent the root dir
        sector_offset = fat->info().root_start_sector;
    } else {
        cluster = starting_cluster;
        sector_offset = 0;
    }
}

file_block_iterator::~file_block_iterator() {
    put_bcache_block();
}

// move N sectors ahead
status_t file_block_iterator::next_sectors(uint32_t sectors) {
    if (cluster == 0) {
        // on linear dirs it's just incrementing sectors
        sector_offset += sectors;
        // are we at the end of the root dir?
        if (sector_offset >= fat->info().root_start_sector + fat->info().root_dir_sectors) {
            return ERR_OUT_OF_RANGE;
        }
    } else {
        // for non linear dirs we step sector by sector into clusters
        // and then wrap to the next cluster.
        for (uint32_t i = 0; i < sectors; i++) {
            sector_offset++;
            if (sector_offset == fat->info().sectors_per_cluster) {
                sector_offset = 0;

                cluster = fat_next_cluster_in_chain(fat, cluster);
                if (is_eof_cluster(cluster)) {
                    return ERR_OUT_OF_RANGE;
                }
            }
        }
    }
    // track the number of sectors we've moved forward
    sector_inc_count += sectors;

    return load_current_bcache_block();
}

status_t file_block_iterator::load_current_bcache_block() {
    // close the current bcache
    put_bcache_block();

    if (cluster == 0) {
        // we're in a linear root dir, the sector_offset variable represents the raw sector number
        return load_bcache_block(sector_offset);
    } else { // cluster != 0
        DEBUG_ASSERT(sector_offset < fat->info().bytes_per_sector);

        // compute the sector we should be on given the cluster and sector_offset
        auto sector = fat_sector_for_cluster(fat, cluster) + sector_offset;

        return load_bcache_block(sector);
    }
}

void file_block_iterator::put_bcache_block() {
    if (bcache_buf) {
        bcache_put_block(fat->bcache(), bcache_bnum);
        bcache_buf = nullptr;
        bcache_bnum = 0;
    }
}

status_t file_block_iterator::load_bcache_block(bnum_t bnum) {
    DEBUG_ASSERT(!bcache_buf);

    auto err = bcache_get_block(fat->bcache(), &bcache_buf, bnum);
    if (err >= 0) {
        DEBUG_ASSERT(bcache_buf);
        bcache_bnum = bnum;
    }

    return err;
}

