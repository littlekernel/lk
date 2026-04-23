/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <lk/trace.h>
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <lib/bcache/bcache_block_ref.h>

#include "fat_fs.h"
#include "fat_priv.h"

#define LOCAL_TRACE FAT_GLOBAL_TRACE(0)

// given a cluster number and fat table index, compute the sector and the offset within
// the sector where the fat entry exists.
static void compute_fat_entry_address_for_table(fat_fs *fat, const uint32_t cluster,
                                                uint32_t fat_index, uint32_t *sector,
                                                uint32_t *offset_within_sector) {
    DEBUG_ASSERT(cluster < fat->info().total_clusters);
    DEBUG_ASSERT(fat_index < fat->info().fat_count);

    // offset in bytes into the FAT for this entry
    uint32_t fat_offset;
    if (fat->info().fat_bits == 32) {
        fat_offset = cluster * 4;
    } else if (fat->info().fat_bits == 16) {
        fat_offset = cluster * 2;
    } else {
        fat_offset = cluster + (cluster / 2);
    }
    LTRACEF_LEVEL(2, "cluster %#x, fat_offset %u\n", cluster, fat_offset);

    const uint32_t fat_sector = fat_offset / fat->info().bytes_per_sector;
    const uint32_t fat_base_sector = fat->info().reserved_sectors +
                                     fat_index * fat->info().sectors_per_fat;
    *sector = fat_base_sector + fat_sector;
    *offset_within_sector = fat_offset % fat->info().bytes_per_sector;
}

// given a cluster number, compute the sector and the offset within the sector where
// the fat entry exists in the currently active FAT.
static void compute_fat_entry_address(fat_fs *fat, const uint32_t cluster,
                                      uint32_t *sector, uint32_t *offset_within_sector) {
    compute_fat_entry_address_for_table(fat, cluster, fat->info().active_fat,
                                        sector, offset_within_sector);
}

// return the next cluster # in a chain, given a starting cluster
uint32_t fat_next_cluster_in_chain(fat_fs *fat, uint32_t cluster) {
    DEBUG_ASSERT(fat->lock.is_held());

    if (is_eof_cluster(cluster)) {
        return cluster;
    }

    // compute the starting address
    uint32_t sector;
    uint32_t fat_offset_in_sector;
    compute_fat_entry_address(fat, cluster, &sector, &fat_offset_in_sector);

    // grab a pointer to the sector holding the fat entry
    bcache_block_ref bref(fat->bcache());
    int err = bref.get_block(sector);
    if (err < 0) {
        printf("bcache_get_block returned: %i\n", err);
        return EOF_CLUSTER;
    }

    uint32_t next_cluster;
    if (fat->info().fat_bits == 32) {
        const auto *table = (const uint32_t *)bref.ptr();
        const auto index = fat_offset_in_sector / 4;
        next_cluster = table[index];
        LE32SWAP(next_cluster);

        // mask out the top nibble
        next_cluster &= 0x0fffffff;
    } else if (fat->info().fat_bits == 16) {
        const auto *table = (const uint16_t *)bref.ptr();
        const auto index = fat_offset_in_sector / 2;
        next_cluster = table[index];
        LE16SWAP(next_cluster);

        // if it's a EOF 16 bit entry, extend it so that it looks to be 32bit
        if (next_cluster >= 0xfff8) {
            next_cluster |= 0x0fff0000;
        }
    } else { // fat12
        DEBUG_ASSERT(fat->info().fat_bits == 12);
        DEBUG_ASSERT(fat_offset_in_sector < fat->info().bytes_per_sector);

        if (fat_offset_in_sector != (fat->info().bytes_per_sector - 1)) {
            // normal, non sector straddling logic
            next_cluster = fat_read16(bref.ptr(), fat_offset_in_sector);
        } else {
            // need to straddle a fat sector

            // read the first byte of the entry
            next_cluster = ((const uint8_t *)bref.ptr())[fat_offset_in_sector];

            // close the block cache and open the next sector
            err = bref.get_block(++sector);
            if (err < 0) {
                printf("bcache_get_block returned: %i\n", err);
                return EOF_CLUSTER;
            }

            // read the second byte
            next_cluster |= ((const uint8_t *)bref.ptr())[0] << 8;
        }

        // odd cluster, shift over to get our value
        if (cluster & 1) {
            next_cluster >>= 4;
        } else {
            next_cluster &= 0x0fff;
        }

        // if it's a EOF 12 bit entry, extend it so that it looks to be 32bit
        if (next_cluster >= 0xff8) {
            next_cluster |= 0x0ffff000;
        }
    }

    LTRACEF("returning cluster %#x\n", next_cluster);

    return next_cluster;
}

