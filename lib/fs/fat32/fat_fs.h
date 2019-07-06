/*
 * Copyright (c) 2015 Steve White
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

typedef struct {
    fat_fs_t *fat_fs;
    uint32_t start_cluster;
    uint32_t length;
    uint8_t attributes;
} fat_file_t;

typedef enum {
    fat_attribute_read_only = 0x01,
    fat_attribute_hidden = 0x02,
    fat_attribute_system = 0x04,
    fat_attribute_volume_id = 0x08,
    fat_attribute_directory = 0x10,
    fat_attribute_archive = 0x20,
    fat_attribute_lfn = fat_attribute_read_only | fat_attribute_hidden | fat_attribute_system | fat_attribute_volume_id,
} fat_attributes;

#define fat_read32(buffer,off) \
(((uint8_t *)buffer)[(off)] + (((uint8_t *)buffer)[(off)+1] << 8) + \
(((uint8_t *)buffer)[(off)+2] << 16) + (((uint8_t *)buffer)[(off)+3] << 24))

#define fat_read16(buffer,off) \
(((uint8_t *)buffer)[(off)] + (((uint8_t *)buffer)[(off)+1] << 8))

