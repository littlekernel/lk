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
#include "file.h"
#include "dir.h"

#define LOCAL_TRACE FAT_GLOBAL_TRACE(0)

__NO_INLINE static void fat_dump(fat_fs *fat) {
    const auto info = fat->info();
    printf("bytes_per_sector %u\n", info.bytes_per_sector);
    printf("sectors_per_cluster %u\n", info.sectors_per_cluster);
    printf("bytes_per_cluster %u\n", info.bytes_per_cluster);
    printf("reserved_sectors %u\n", info.reserved_sectors);
    printf("fat_bits %u\n", info.fat_bits);
    printf("fat_count %u\n", info.fat_count);
    printf("sectors_per_fat %u\n", info.sectors_per_fat);
    printf("total_sectors %u\n", info.total_sectors);
    printf("active_fat %u\n", info.active_fat);
    printf("data_start_sector %u\n", info.data_start_sector);
    printf("total_clusters %u\n", info.total_clusters);
    printf("root_cluster %u\n", info.root_cluster);
    printf("root_entries %u\n", info.root_entries);
    printf("root_start_sector %u\n", info.root_start_sector);
    printf("root_dir_sectors %u\n", info.root_dir_sectors);
}

fat_fs::fat_fs() = default;
fat_fs::~fat_fs() = default;

status_t fat_fs::mount(bdev_t *dev, fscookie **cookie) {
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
    auto *fat = new fat_fs;
    if (!fat) {
        return ERR_NO_MEMORY;
    }
    fat->dev_ = dev;

    // if we early terminate, free the fat structure
    auto ac2 = lk::make_auto_call([&]() { delete(fat); });

    auto *info = &fat->info_;

    info->bytes_per_sector = fat_read16(bs,0xb);
    if ((info->bytes_per_sector != 0x200) && (info->bytes_per_sector != 0x400) && (info->bytes_per_sector != 0x800)) {
        printf("unsupported sector size (%x)\n", info->bytes_per_sector);
        return ERR_NOT_VALID;
    }

    info->sectors_per_cluster = bs[0xd];
    switch (info->sectors_per_cluster) {
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
            printf("unsupported sectors/cluster (%x)\n", info->sectors_per_cluster);
            return ERR_NOT_VALID;
    }

    info->reserved_sectors = fat_read16(bs, 0xe);
    info->fat_count = bs[0x10];

    if ((info->fat_count == 0) || (info->fat_count > 8)) {
        printf("unreasonable FAT count (%x)\n", info->fat_count);
        return ERR_NOT_VALID;
    }

    if (bs[0x15] != 0xf8) {
        printf("unsupported media descriptor byte (%x)\n", bs[0x15]);
        return ERR_NOT_VALID;
    }

    // determine the FAT type according to logic in the FAT specification

    // read the number of root entries
    info->root_entries = fat_read16(bs, 0x11); // number of root entries, shall be 0 for fat32
    if (info->root_entries % (info->bytes_per_sector / 0x20)) {
        printf("illegal number of root entries (%x)\n", info->root_entries);
        return ERR_NOT_VALID;
    }
    info->root_dir_sectors = ((info->root_entries * 32) + (info->bytes_per_sector - 1)) / info->bytes_per_sector;

    // sectors per fat
    info->sectors_per_fat = fat_read16(bs, 0x16); // read FAT size 16 bit
    if (info->sectors_per_fat == 0) {
        info->sectors_per_fat = fat_read32(bs,0x24); // read FAT size 32 bit
        if (info->sectors_per_fat == 0) {
            printf("invalid sectors per fat 0\n");
            return ERR_NOT_VALID;
        }
    }

    // total sectors
    info->total_sectors = fat_read16(bs,0x13); // total sectors 16
    if (info->total_sectors == 0) {
        info->total_sectors = fat_read32(bs,0x20); // total sectors 32
    }
    if (info->total_sectors == 0) {
        // TODO: test that total sectors <= bio device size
        printf("invalid total sector count 0\n");
        return ERR_NOT_VALID;
    }

    // first data sector is just after the root dir (in fat12/16) which is just after the FAT(s), which is just
    // after the reserved sectors
    info->data_start_sector = info->reserved_sectors + (info->fat_count * info->sectors_per_fat) + info->root_dir_sectors;
    auto data_sectors = info->total_sectors - info->data_start_sector;
    LTRACEF("data sectors %u\n", data_sectors);

    info->total_clusters = data_sectors / info->sectors_per_cluster;
    LTRACEF("total clusters %u\n", info->total_clusters);

    // table according to FAT spec
    if (info->total_clusters < 4085) {
        info->fat_bits = 12;
    } else if (info->total_clusters < 65525) {
        info->fat_bits = 16;
    } else {
        info->fat_bits = 32;
    }

    if (info->fat_bits == 32) {
        // FAT32 root entries should be zero because it's cluster based
        if (info->root_entries != 0) {
            printf("nonzero root entries (%u) in a FAT32 volume. invalid\n", info->root_entries);
            return ERR_NOT_VALID;
        }

        // In FAT32, root directory acts much like a file and occupies a cluster chain starting generally
        // at cluster 2.
        info->root_cluster = fat_read32(bs, 0x2c);
        if (info->root_cluster >= info->total_clusters) {
            printf("root cluster too large (%x > %x)\n", info->root_cluster, info->total_clusters);
            return ERR_NOT_VALID;
        }
        if (info->root_cluster < 2) {
            printf("root cluster too small (<2) %u\n", info->root_cluster);
            return ERR_NOT_VALID;
        }

        // read the active fat
        info->active_fat = (bs[0x28] & 0x80) ? 0 : (bs[0x28] & 0xf);

        // TODO: read in fsinfo structure
    } else {
        // On a FAT 12 or FAT 16 volumes the root directory is at a fixed position immediately after the File Allocation Tables
        info->root_start_sector = info->reserved_sectors + info->fat_count * info->sectors_per_fat;
    }

    info->bytes_per_cluster = info->sectors_per_cluster * info->bytes_per_sector;
    fat->bcache_ = bcache_create(fat->dev(), info->bytes_per_sector, 16);

    // we're okay, cancel our cleanup of the fat structure
    ac2.cancel();

#if LOCAL_TRACE
    printf("Mounted FAT volume, some information:\n");
    fat_dump(fat);
#endif

    *cookie = (fscookie *)fat;

    return result;
}