uint32_t fat_find_last_cluster_in_chain(fat_fs *fat, uint32_t starting_cluster) {
    LTRACEF("fat %p, starting cluster %u\n", fat, starting_cluster);

    if (starting_cluster == 0) {
        return 0;
    }

    uint32_t last_cluster = starting_cluster;
    for (;;) {
        uint32_t next = fat_next_cluster_in_chain(fat, last_cluster);
        if (is_eof_cluster(next)) {
            return last_cluster;
        }
        last_cluster = next;
    }
}

// write a value into a fat entry
static status_t fat_mark_entry(fat_fs *fat, uint32_t cluster, uint32_t val) {
    LTRACEF("fat %p, cluster %u, val %#x\n", fat, cluster, val);

    // keep the value within the representable fat entry size.
    uint32_t entry_value = val;
    if (fat->info().fat_bits == 32) {
        entry_value &= 0x0fffffff;
    } else if (fat->info().fat_bits == 16) {
        entry_value &= 0xffff;
    } else {
        entry_value &= 0x0fff;
    }

    // keep all FAT copies in sync.
    for (uint32_t fat_index = 0; fat_index < fat->info().fat_count; fat_index++) {
        uint32_t sector;
        uint32_t fat_offset_in_sector;
        compute_fat_entry_address_for_table(fat, cluster, fat_index, &sector,
                                            &fat_offset_in_sector);

        bcache_block_ref bref(fat->bcache());
        int err = bref.get_block(sector);
        if (err < 0) {
            printf("bcache_get_block returned: %i\n", err);
            return err;
        }

        if (fat->info().fat_bits == 32) {
            auto *table = (uint32_t *)bref.ptr();
            const auto index = fat_offset_in_sector / 4;

            // preserve the reserved high nibble in FAT32 entries.
            uint32_t old_entry = table[index];
            LE32SWAP(old_entry);
            uint32_t new_entry = (old_entry & 0xf0000000) | entry_value;
            table[index] = LE32(new_entry);
            bref.mark_dirty();
        } else if (fat->info().fat_bits == 16) {
            auto *table = (uint16_t *)bref.ptr();
            const auto index = fat_offset_in_sector / 2;
            table[index] = LE16((uint16_t)entry_value);
            bref.mark_dirty();
        } else { // fat12
            DEBUG_ASSERT(fat_offset_in_sector < fat->info().bytes_per_sector);
            auto *block = (uint8_t *)bref.ptr();

            if (fat_offset_in_sector != (fat->info().bytes_per_sector - 1)) {
                uint16_t old_entry = fat_read16(block, fat_offset_in_sector);
                uint16_t new_entry;
                if (cluster & 1) {
                    new_entry = (uint16_t)((old_entry & 0x000f) | (entry_value << 4));
                } else {
                    new_entry = (uint16_t)((old_entry & 0xf000) | entry_value);
                }
                fat_write16(block, fat_offset_in_sector, new_entry);
                bref.mark_dirty();
            } else {
                // The 12-bit entry straddles two FAT sectors.
                uint8_t first_byte = block[fat_offset_in_sector];
                if (cluster & 1) {
                    block[fat_offset_in_sector] =
                        (uint8_t)((first_byte & 0x0f) | ((entry_value & 0x000f) << 4));
                } else {
                    block[fat_offset_in_sector] = (uint8_t)(entry_value & 0xff);
                }
                bref.mark_dirty();

                err = bref.get_block(++sector);
                if (err < 0) {
                    printf("bcache_get_block returned: %i\n", err);
                    return err;
                }

                block = (uint8_t *)bref.ptr();
                if (cluster & 1) {
                    block[0] = (uint8_t)((entry_value >> 4) & 0xff);
                } else {
                    block[0] = (uint8_t)((block[0] & 0xf0) | ((entry_value >> 8) & 0x0f));
                }
                bref.mark_dirty();
            }
        }
    }

    return NO_ERROR;
}

