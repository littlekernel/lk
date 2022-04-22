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

#define LOCAL_TRACE 1

uint32_t fat_next_cluster_in_chain(fat_fs_t *fat, uint32_t cluster) {
    LTRACEF("cluster %#x\n", cluster);

    // offset in bytes into the FAT for this entry
    uint32_t fat_offset;
    if (fat->fat_bits == 32) {
        fat_offset = cluster * 4;
    } else if (fat->fat_bits == 16) {
        fat_offset = cluster * 2;
    } else {
        fat_offset = cluster + (cluster / 2);
        PANIC_UNIMPLEMENTED;
    }

    uint32_t fat_sector = fat_offset / fat->bytes_per_sector;

    uint32_t bnum = fat->reserved_sectors + fat_sector;
    uint32_t next_cluster = EOF_CLUSTER;

    void *cache_ptr;
    int err = bcache_get_block(fat->cache, &cache_ptr, bnum);
    if (err < 0) {
        printf("bcache_get_block returned: %i\n", err);
        return EOF_CLUSTER;
    }

    if (fat->fat_bits == 32) {
        uint32_t *table = (uint32_t *)cache_ptr;
        uint32_t index = fat_offset % fat->bytes_per_sector / 4;
        next_cluster = table[index];
        LE32SWAP(next_cluster);

        // mask out the top nibble
        next_cluster &= 0x0fffffff;
    } else if (fat->fat_bits == 16) {
        uint16_t *table = (uint16_t *)cache_ptr;
        uint32_t index = fat_offset % fat->bytes_per_sector / 2;
        next_cluster = table[index];
        LE16SWAP(next_cluster);

        if (next_cluster > 0xfff0) {
            next_cluster |= 0x0fff0000;
        }
    } else {
        // implement logic for fat12 and sector straddling logic.
        PANIC_UNIMPLEMENTED;
    }

    bcache_put_block(fat->cache, bnum);

    LTRACEF("returning cluster %#x\n", next_cluster);

    return next_cluster;
}

// given a starting fat cluster, walk the fat chain for offset bytes, returning a new cluster or end of file
uint32_t file_offset_to_cluster(fat_fs_t *fat, uint32_t start_cluster, off_t offset) {
    // negative offsets do not make sense
    DEBUG_ASSERT(offset >= 0);
    if (offset < 0) {
        return EOF_CLUSTER;
    }

    // starting at the start cluster, walk forward N clusters, based on how far
    // the offset is units of cluster bytes
    uint32_t found_cluster = start_cluster;
    size_t clusters_to_walk = (size_t)offset / fat->bytes_per_cluster;
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

uint32_t fat_sector_for_cluster(fat_fs_t *fat, uint32_t cluster) {
    // cluster 0 and 1 are undefined
    DEBUG_ASSERT(cluster >= 2);
    DEBUG_ASSERT(cluster < fat->total_clusters);
    if (cluster >= fat->total_clusters) {
        return 0;
    }

    uint32_t sector = fat->data_start_sector + (cluster - 2) * fat->sectors_per_cluster;

    DEBUG_ASSERT(sector < fat->total_sectors);

    return sector;
}

ssize_t fat_read_cluster(fat_fs_t *fat, void *buf, uint32_t cluster) {
    LTRACEF("buf %p, cluster %u\n", buf, cluster);

    auto sector = fat_sector_for_cluster(fat, cluster);
    return bio_read_block(fat->dev, buf, sector, fat->sectors_per_cluster);
}


