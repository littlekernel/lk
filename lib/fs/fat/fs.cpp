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
#include <lk/debug.h>
#include <lk/cpp.h>
#include <malloc.h>
#include <string.h>
#include <endian.h>

#include "fat_priv.h"
#include "fat_fs.h"

#define LOCAL_TRACE 0

__NO_INLINE static void fat_dump(fat_fs_t *fat) {
    printf("bytes_per_sector %u\n", fat->bytes_per_sector);
    printf("sectors_per_cluster %u\n", fat->sectors_per_cluster);
    printf("bytes_per_cluster %u\n", fat->bytes_per_cluster);
    printf("reserved_sectors %u\n", fat->reserved_sectors);
    printf("fat_bits %u\n", fat->fat_bits);
    printf("fat_count %u\n", fat->fat_count);
    printf("sectors_per_fat %u\n", fat->sectors_per_fat);
    printf("total_sectors %u\n", fat->total_sectors);
    printf("active_fat %u\n", fat->active_fat);
    printf("data_start_sector %u\n", fat->data_start_sector);
    printf("total_clusters %u\n", fat->total_clusters);
    printf("root_cluster %u\n", fat->root_cluster);
    printf("root_entries %u\n", fat->root_entries);
    printf("root_start_sector %u\n", fat->root_start_sector);
}