// allocate a cluster chain
// start_cluster is an existing cluster that it should link to (or 0 if its the first in the chain)
// count is number of clusters to allocate
// first and last cluster are the first and lastly allocated in the new part of the list (may be the same)
status_t fat_allocate_cluster_chain(fat_fs *fat, uint32_t start_cluster, uint32_t count,
                                    uint32_t *first_cluster, uint32_t *last_cluster,
                                    bool zero_new_blocks) {
    LTRACEF("fat %p, starting %u, count %u, zero %u\n", fat, start_cluster, count, zero_new_blocks);

    DEBUG_ASSERT(fat->lock.is_held());

    *first_cluster = *last_cluster = 0;
    uint32_t prev_cluster = start_cluster;

    // TODO: start search at start_cluster instead of the beginning
    uint32_t search_cluster = 2; // first 2 clusters are reserved

    // compute the starting address
    uint32_t sector;
    uint32_t fat_offset_in_sector;
    compute_fat_entry_address(fat, search_cluster, &sector, &fat_offset_in_sector);

    // grab a pointer to the sector holding the fat entry
    bcache_block_ref bref(fat->bcache());
    int err = bref.get_block(sector);
    if (err < 0) {
        printf("bcache_get_block returned: %i\n", err);
        return EOF_CLUSTER;
    }

    // start walking forward until we have found up to count clusters or we run out of clusters
    const auto total_clusters = fat->info().total_clusters;
    while (count > 0) {
        if (fat->info().fat_bits == 12) {
            // FAT12 entries do not advance linearly by whole bytes, so recompute from
            // the current cluster each iteration.
            compute_fat_entry_address(fat, search_cluster, &sector, &fat_offset_in_sector);
        }

        uint32_t entry;
        if (fat->info().fat_bits == 32) {
            const auto *table = (const uint32_t *)bref.ptr();
            const auto index = fat_offset_in_sector / 4;
            entry = table[index];
            LE32SWAP(entry);

            // mask out the top nibble
            entry &= 0x0fffffff;
        } else if (fat->info().fat_bits == 16) {
            const auto *table = (const uint16_t *)bref.ptr();
            const auto index = fat_offset_in_sector / 2;
            entry = LE16(table[index]);
        } else { // fat12
            entry = fat_next_cluster_in_chain(fat, search_cluster);
        }

        LTRACEF_LEVEL(2, "search_cluster %u, sector %u, offset %u: entry %#x\n", search_cluster, sector, fat_offset_in_sector, entry);
        if (entry == 0) {
            // its a free entry, allocate it and move on
            LTRACEF("found free cluster %u, sector %u, offset %u\n", search_cluster, sector, fat_offset_in_sector);

            // zero the cluster first
            if (zero_new_blocks) {
                fat_zero_cluster(fat, search_cluster);
            }

            // add it to the chain
            if (prev_cluster > 0) {
                // link the last cluster we had found before to this one.
                // NOTE: may be start_cluster if this is the first iteration
                fat_mark_entry(fat, prev_cluster, search_cluster);
            }
            if (*first_cluster == 0) {
                // this is the first one in the chain
                *first_cluster = search_cluster;
            }
            *last_cluster = search_cluster;
            prev_cluster = search_cluster;

            // Keep FAT32 FSInfo metadata up to date for allocated clusters.
            fat->adjust_fsinfo_free_clusters(-1);
            fat->set_fsinfo_next_free(search_cluster + 1);

            count--;
            if (count == 0) {
                // we're at the end of this chain, mark it as EOF
                fat_mark_entry(fat, search_cluster, EOF_CLUSTER);

                // early terminate here, since there's no point pushing
                // to the next sector
                break;
            }
        }

        // helper to move to the next sector, dropping old block cache and
        // loading the next one.
        auto inc_sector = [&bref, &sector]() -> status_t {
            status_t localerr = bref.get_block(++sector);
            if (localerr < 0) {
                printf("bcache_get_block returned: %i\n", localerr);
                return EOF_CLUSTER;
            }

            return NO_ERROR;
        };

        // next entry
        search_cluster++;
        if (search_cluster >= total_clusters) {
            // no more clusters, abort
            break;
        }
        if (fat->info().fat_bits == 32) {
            fat_offset_in_sector += 4;
            if (fat_offset_in_sector == fat->info().bytes_per_sector) {
                fat_offset_in_sector = 0;
                if ((err = inc_sector()) != NO_ERROR) {
                    return err;
                }
            }
        } else if (fat->info().fat_bits == 16) {
            fat_offset_in_sector += 2;
            if (fat_offset_in_sector == fat->info().bytes_per_sector) {
                fat_offset_in_sector = 0;
                if ((err = inc_sector()) != NO_ERROR) {
                    return err;
                }
            }
        } else { // fat12
            // address is recomputed from the cluster number at top of loop
        }
    }

    if (count == 0) {
        return NO_ERROR;
    } else {
        return ERR_NO_RESOURCES;
    }
}

