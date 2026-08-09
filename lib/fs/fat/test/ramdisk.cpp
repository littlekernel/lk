/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "ramdisk.h"

#include <arch/defines.h>
#include <lk/err.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

namespace fat_test {

// The geometry sweep. The point of the boundary rows is that FAT12 and FAT16
// are chosen by cluster count alone, so a volume one cluster either side of a
// threshold exercises a completely different set of FAT accessors at nearly
// identical size. FAT32's minimum is 65525 clusters, which at 512 byte clusters
// is roughly 34MB -- kept at the minimum so it has a chance of fitting on small
// targets, and skipped gracefully where it does not.
// The two "max" rows and the FAT32 row are inherently large: the cluster count
// is what selects the width, and the smallest volume with that many clusters
// uses one sector per cluster, so FAT16's upper boundary and FAT32's lower one
// both land around 34MB. Those are the rows that skip on a small target.
const geometry kGeometries[] = {
    // name                fat  sector  spc  target clusters   approx size
    {"fat12-min",           12,    512,   1,             64},  //    64KB
    {"fat12-max",           12,    512,   1,           4084},  //     2MB, last FAT12
    {"fat12-sector-span",   12,    512,   1,           2000},  //     1MB, straddling entries
    {"fat12-4k-cluster",    12,    512,   8,           1000},  //     4MB
    {"fat16-min",           16,    512,   1,           4085},  //     2MB, first FAT16
    {"fat16-1k-sector",     16,   1024,   2,           5000},  //    10MB
    {"fat16-2k-sector",     16,   2048,   1,           5000},  //    10MB
    {"fat16-max",           16,    512,   1,          65524},  //    34MB, last FAT16
    {"fat32-min",           32,    512,   1,          65525},  //    34MB, first FAT32
};
const size_t kGeometryCount = countof(kGeometries);

namespace {

uint64_t fat_bytes_for_entries(uint64_t entries, uint32_t fat_bits) {
    switch (fat_bits) {
        case 12:
            return (entries * 3 + 1) / 2;
        case 16:
            return entries * 2;
        default:
            return entries * 4;
    }
}

// Mirror of the fixed point solver in format.cpp: how many data clusters does a
// volume of this many sectors end up with?
uint32_t clusters_for_sectors(const geometry &g, uint64_t total_sectors) {
    const uint64_t reserved = (g.fat_bits == 32) ? 32 : 1;
    const uint64_t root_sectors =
        (g.fat_bits == 32) ? 0 : ((512 * 32 + g.bytes_per_sector - 1) / g.bytes_per_sector);

    uint64_t sectors_per_fat = 1;
    for (int i = 0; i < 16; i++) {
        const uint64_t overhead = reserved + 2 * sectors_per_fat + root_sectors;
        if (overhead >= total_sectors) {
            return 0;
        }
        const uint64_t clusters = (total_sectors - overhead) / g.sectors_per_cluster;
        const uint64_t needed = (fat_bytes_for_entries(clusters + 2, g.fat_bits) +
                                 g.bytes_per_sector - 1) / g.bytes_per_sector;
        if (needed <= sectors_per_fat) {
            return (uint32_t)clusters;
        }
        sectors_per_fat = needed;
    }
    return 0;
}

} // anonymous namespace

size_t volume_size_for(const geometry &g) {
    // The boundary rows only mean something if they land on the exact cluster
    // count they name, so solve for the volume size rather than estimating it.
    // A bigger FAT eats data sectors, so a naive estimate always undershoots.
    const uint64_t reserved = (g.fat_bits == 32) ? 32 : 1;
    const uint64_t root_sectors =
        (g.fat_bits == 32) ? 0 : ((512 * 32 + g.bytes_per_sector - 1) / g.bytes_per_sector);
    const uint64_t fat_sectors =
        (fat_bytes_for_entries((uint64_t)g.target_clusters + 2, g.fat_bits) +
         g.bytes_per_sector - 1) / g.bytes_per_sector;

    uint64_t total_sectors = reserved + 2 * fat_sectors + root_sectors +
                             (uint64_t)g.target_clusters * g.sectors_per_cluster;

    // Nudge the size until the layout yields exactly the target. Each extra
    // cluster's worth of sectors adds one cluster, except where it pushes the
    // FAT over a sector boundary, so this converges in a handful of rounds.
    for (int i = 0; i < 32; i++) {
        const uint32_t got = clusters_for_sectors(g, total_sectors);
        if (got == g.target_clusters) {
            break;
        }
        if (got < g.target_clusters) {
            total_sectors += (uint64_t)(g.target_clusters - got) * g.sectors_per_cluster;
        } else {
            total_sectors -= (uint64_t)(got - g.target_clusters) * g.sectors_per_cluster;
        }
    }

    return (size_t)(total_sectors * g.bytes_per_sector);
}

fat_format_args_t format_args_for(const geometry &g) {
    fat_format_args_t args = {};
    args.fat_bits = g.fat_bits;
    args.bytes_per_sector = g.bytes_per_sector;
    args.sectors_per_cluster = g.sectors_per_cluster;
    args.volume_label = g.name;
    return args;
}

ram_volume::~ram_volume() {
    destroy();
}

void ram_volume::destroy() {
    if (mounted_) {
        fs_unmount(mount_path_);
        mounted_ = false;
    }
    if (dev_) {
        // drop our reference, then remove the device from the global list
        bio_close(dev_);
        bio_unregister_device(dev_);
        dev_ = nullptr;
    }
    if (mem_) {
        free(mem_);
        mem_ = nullptr;
    }
    size_ = 0;
}

status_t ram_volume::create(const char *name, const char *mount_path, size_t size,
                            const fat_format_args_t &args) {
    destroy();

    strlcpy(device_name_, name, sizeof(device_name_));
    strlcpy(mount_path_, mount_path, sizeof(mount_path_));

    mem_ = memalign(CACHE_LINE, size);
    if (!mem_) {
        printf("ram_volume: cannot allocate %zu bytes for '%s'\n", size, name);
        return ERR_NO_MEMORY;
    }
    size_ = size;

    // create_membdev keeps a pointer to the buffer; it does not copy it
    int err = create_membdev(device_name_, mem_, size_);
    if (err < 0) {
        destroy();
        return err;
    }

    // hold a reference so the device stays alive for the volume's lifetime and
    // so destroy() has a handle to unregister
    dev_ = bio_open(device_name_);
    if (!dev_) {
        destroy();
        return ERR_NOT_FOUND;
    }

    status_t st = fs_format_device("fat", device_name_, &args);
    if (st < 0) {
        printf("ram_volume: format of '%s' failed: %d\n", name, st);
        destroy();
        return st;
    }

    st = fs_mount(mount_path_, "fat", device_name_, FS_MOUNT_OPTION_NONE);
    if (st < 0) {
        printf("ram_volume: mount of '%s' failed: %d\n", name, st);
        destroy();
        return st;
    }
    mounted_ = true;

    return NO_ERROR;
}

status_t ram_volume::remount() {
    if (!mounted_) {
        return ERR_BAD_STATE;
    }

    status_t st = fs_unmount(mount_path_);
    if (st < 0) {
        return st;
    }
    mounted_ = false;

    st = fs_mount(mount_path_, "fat", device_name_, FS_MOUNT_OPTION_NONE);
    if (st < 0) {
        return st;
    }
    mounted_ = true;

    return NO_ERROR;
}

} // namespace fat_test
