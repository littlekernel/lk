/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lib/bio.h>
#include <lib/fs.h>
#include <lib/fs/fat.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <memory>
#include <new>
#include <stdlib.h>
#include <string.h>

#include "fat_fs.h"
#include "fat_priv.h"

#define LOCAL_TRACE FAT_GLOBAL_TRACE(0)

namespace {

constexpr uint8_t kMediaDescriptor = 0xf8;
constexpr uint32_t kDefaultRootEntries = 512;
constexpr uint32_t kDefaultFatCount = 2;
constexpr uint32_t kFat32ReservedSectors = 32;
constexpr uint32_t kFat32FsInfoSector = 1;
constexpr uint32_t kFat32BackupBootSector = 6;
constexpr uint32_t kFat32RootCluster = 2;
constexpr uint32_t kDefaultVolumeId = 0x4c4b4641; // "LKFA"

constexpr uint32_t kFsInfoLeadSig = 0x41615252;
constexpr uint32_t kFsInfoStructSig = 0x61417272;
constexpr uint32_t kFsInfoTrailSig = 0xaa550000;

// Everything the layout solver produces. Mirrors the subset of fat_info that
// the on-disk BPB actually encodes.
struct layout {
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t total_sectors;
    uint32_t reserved_sectors;
    uint32_t fat_count;
    uint32_t sectors_per_fat;
    uint32_t root_entries;
    uint32_t root_dir_sectors;
    uint32_t fat_bits;
    uint32_t data_start_sector;
    uint32_t data_cluster_count;
};

// Bytes needed to hold `entries` FAT entries at the given width. Entry counts
// include the two reserved entries.
uint64_t fat_bytes_for_entries(uint64_t entries, uint32_t fat_bits) {
    switch (fat_bits) {
        case 12:
            return (entries * 3 + 1) / 2; // 1.5 bytes each, rounded up
        case 16:
            return entries * 2;
        default:
            return entries * 4;
    }
}

// Select the FAT width from a data cluster count, using the FAT specification's
// thresholds. Must agree with the same decision in fat_fs::mount().
uint32_t fat_bits_for_cluster_count(uint32_t data_cluster_count) {
    if (data_cluster_count < 4085) {
        return 12;
    } else if (data_cluster_count < 65525) {
        return 16;
    }
    return 32;
}

bool is_valid_sectors_per_cluster(uint32_t spc) {
    return spc >= 1 && spc <= 128 && (spc & (spc - 1)) == 0;
}

// Solve the circular dependency between sectors_per_fat and the cluster count:
// a bigger FAT leaves fewer data sectors, which needs a smaller FAT. Start from
// a one sector FAT and grow until it is large enough for the cluster count it
// itself produces. This converges in a couple of rounds and only ever grows, so
// the result is always big enough (it may be a sector larger than strictly
// necessary, which is harmless).
status_t solve_layout(layout *out, uint32_t fat_bits) {
    out->fat_bits = fat_bits;
    out->root_dir_sectors =
        (fat_bits == 32)
            ? 0
            : ((out->root_entries * 32 + out->bytes_per_sector - 1) / out->bytes_per_sector);

    uint32_t sectors_per_fat = 1;
    for (int i = 0; i < 16; i++) {
        const uint64_t overhead = (uint64_t)out->reserved_sectors +
                                  (uint64_t)out->fat_count * sectors_per_fat +
                                  out->root_dir_sectors;
        if (overhead >= out->total_sectors) {
            return ERR_TOO_BIG;
        }

        const uint32_t data_sectors = out->total_sectors - (uint32_t)overhead;
        const uint32_t clusters = data_sectors / out->sectors_per_cluster;

        const uint64_t needed_bytes = fat_bytes_for_entries((uint64_t)clusters + 2, fat_bits);
        const uint32_t needed_sectors =
            (uint32_t)((needed_bytes + out->bytes_per_sector - 1) / out->bytes_per_sector);

        if (needed_sectors <= sectors_per_fat) {
            // Stable: this FAT holds every cluster the layout produces.
            out->sectors_per_fat = sectors_per_fat;
            out->data_start_sector = (uint32_t)overhead;
            out->data_cluster_count = clusters;
            return NO_ERROR;
        }
        sectors_per_fat = needed_sectors;
    }

    return ERR_NOT_VALID;
}

// Re-derive the cluster count and FAT width the exact way fat_fs::mount() does,
// and confirm the layout mounts as the width it was built for. This is the
// check that keeps the formatter and the mount path from silently disagreeing.
status_t verify_layout_mounts_as(const layout &l, uint32_t expected_fat_bits) {
    const uint32_t data_sectors = l.total_sectors - l.data_start_sector;
    const uint32_t total_clusters = data_sectors / l.sectors_per_cluster + 2;
    const uint32_t data_cluster_count = total_clusters - 2;

    if (data_cluster_count != l.data_cluster_count) {
        printf("fat_format: cluster count mismatch: layout %u, mount would see %u\n",
               l.data_cluster_count, data_cluster_count);
        return ERR_NOT_VALID;
    }

    const uint32_t selected = fat_bits_for_cluster_count(data_cluster_count);
    if (selected != expected_fat_bits) {
        printf("fat_format: %u data clusters would mount as FAT%u, not FAT%u\n",
               data_cluster_count, selected, expected_fat_bits);
        return ERR_INVALID_ARGS;
    }

    // The FAT must be able to address every cluster it claims to have.
    const uint64_t fat_capacity = (uint64_t)l.sectors_per_fat * l.bytes_per_sector;
    const uint64_t fat_needed = fat_bytes_for_entries((uint64_t)data_cluster_count + 2,
                                                      expected_fat_bits);
    if (fat_capacity < fat_needed) {
        printf("fat_format: FAT of %llu bytes cannot hold %u clusters (needs %llu)\n",
               fat_capacity, data_cluster_count, fat_needed);
        return ERR_NOT_VALID;
    }

    return NO_ERROR;
}

// A reasonable cluster size for a volume, mirroring the usual mkfs heuristics:
// keep the cluster count inside the chosen width without making clusters huge.
uint32_t default_sectors_per_cluster(uint32_t total_sectors, uint32_t bytes_per_sector) {
    const uint64_t total_bytes = (uint64_t)total_sectors * bytes_per_sector;
    uint32_t target_cluster_bytes;
    if (total_bytes <= 64ULL * 1024 * 1024) {
        target_cluster_bytes = 512;
    } else if (total_bytes <= 256ULL * 1024 * 1024) {
        target_cluster_bytes = 4096;
    } else if (total_bytes <= 8ULL * 1024 * 1024 * 1024) {
        target_cluster_bytes = 8192;
    } else {
        target_cluster_bytes = 32768;
    }

    uint32_t spc = target_cluster_bytes / bytes_per_sector;
    if (spc < 1) {
        spc = 1;
    }
    if (spc > 128) {
        spc = 128;
    }
    return spc;
}

void write_volume_label(uint8_t *buf, size_t offset, const char *label) {
    memset(buf + offset, ' ', 11);
    if (!label) {
        label = "NO NAME";
    }
    for (size_t i = 0; i < 11 && label[i]; i++) {
        buf[offset + i] = (uint8_t)label[i];
    }
}

void build_boot_sector(uint8_t *bs, const layout &l, const fat_format_args_t &args) {
    memset(bs, 0, l.bytes_per_sector);

    // short jump then nop, as every real formatter writes
    bs[0] = 0xeb;
    bs[1] = (l.fat_bits == 32) ? 0x58 : 0x3c;
    bs[2] = 0x90;
    memcpy(bs + 3, "MSWIN4.1", 8);

    fat_write16(bs, 0x0b, (uint16_t)l.bytes_per_sector);
    bs[0x0d] = (uint8_t)l.sectors_per_cluster;
    fat_write16(bs, 0x0e, (uint16_t)l.reserved_sectors);
    bs[0x10] = (uint8_t)l.fat_count;
    fat_write16(bs, 0x11, (uint16_t)l.root_entries);
    // total sectors goes in the 16 bit field when it fits, otherwise the 32 bit one
    if (l.total_sectors <= 0xffff && l.fat_bits != 32) {
        fat_write16(bs, 0x13, (uint16_t)l.total_sectors);
        fat_write32(bs, 0x20, 0);
    } else {
        fat_write16(bs, 0x13, 0);
        fat_write32(bs, 0x20, l.total_sectors);
    }
    bs[0x15] = kMediaDescriptor;
    fat_write16(bs, 0x16, (l.fat_bits == 32) ? 0 : (uint16_t)l.sectors_per_fat);
    fat_write16(bs, 0x18, 32); // sectors per track, geometry is not meaningful here
    fat_write16(bs, 0x1a, 8);  // number of heads, likewise
    fat_write32(bs, 0x1c, 0);  // hidden sectors

    const uint32_t volume_id = args.volume_id ? args.volume_id : kDefaultVolumeId;

    if (l.fat_bits == 32) {
        fat_write32(bs, 0x24, l.sectors_per_fat);
        fat_write16(bs, 0x28, 0); // ext flags: FAT mirroring enabled, active FAT 0
        fat_write16(bs, 0x2a, 0); // filesystem version
        fat_write32(bs, 0x2c, kFat32RootCluster);
        fat_write16(bs, 0x30, kFat32FsInfoSector);
        fat_write16(bs, 0x32, kFat32BackupBootSector);
        bs[0x40] = 0x80; // drive number
        bs[0x42] = 0x29; // extended boot signature
        fat_write32(bs, 0x43, volume_id);
        write_volume_label(bs, 0x47, args.volume_label);
        memcpy(bs + 0x52, "FAT32   ", 8);
    } else {
        bs[0x24] = 0x80;
        bs[0x26] = 0x29;
        fat_write32(bs, 0x27, volume_id);
        write_volume_label(bs, 0x2b, args.volume_label);
        memcpy(bs + 0x36, (l.fat_bits == 12) ? "FAT12   " : "FAT16   ", 8);
    }

    bs[0x1fe] = 0x55;
    bs[0x1ff] = 0xaa;
}

void build_fsinfo_sector(uint8_t *buf, const layout &l) {
    memset(buf, 0, l.bytes_per_sector);
    fat_write32(buf, 0x000, kFsInfoLeadSig);
    fat_write32(buf, 0x1e4, kFsInfoStructSig);
    // The root directory occupies one cluster already.
    fat_write32(buf, 0x1e8, l.data_cluster_count - 1);
    fat_write32(buf, 0x1ec, kFat32RootCluster + 1);
    fat_write32(buf, 0x1fc, kFsInfoTrailSig);
}

// Write `count` zeroed sectors starting at `start_sector`, in chunks so that a
// multi-megabyte FAT does not need a matching allocation.
status_t zero_sectors(bdev_t *dev, const layout &l, uint32_t start_sector, uint32_t count) {
    if (count == 0) {
        return NO_ERROR;
    }

    const uint32_t chunk_sectors = MIN(count, MAX(1u, 65536u / l.bytes_per_sector));
    const size_t chunk_bytes = (size_t)chunk_sectors * l.bytes_per_sector;

    std::unique_ptr<uint8_t[]> buf(new (std::nothrow) uint8_t[chunk_bytes]());
    if (!buf) {
        return ERR_NO_MEMORY;
    }

    uint32_t remaining = count;
    uint32_t sector = start_sector;
    while (remaining > 0) {
        const uint32_t this_count = MIN(remaining, chunk_sectors);
        const size_t this_bytes = (size_t)this_count * l.bytes_per_sector;
        ssize_t err = bio_write(dev, buf.get(), (off_t)sector * l.bytes_per_sector, this_bytes);
        if (err < 0) {
            return (status_t)err;
        }
        if ((size_t)err != this_bytes) {
            return ERR_IO;
        }
        sector += this_count;
        remaining -= this_count;
    }

    return NO_ERROR;
}

status_t write_sector(bdev_t *dev, const layout &l, uint32_t sector, const void *buf) {
    ssize_t err = bio_write(dev, buf, (off_t)sector * l.bytes_per_sector, l.bytes_per_sector);
    if (err < 0) {
        return (status_t)err;
    }
    return ((size_t)err == l.bytes_per_sector) ? NO_ERROR : ERR_IO;
}

// Write the first sector of every FAT copy: the two reserved entries, plus the
// end-of-chain marker for the FAT32 root directory cluster.
status_t write_fat_head_sectors(bdev_t *dev, const layout &l) {
    std::unique_ptr<uint8_t[]> buf_storage(new (std::nothrow) uint8_t[l.bytes_per_sector]());
    if (!buf_storage) {
        return ERR_NO_MEMORY;
    }
    uint8_t *buf = buf_storage.get();

    switch (l.fat_bits) {
        case 12:
            // entry 0 is the media descriptor extended with ones, entry 1 is EOC.
            // Packed 12 bit: 0xff8 then 0xfff -> f8 ff ff
            buf[0] = kMediaDescriptor;
            buf[1] = 0xff;
            buf[2] = 0xff;
            break;
        case 16:
            // entry 1 keeps both the clean-shutdown and no-hard-error bits set
            fat_write16(buf, 0, 0xff00 | kMediaDescriptor);
            fat_write16(buf, 2, 0xffff);
            break;
        default:
            fat_write32(buf, 0, 0x0fffff00 | kMediaDescriptor);
            fat_write32(buf, 4, 0x0fffffff);
            // the root directory is a one cluster chain that ends immediately
            fat_write32(buf, 8, 0x0fffffff);
            break;
    }

    for (uint32_t i = 0; i < l.fat_count; i++) {
        const uint32_t fat_start = l.reserved_sectors + i * l.sectors_per_fat;
        status_t err = write_sector(dev, l, fat_start, buf);
        if (err < 0) {
            return err;
        }
    }

    return NO_ERROR;
}

} // anonymous namespace

