/*
 * Copyright (c) 2015 Steve White
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <lib/bio.h>
#include <lib/fs.h>
#include <lk/trace.h>
#include <lk/debug.h>
#include <malloc.h>
#include <string.h>
#include <endian.h>

#include "fat32_priv.h"
#include "fat_fs.h"

static void fat32_dump(fat_fs_t *fat) {
    printf("bytes_per_sector %u\n", fat->bytes_per_sector);
    printf("sectors_per_cluster %u\n", fat->sectors_per_cluster);
    printf("bytes_per_cluster %u\n", fat->bytes_per_cluster);
    printf("reserved_sectors %u\n", fat->reserved_sectors);
    printf("fat_bits %u\n", fat->fat_bits);
    printf("fat_count %u\n", fat->fat_count);
    printf("sectors_per_fat %u\n", fat->sectors_per_fat);
    printf("total_sectors %u\n", fat->total_sectors);
    printf("active_fat %u\n", fat->active_fat);
    printf("data_start %u\n", fat->data_start);
    printf("total_clusters %u\n", fat->total_clusters);
    printf("root_cluster %u\n", fat->root_cluster);
    printf("root_entries %u\n", fat->root_entries);
    printf("root_start %u\n", fat->root_start);
}

status_t fat32_mount(bdev_t *dev, fscookie **cookie) {
    status_t result = NO_ERROR;

    if (!dev)
        return ERR_NOT_VALID;

    uint8_t *bs = (uint8_t *)malloc(512);
    int err = bio_read(dev, bs, 0, 512);
    if (err < 0) {
        result = ERR_GENERIC;
        goto end;
    }

    if (((bs[0x1fe] != 0x55) || (bs[0x1ff] != 0xaa)) && (bs[0x15] == 0xf8)) {
        printf("missing boot signature\n");
        result = ERR_NOT_VALID;
        goto end;
    }

    fat_fs_t *fat;
    fat = (fat_fs_t *)malloc(sizeof(fat_fs_t));
    fat->lba_start = 0;
    fat->dev = dev;

    fat->bytes_per_sector = fat_read16(bs,0xb);
    if ((fat->bytes_per_sector != 0x200) && (fat->bytes_per_sector != 0x400) && (fat->bytes_per_sector != 0x800)) {
        printf("unsupported sector size (%x)\n", fat->bytes_per_sector);
        result = ERR_NOT_VALID;
        goto end;
    }

    fat->sectors_per_cluster = bs[0xd];
    switch (fat->sectors_per_cluster) {
        case 1:
        case 2:
        case 4:
        case 8:
        case 0x10:
        case 0x20:
        case 0x40:
        case 0x80:
            break;
        default:
            printf("unsupported sectors/cluster (%x)\n", fat->sectors_per_cluster);
            result = ERR_NOT_VALID;
            goto end;
    }

    fat->reserved_sectors = fat_read16(bs, 0xe);
    fat->fat_count = bs[0x10];

    if ((fat->fat_count == 0) || (fat->fat_count > 8)) {
        printf("unreasonable FAT count (%x)\n", fat->fat_count);
        result = ERR_NOT_VALID;
        goto end;
    }

    if (bs[0x15] != 0xf8) {
        printf("unsupported media descriptor byte (%x)\n", bs[0x15]);
        result = ERR_NOT_VALID;
        goto end;
    }

    fat->sectors_per_fat = fat_read16(bs, 0x16);
    if (fat->sectors_per_fat == 0) {
        fat->fat_bits = 32;
        fat->sectors_per_fat = fat_read32(bs,0x24);
        fat->total_sectors = fat_read32(bs,0x20);
        if (fat->total_sectors == 0) {
            // FAT32 is supposed to always use the 32bit version, but if it's zero fall back
            // to the 16 bit version
            fat->total_sectors = fat_read16(bs,0x13);
        }
        fat->active_fat = (bs[0x28] & 0x80) ? 0 : (bs[0x28] & 0xf);
        fat->data_start = fat->reserved_sectors + (fat->fat_count * fat->sectors_per_fat);
        fat->total_clusters = (fat->total_sectors - fat->data_start) / fat->sectors_per_cluster;

        // In FAT32, root directory appears in data area on given cluster and can be a cluster chain.
        fat->root_cluster = fat_read32(bs,0x2c);
        fat->root_start = 0;
        if (fat->root_cluster >= fat->total_clusters) {
            printf("root cluster too large (%x > %x)\n", fat->root_cluster, fat->total_clusters);
            result = ERR_NOT_VALID;
            goto end;
        }
        fat->root_entries = 0;
    } else {
        if (fat->fat_count != 2) {
            printf("illegal FAT count (%x)\n", fat->fat_count);
            result = ERR_NOT_VALID;
            goto end;
        }

        // On a FAT 12 or FAT 16 volumes the root directory is at a fixed position immediately after the File Allocation Tables
        fat->root_cluster = 0;
        fat->root_entries = fat_read16(bs,0x11);
        if (fat->root_entries % (fat->bytes_per_sector / 0x20)) {
            printf("illegal number of root entries (%x)\n", fat->root_entries);
            result = ERR_NOT_VALID;
            goto end;
        }

        fat->total_sectors = fat_read16(bs,0x13);
        if (fat->total_sectors == 0) {
            fat->total_sectors = fat_read32(bs,0x20);
        }

        fat->root_start = fat->reserved_sectors + fat->fat_count * fat->sectors_per_fat;
        fat->data_start = fat->root_start + fat->root_entries * 0x20 / fat->bytes_per_sector;
        fat->total_clusters = (fat->total_sectors - fat->data_start) / fat->sectors_per_cluster;

        if (fat->total_clusters < 0xff2) {
            printf("small FAT12, not supported\n");
            result = ERR_NOT_VALID;
            goto end;
        }
        fat->fat_bits = 16;
    }

    fat->bytes_per_cluster = fat->sectors_per_cluster * fat->bytes_per_sector;
    fat->cache = bcache_create(fat->dev, fat->bytes_per_sector, 16);

    printf("Mounted FAT volume, some information:\n");
    fat32_dump(fat);

    *cookie = (fscookie *)fat;
end:
    free(bs);
    return result;
}

status_t fat32_unmount(fscookie *cookie) {
    fat_fs_t *fat = (fat_fs_t *)cookie;
    bcache_destroy(fat->cache);
    free(fat);
    return NO_ERROR;
}

static const struct fs_api fat32_api = {
    .format = nullptr,
    .fs_stat = nullptr,

    .mount = fat32_mount,
    .unmount = fat32_unmount,
    .open = fat32_open_file,
    .create = nullptr,
    .remove = nullptr,
    .truncate = nullptr,
    .stat = fat32_stat_file,
    .read = fat32_read_file,
    .write = nullptr,
    .close = fat32_close_file,

    .mkdir = nullptr,
    .opendir = nullptr,
    .readdir = nullptr,
    .closedir = nullptr,

    .file_ioctl = nullptr,
};

STATIC_FS_IMPL(fat32, &fat32_api);