status_t fat_fs::unmount(fscookie *cookie) {
    auto *fat = (fat_fs *)cookie;

    {
        AutoLock guard(fat->lock);

        // TODO: handle unmounting when files/dirs are active
        DEBUG_ASSERT(list_is_empty(&fat->file_list_));

        bcache_destroy(fat->bcache());
    }

    delete fat;

    return NO_ERROR;
}

void fat_fs::add_to_file_list(fat_file *file) {
    DEBUG_ASSERT(lock.is_held());
    DEBUG_ASSERT(!list_in_list(&file->node_));

    LTRACEF("file %p, location %u:%u\n", file, file->dir_loc().starting_dir_cluster, file->dir_loc().dir_offset);

    list_add_head(&file_list_, &file->node_);
}

fat_file *fat_fs::lookup_file(const dir_entry_location &loc) {
    DEBUG_ASSERT(lock.is_held());

    fat_file *f;
    list_for_every_entry(&file_list_, f, fat_file, node_) {
        if (loc == f->dir_loc()) {
            return f;
        }
    }

    return nullptr;
}

static const struct fs_api fat_api = {
    .format = nullptr,
    .fs_stat = nullptr,

    .mount = fat_fs::mount,
    .unmount = fat_fs::unmount,
    .open = fat_file::open_file,
    .create = nullptr,
    .remove = nullptr,
    .truncate = nullptr,
    .stat = fat_file::stat_file,
    .read = fat_file::read_file,
    .write = nullptr,
    .close = fat_file::close_file,

    .mkdir = nullptr,
    .opendir = fat_dir::opendir,
    .readdir = fat_dir::readdir,
    .closedir = fat_dir::closedir,

    .file_ioctl = nullptr,
};

STATIC_FS_IMPL(fat, &fat_api);