// static
status_t fat_fs::format(bdev_t *dev, const void *_args) {
    LTRACEF("dev %p, args %p\n", dev, _args);

    if (!dev) {
        return ERR_INVALID_ARGS;
    }

    const fat_format_args_t default_args = {};
    const fat_format_args_t &args = _args ? *(const fat_format_args_t *)_args : default_args;

    layout l = {};

    // sector size: explicit, else the device's block size
    l.bytes_per_sector = args.bytes_per_sector ? args.bytes_per_sector
                                               : (uint32_t)dev->block_size;
    if (l.bytes_per_sector != 512 && l.bytes_per_sector != 1024 && l.bytes_per_sector != 2048) {
        printf("fat_format: unsupported sector size %u\n", l.bytes_per_sector);
        return ERR_INVALID_ARGS;
    }

    // volume size in sectors: explicit, else everything the device holds
    const uint64_t device_sectors = (uint64_t)dev->total_size / l.bytes_per_sector;
    if (device_sectors == 0) {
        return ERR_TOO_BIG;
    }
    if (args.total_sectors) {
        if (args.total_sectors > device_sectors) {
            printf("fat_format: %u sectors requested but the device holds %llu\n",
                   args.total_sectors, device_sectors);
            return ERR_INVALID_ARGS;
        }
        l.total_sectors = args.total_sectors;
    } else {
        l.total_sectors = (uint32_t)MIN(device_sectors, (uint64_t)UINT32_MAX);
    }

    l.fat_count = args.fat_count ? args.fat_count : kDefaultFatCount;
    if (l.fat_count > 8) {
        printf("fat_format: unreasonable FAT count %u\n", l.fat_count);
        return ERR_INVALID_ARGS;
    }

    l.sectors_per_cluster = args.sectors_per_cluster
                                ? args.sectors_per_cluster
                                : default_sectors_per_cluster(l.total_sectors, l.bytes_per_sector);
    if (!is_valid_sectors_per_cluster(l.sectors_per_cluster)) {
        printf("fat_format: sectors per cluster %u is not a power of two in 1..128\n",
               l.sectors_per_cluster);
        return ERR_INVALID_ARGS;
    }

    if (args.fat_bits && args.fat_bits != 12 && args.fat_bits != 16 && args.fat_bits != 32) {
        printf("fat_format: unsupported FAT width %u\n", args.fat_bits);
        return ERR_INVALID_ARGS;
    }

    // Pick a starting width. Without an explicit request, estimate from the
    // cluster count ignoring FAT overhead; the loop below corrects it.
    uint32_t want_bits = args.fat_bits;
    if (!want_bits) {
        const uint32_t estimate = l.total_sectors / l.sectors_per_cluster;
        want_bits = fat_bits_for_cluster_count(estimate);
    }

    // Solve the layout, and if an auto-selected width turns out not to match what
    // the finished layout mounts as, adopt that and solve again.
    status_t err = ERR_NOT_VALID;
    for (int attempt = 0; attempt < 4; attempt++) {
        // These depend on the width, so reset them every attempt.
        if (want_bits == 32) {
            if (args.root_entries) {
                printf("fat_format: root_entries must be 0 for FAT32\n");
                return ERR_INVALID_ARGS;
            }
            l.root_entries = 0;
            l.reserved_sectors = args.reserved_sectors ? args.reserved_sectors
                                                       : kFat32ReservedSectors;
            if (l.reserved_sectors <= kFat32BackupBootSector) {
                printf("fat_format: FAT32 needs more than %u reserved sectors\n",
                       kFat32BackupBootSector);
                return ERR_INVALID_ARGS;
            }
        } else {
            l.root_entries = args.root_entries ? args.root_entries : kDefaultRootEntries;
            // mount() rejects a root directory that does not fill whole sectors
            if (l.root_entries % (l.bytes_per_sector / 32)) {
                printf("fat_format: root entries %u is not a multiple of %u\n",
                       l.root_entries, l.bytes_per_sector / 32);
                return ERR_INVALID_ARGS;
            }
            l.reserved_sectors = args.reserved_sectors ? args.reserved_sectors : 1;
        }

        err = solve_layout(&l, want_bits);
        if (err < 0) {
            printf("fat_format: no FAT%u layout fits in %u sectors of %u bytes\n",
                   want_bits, l.total_sectors, l.bytes_per_sector);
            return err;
        }

        const uint32_t selected = fat_bits_for_cluster_count(l.data_cluster_count);
        if (selected == want_bits) {
            break;
        }
        if (args.fat_bits) {
            // The caller asked for a specific width and this geometry cannot
            // produce it. Say so rather than quietly formatting something else.
            printf("fat_format: FAT%u requested but %u data clusters selects FAT%u; "
                   "adjust sectors_per_cluster or total_sectors\n",
                   args.fat_bits, l.data_cluster_count, selected);
            return ERR_INVALID_ARGS;
        }
        want_bits = selected;
        err = ERR_NOT_VALID;
    }
    if (err < 0) {
        return err;
    }

    err = verify_layout_mounts_as(l, want_bits);
    if (err < 0) {
        return err;
    }

    dprintf(INFO,
            "fat_format: FAT%u, %u byte sectors, %u sectors/cluster, %u total sectors, "
            "%u FATs of %u sectors, %u root entries, %u data clusters\n",
            l.fat_bits, l.bytes_per_sector, l.sectors_per_cluster, l.total_sectors,
            l.fat_count, l.sectors_per_fat, l.root_entries, l.data_cluster_count);

    // Zero the whole metadata region: reserved sectors, every FAT, and the fixed
    // root directory. The data area is left alone.
    err = zero_sectors(dev, l, 0, l.data_start_sector);
    if (err < 0) {
        return err;
    }

    // FAT32 root directory lives in the data area and must be zeroed too.
    if (l.fat_bits == 32) {
        err = zero_sectors(dev, l, l.data_start_sector, l.sectors_per_cluster);
        if (err < 0) {
            return err;
        }
    }

    std::unique_ptr<uint8_t[]> sector_buf_storage(new (std::nothrow) uint8_t[l.bytes_per_sector]());
    if (!sector_buf_storage) {
        return ERR_NO_MEMORY;
    }
    uint8_t *sector_buf = sector_buf_storage.get();

    build_boot_sector(sector_buf, l, args);
    err = write_sector(dev, l, 0, sector_buf);
    if (err < 0) {
        return err;
    }
    if (l.fat_bits == 32) {
        err = write_sector(dev, l, kFat32BackupBootSector, sector_buf);
        if (err < 0) {
            return err;
        }

        build_fsinfo_sector(sector_buf, l);
        err = write_sector(dev, l, kFat32FsInfoSector, sector_buf);
        if (err < 0) {
            return err;
        }
        err = write_sector(dev, l, kFat32BackupBootSector + kFat32FsInfoSector, sector_buf);
        if (err < 0) {
            return err;
        }
    }

    err = write_fat_head_sectors(dev, l);
    if (err < 0) {
        return err;
    }

    // Everything above went out through bio_write, which is not cached at this
    // layer, so the volume is already on the device.
    return NO_ERROR;
}
