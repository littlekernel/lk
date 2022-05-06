/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/trace.h>
#include <endian.h>
#include <stdint.h>
#include <stdio.h>

#include "fat_fs.h"
#include "fat_priv.h"

#define LOCAL_TRACE FAT_GLOBAL_TRACE(0)

uint32_t fat_next_cluster_in_chain(fat_fs *fat, uint32_t cluster) {
    DEBUG_ASSERT(fat->lock.is_held());

    // offset in bytes into the FAT for this entry
    uint32_t fat_offset;
    if (fat->info().fat_bits == 32) {
        fat_offset = cluster * 4;
    } else if (fat->info().fat_bits == 16) {
        fat_offset = cluster * 2;
    } else {
        fat_offset = cluster + (cluster / 2);
    }
    LTRACEF("cluster %#x, fat_offset %u\n", cluster, fat_offset);

    const uint32_t fat_sector = fat_offset / fat->info().bytes_per_sector;
    uint32_t bnum = fat->info().reserved_sectors + fat_sector;
    const uint32_t fat_offset_in_sector = fat_offset % fat->info().bytes_per_sector;

    // grab a pointer to the sector holding the fat entry
    void *cache_ptr;
    int err = bcache_get_block(fat->bcache(), &cache_ptr, bnum);
    if (err < 0) {
        printf("bcache_get_block returned: %i\n", err);
        return EOF_CLUSTER;
    }

    uint32_t next_cluster;
    if (fat->info().fat_bits == 32) {
        const auto *table = (const uint32_t *)cache_ptr;
        const auto index = fat_offset_in_sector / 4;
        next_cluster = table[index];
        LE32SWAP(next_cluster);

        // mask out the top nibble
        next_cluster &= 0x0fffffff;
    } else if (fat->info().fat_bits == 16) {
        const auto *table = (const uint16_t *)cache_ptr;
        const auto index = fat_offset_in_sector / 2;
        next_cluster = table[index];
        LE16SWAP(next_cluster);

        // if it's a EOF 16 bit entry, extend it so that it looks to be 32bit
        if (next_cluster > 0xfff0) {
            next_cluster |= 0x0fff0000;
        }
    } else { // fat12
        DEBUG_ASSERT(fat->info().fat_bits == 12);
        DEBUG_ASSERT(fat_offset_in_sector < fat->info().bytes_per_sector);

        if (fat_offset_in_sector != (fat->info().bytes_per_sector - 1)) {
            // normal, non sector straddling logic
            next_cluster = fat_read16(cache_ptr, fat_offset_in_sector);
        } else {
            // need to straddle a fat sector

            // read the first byte of the entry
            next_cluster = ((const uint8_t *)cache_ptr)[fat_offset_in_sector];

            // close the block cache and open the next sector
            bcache_put_block(fat->bcache(), bnum);
            bnum++;
            err = bcache_get_block(fat->bcache(), &cache_ptr, bnum);
            if (err < 0) {
                printf("bcache_get_block returned: %i\n", err);
                return EOF_CLUSTER;
            }

            // read the second byte
            next_cluster |= ((const uint8_t *)cache_ptr)[0] << 8;
        }

        // odd cluster, shift over to get our value
        if (cluster & 1) {
            next_cluster >>= 4;
        } else {
            next_cluster &= 0x0fff;
        }

        // if it's a EOF 12 bit entry, extend it so that it looks to be 32bit
        if (next_cluster > 0xff0) {
            next_cluster |= 0x0ffff000;
        }
    }

    // return the sector to the block cache
    bcache_put_block(fat->bcache(), bnum);

    LTRACEF("returning cluster %#x\n", next_cluster);

    return next_cluster;
}

// given a starting fat cluster, walk the fat chain for offset bytes, returning a new cluster or end of file
uint32_t file_offset_to_cluster(fat_fs *fat, uint32_t start_cluster, off_t offset) {
    DEBUG_ASSERT(fat->lock.is_held());

    // negative offsets do not make sense
    DEBUG_ASSERT(offset >= 0);
    if (offset < 0) {
        return EOF_CLUSTER;
    }

    // starting at the start cluster, walk forward N clusters, based on how far
    // the offset is units of cluster bytes
    uint32_t found_cluster = start_cluster;
    size_t clusters_to_walk = (size_t)offset / fat->info().bytes_per_cluster;
    while (clusters_to_walk > 0) {
        // walk foward these many clusters, returning the FAT entry at that spot
        found_cluster = fat_next_cluster_in_chain(fat, found_cluster);
        if (is_eof_cluster(found_cluster)) {
            break;
        }
        clusters_to_walk--;
    }

    return found_cluster;
}

uint32_t fat_sector_for_cluster(fat_fs *fat, uint32_t cluster) {
    DEBUG_ASSERT(fat->lock.is_held());

    // cluster 0 and 1 are undefined
    DEBUG_ASSERT(cluster >= 2);
    DEBUG_ASSERT(cluster < fat->info().total_clusters);
    if (cluster >= fat->info().total_clusters) {
        return 0;
    }

    uint32_t sector = fat->info().data_start_sector + (cluster - 2) * fat->info().sectors_per_cluster;

    DEBUG_ASSERT(sector < fat->info().total_sectors);

    return sector;
}

ssize_t fat_read_cluster(fat_fs *fat, void *buf, uint32_t cluster) {
    DEBUG_ASSERT(fat->lock.is_held());

    LTRACEF("buf %p, cluster %u\n", buf, cluster);

    auto sector = fat_sector_for_cluster(fat, cluster);
    return bio_read_block(fat->dev(), buf, sector, fat->info().sectors_per_cluster);
}


