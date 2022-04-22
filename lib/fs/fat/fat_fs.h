/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/bio.h>
#include <lib/bcache.h>

typedef struct {
    bdev_t *dev;
    bcache_t cache;

    uint32_t lba_start;

    // data computed from BPB
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t bytes_per_cluster;
    uint32_t reserved_sectors;
    uint32_t fat_bits;
    uint32_t fat_count;
    uint32_t sectors_per_fat;
    uint32_t total_sectors;
    uint32_t active_fat;
    uint32_t data_start;
    uint32_t total_clusters;
    uint32_t root_cluster;
    uint32_t root_entries;
    uint32_t root_start;
} fat_fs_t;

enum class fat_attribute : uint8_t {
    read_only = 0x01,
    hidden = 0x02,
    system = 0x04,
    volume_id = 0x08,
    directory = 0x10,
    archive = 0x20,
    lfn = read_only | hidden | system | volume_id,
};

typedef struct {
    fat_fs_t *fat_fs;

    uint32_t start_cluster;
    uint32_t length;

    fat_attribute attributes;
} fat_file_t;

inline uint32_t fat_read32(const void *_buffer, size_t offset) {
    auto *buffer = (const uint8_t *)_buffer;

    return buffer[offset] +
          (buffer[offset + 1] << 8) +
          (buffer[offset + 2] << 16) +
          (buffer[offset + 3] << 24);
}

inline uint16_t fat_read16(const void *_buffer, size_t offset) {
    auto *buffer = (const uint8_t *)_buffer;

    return buffer[offset] +
          (buffer[offset + 1] << 8);
}