status_t fat_free_cluster_chain(fat_fs *fat, uint32_t start_cluster) {
    DEBUG_ASSERT(fat->lock.is_held());

    if (start_cluster < 2 || start_cluster >= fat->info().total_clusters) {
        return NO_ERROR;
    }

    fat->set_fsinfo_next_free(start_cluster);

    uint32_t cluster = start_cluster;
    while (cluster >= 2 && cluster < fat->info().total_clusters) {
        uint32_t next = fat_next_cluster_in_chain(fat, cluster);

        status_t err = fat_mark_entry(fat, cluster, 0);
        if (err != NO_ERROR) {
            return err;
        }

        fat->adjust_fsinfo_free_clusters(1);

        if (is_eof_cluster(next) || next < 2 || next >= fat->info().total_clusters) {
            break;
        }

        cluster = next;
    }

    return NO_ERROR;
}

status_t fat_truncate_cluster_chain(fat_fs *fat, uint32_t keep_last_cluster) {
    DEBUG_ASSERT(fat->lock.is_held());

    if (keep_last_cluster < 2 || keep_last_cluster >= fat->info().total_clusters) {
        return ERR_INVALID_ARGS;
    }

    uint32_t first_free = fat_next_cluster_in_chain(fat, keep_last_cluster);

    status_t err = fat_mark_entry(fat, keep_last_cluster, EOF_CLUSTER);
    if (err != NO_ERROR) {
        return err;
    }

    if (!is_eof_cluster(first_free) && first_free >= 2 &&
        first_free < fat->info().total_clusters) {
        err = fat_free_cluster_chain(fat, first_free);
        if (err != NO_ERROR) {
            return err;
        }
    }

    return NO_ERROR;
}

// return the disk sector that corresponds to a cluster number, with
// appropriate offsets applied
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

// read a cluster directly into a buffer, using the bcache
ssize_t fat_read_cluster(fat_fs *fat, void *buf, uint32_t cluster) {
    DEBUG_ASSERT(fat->lock.is_held());

    LTRACEF("buf %p, cluster %u\n", buf, cluster);

    auto sector = fat_sector_for_cluster(fat, cluster);

    uint8_t *buf8 = (uint8_t *)buf;
    for (size_t i = 0; i < fat->info().sectors_per_cluster; i++) {
        status_t err = bcache_read_block(fat->bcache(), buf8, sector);
        if (err < 0) {
            return err;
        }

        buf8 += fat->info().bytes_per_sector;
        sector++;
    }

    return NO_ERROR;
}

// zero a cluster, using the bcache
ssize_t fat_zero_cluster(fat_fs *fat, uint32_t cluster) {
    DEBUG_ASSERT(fat->lock.is_held());

    LTRACEF("cluster %u\n", cluster);

    auto sector = fat_sector_for_cluster(fat, cluster);

    for (size_t i = 0; i < fat->info().sectors_per_cluster; i++) {
        status_t err = bcache_zero_block(fat->bcache(), sector);
        if (err < 0) {
            return err;
        }

        sector++;
    }

    return NO_ERROR;
}