status_t fat_mount(bdev_t *dev, fscookie **cookie) {
    status_t result = NO_ERROR;

    if (!dev)
        return ERR_NOT_VALID;

    uint8_t *bs = (uint8_t *)malloc(512);
    if (!bs) {
        return ERR_NO_MEMORY;
    }

    // free the block on the way out of the function
    auto ac = lk::make_auto_call([&]() { free(bs); });

    ssize_t err = bio_read(dev, bs, 0, 512);
    if (err < 0) {
        return err;
    }

    if (((bs[0x1fe] != 0x55) || (bs[0x1ff] != 0xaa)) && (bs[0x15] == 0xf8)) {
        printf("missing boot signature\n");
        return ERR_NOT_VALID;
    }

    // allocate a structure, all fields implicity zeroed
    auto *fat = new fat_fs_t;
    if (!fat) {
        return ERR_NO_MEMORY;
    }
    fat->dev = dev;

    // if we early terminate, free the fat structure
    auto ac2 = lk::make_auto_call([&]() { delete(fat); });

    fat->bytes_per_sector = fat_read16(bs,0xb);
    if ((fat->bytes_per_sector != 0x200) && (fat->bytes_per_sector != 0x400) && (fat->bytes_per_sector != 0x800)) {
        printf("unsupported sector size (%x)\n", fat->bytes_per_sector);
        return ERR_NOT_VALID;
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
            return ERR_NOT_VALID;
    }

    fat->reserved_sectors = fat_read16(bs, 0xe);
    fat->fat_count = bs[0x10];

    if ((fat->fat_count == 0) || (fat->fat_count > 8)) {
        printf("unreasonable FAT count (%x)\n", fat->fat_count);
        return ERR_NOT_VALID;
    }

    if (bs[0x15] != 0xf8) {
        printf("unsupported media descriptor byte (%x)\n", bs[0x15]);
        return ERR_NOT_VALID;
    }

    // determine the FAT type according to logic in the FAT specification

    // read the number of root entries
    fat->root_entries = fat_read16(bs, 0x11); // number of root entries, shall be 0 for fat32
    if (fat->root_entries % (fat->bytes_per_sector / 0x20)) {
        printf("illegal number of root entries (%x)\n", fat->root_entries);
        return ERR_NOT_VALID;
    }
    auto root_dir_sectors = ((fat->root_entries * 32) + (fat->bytes_per_sector - 1)) / fat->bytes_per_sector;

    // sectors per fat
    fat->sectors_per_fat = fat_read16(bs, 0x16); // read FAT size 16 bit
    if (fat->sectors_per_fat == 0) {
        fat->sectors_per_fat = fat_read32(bs,0x24); // read FAT size 32 bit
        if (fat->sectors_per_fat == 0) {
            printf("invalid sectors per fat 0\n");
            return ERR_NOT_VALID;
        }
    }

    // total sectors
    fat->total_sectors = fat_read16(bs,0x13); // total sectors 16
    if (fat->total_sectors == 0) {
        fat->total_sectors = fat_read32(bs,0x20); // total sectors 32
    }
    if (fat->total_sectors == 0) {
        // TODO: test that total sectors <= bio device size
        printf("invalid total sector count 0\n");
        return ERR_NOT_VALID;
    }

    // first data sector is just after the root dir (in fat12/16) which is just after the FAT(s), which is just
    // after the reserved sectors
    fat->data_start_sector = fat->reserved_sectors + (fat->fat_count * fat->sectors_per_fat) + root_dir_sectors;
    auto data_sectors = fat->total_sectors - fat->data_start_sector;
    LTRACEF("data sectors %u\n", data_sectors);

    fat->total_clusters = data_sectors / fat->sectors_per_cluster;
    LTRACEF("total clusters %u\n", fat->total_clusters);

    // table according to FAT spec
    if (fat->total_clusters < 4085) {
        fat->fat_bits = 12;
    } else if (fat->total_clusters < 65525) {
        fat->fat_bits = 16;
    } else {
        fat->fat_bits = 32;
    }

    if (fat->fat_bits == 32) {
        // FAT32 root entries should be zero because it's cluster based
        if (fat->root_entries != 0) {
            printf("nonzero root entries (%u) in a FAT32 volume. invalid\n", fat->root_entries);
            return ERR_NOT_VALID;
        }

        // In FAT32, root directory acts much like a file and occupies a cluster chain starting generally
        // at cluster 2.
        fat->root_cluster = fat_read32(bs, 0x2c);
        if (fat->root_cluster >= fat->total_clusters) {
            printf("root cluster too large (%x > %x)\n", fat->root_cluster, fat->total_clusters);
            return ERR_NOT_VALID;
        }
        if (fat->root_cluster < 2) {
            printf("root cluster too small (<2) %u\n", fat->root_cluster);
            return ERR_NOT_VALID;
        }

        // read the active fat
        fat->active_fat = (bs[0x28] & 0x80) ? 0 : (bs[0x28] & 0xf);

        // TODO: read in fsinfo structure
    } else {
        // On a FAT 12 or FAT 16 volumes the root directory is at a fixed position immediately after the File Allocation Tables
        fat->root_start_sector = fat->reserved_sectors + fat->fat_count * fat->sectors_per_fat;
    }

    fat->bytes_per_cluster = fat->sectors_per_cluster * fat->bytes_per_sector;
    fat->cache = bcache_create(fat->dev, fat->bytes_per_sector, 16);

    // we're okay, cancel our cleanup of the fat structure
    ac2.cancel();

    printf("Mounted FAT volume, some information:\n");
    fat_dump(fat);

    *cookie = (fscookie *)fat;

    return result;
}

status_t fat_unmount(fscookie *cookie) {
    auto *fat = (fat_fs_t *)cookie;

    // TODO: handle unmounting when files/dirs are active
    DEBUG_ASSERT(list_is_empty(&fat->dir_list));

    bcache_destroy(fat->cache);

    delete fat;

    return NO_ERROR;
}

static const struct fs_api fat_api = {
    .format = nullptr,
    .fs_stat = nullptr,

    .mount = fat_mount,
    .unmount = fat_unmount,
    .open = fat_open_file,
    .create = nullptr,
    .remove = nullptr,
    .truncate = nullptr,
    .stat = fat_stat_file,
    .read = fat_read_file,
    .write = nullptr,
    .close = fat_close_file,

    .mkdir = nullptr,
    .opendir = fat_opendir,
    .readdir = fat_readdir,
    .closedir = fat_closedir,

    .file_ioctl = nullptr,
};

STATIC_FS_IMPL(fat, &fat_api);
