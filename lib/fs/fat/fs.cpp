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
#include <stdlib.h>
#include <lib/bcache/bcache_block_ref.h>

#include "fat_priv.h"
#include "fat_fs.h"
#include "file.h"
#include "dir.h"

#define LOCAL_TRACE FAT_GLOBAL_TRACE(0)

namespace {

constexpr uint32_t kFsInfoLeadSig = 0x41615252;
constexpr uint32_t kFsInfoStructSig = 0x61417272;
constexpr uint32_t kFsInfoTrailSig = 0xaa550000;

// FAT entry 1 dirty/clean bits (FAT16 bit 15, FAT32 bit 27).
// When set the volume was cleanly unmounted; cleared on mount to mark it dirty.
constexpr uint16_t kFat16ClnShutBit = 0x8000u;
constexpr uint32_t kFat32ClnShutBit = 0x08000000u;

} // anonymous namespace

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

status_t fat_fs::write_fsinfo_locked() {
    DEBUG_ASSERT(lock.is_held());

    if (info_.fat_bits != 32 || !info_.fsinfo_valid) {
        return NO_ERROR;
    }

    bcache_block_ref bref(bcache_);
    status_t err = bref.get_block(info_.fsinfo_sector);
    if (err < 0) {
        return err;
    }

    auto *buf = static_cast<uint8_t *>(bref.ptr());
    fat_write32(buf, 0x000, kFsInfoLeadSig);
    fat_write32(buf, 0x1e4, kFsInfoStructSig);
    fat_write32(buf, 0x1e8, info_.fsinfo_free_clusters);
    fat_write32(buf, 0x1ec, info_.fsinfo_next_free);
    fat_write32(buf, 0x1fc, kFsInfoTrailSig);
    bref.mark_dirty();

    return NO_ERROR;
}

status_t fat_fs::adjust_fsinfo_free_clusters(int32_t delta) {
    DEBUG_ASSERT(lock.is_held());

    if (info_.fat_bits != 32 || !info_.fsinfo_valid ||
        info_.fsinfo_free_clusters == UINT32_MAX) {
        return NO_ERROR;
    }

    if (delta < 0) {
        uint32_t dec = static_cast<uint32_t>(-delta);
        info_.fsinfo_free_clusters = (dec > info_.fsinfo_free_clusters)
                                         ? 0
                                         : (info_.fsinfo_free_clusters - dec);
    } else {
        uint32_t inc = static_cast<uint32_t>(delta);
        uint32_t new_count = info_.fsinfo_free_clusters + inc;
        info_.fsinfo_free_clusters = MIN(new_count, info_.total_clusters);
    }

    return write_fsinfo_locked();
}

status_t fat_fs::set_fsinfo_next_free(uint32_t next_free) {
    DEBUG_ASSERT(lock.is_held());

    if (info_.fat_bits != 32 || !info_.fsinfo_valid) {
        return NO_ERROR;
    }

    if (next_free < 2 || next_free >= info_.total_clusters) {
        info_.fsinfo_next_free = UINT32_MAX;
    } else {
        info_.fsinfo_next_free = next_free;
    }

    return write_fsinfo_locked();
}

// Update FAT entry 1 across all FAT copies: clear ClnShutBit (mark dirty) or set it (mark clean).
// FAT12 has no such bit, so this is a no-op for FAT12.
status_t fat_fs::mark_volume_dirty_locked() {
    return set_volume_clean_bit_locked(false);
}

status_t fat_fs::mark_volume_clean_locked() {
    return set_volume_clean_bit_locked(true);
}

status_t fat_fs::set_volume_clean_bit_locked(bool clean) {
    DEBUG_ASSERT(lock.is_held());

    if (info_.fat_bits != 16 && info_.fat_bits != 32) {
        return NO_ERROR;
    }

    // FAT entry 1 is always in the first FAT sector of each copy.
    // offset in bytes: FAT16 → 1*2 = 2, FAT32 → 1*4 = 4.
    const uint32_t fat1_byte_offset = (info_.fat_bits == 32) ? 4u : 2u;

    for (uint32_t fat_index = 0; fat_index < info_.fat_count; fat_index++) {
        const uint32_t fat_base_sector = info_.reserved_sectors + fat_index * info_.sectors_per_fat;

        bcache_block_ref bref(bcache_);
        status_t err = bref.get_block(fat_base_sector);
        if (err < 0) {
            return err;
        }

        if (info_.fat_bits == 32) {
            auto *table = static_cast<uint32_t *>(bref.ptr());
            const uint32_t index = fat1_byte_offset / 4;
            uint32_t entry = LE32(table[index]);
            if (clean) {
                entry |= kFat32ClnShutBit;
            } else {
                entry &= ~kFat32ClnShutBit;
            }
            table[index] = LE32(entry);
        } else { // FAT16
            auto *table = static_cast<uint16_t *>(bref.ptr());
            const uint32_t index = fat1_byte_offset / 2;
            uint16_t entry = LE16(table[index]);
            if (clean) {
                entry |= kFat16ClnShutBit;
            } else {
                entry &= ~kFat16ClnShutBit;
            }
            table[index] = LE16(entry);
        }
        bref.mark_dirty();
    }

    return NO_ERROR;
}

// static fs hooks
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
        if (info->active_fat >= info->fat_count) {
            info->active_fat = 0;
        }

        // read FSInfo metadata if available and valid.
        info->fsinfo_sector = fat_read16(bs, 0x30);
        if (info->fsinfo_sector < info->reserved_sectors) {
            uint8_t *fsi = static_cast<uint8_t *>(malloc(info->bytes_per_sector));
            if (fsi) {
                auto ac3 = lk::make_auto_call([&]() { free(fsi); });
                ssize_t fsi_err = bio_read(dev, fsi,
                                           static_cast<off_t>(info->fsinfo_sector) * info->bytes_per_sector,
                                           info->bytes_per_sector);
                if (fsi_err >= 0) {
                    uint32_t lead_sig = fat_read32(fsi, 0x000);
                    uint32_t struct_sig = fat_read32(fsi, 0x1e4);
                    uint32_t trail_sig = fat_read32(fsi, 0x1fc);
                    if (lead_sig == kFsInfoLeadSig && struct_sig == kFsInfoStructSig &&
                        trail_sig == kFsInfoTrailSig) {
                        info->fsinfo_valid = true;

                        uint32_t free_clusters = fat_read32(fsi, 0x1e8);
                        info->fsinfo_free_clusters =
                            (free_clusters <= info->total_clusters) ? free_clusters : UINT32_MAX;

                        uint32_t next_free = fat_read32(fsi, 0x1ec);
                        info->fsinfo_next_free =
                            (next_free >= 2 && next_free < info->total_clusters) ? next_free
                                                                                  : UINT32_MAX;
                    }
                }
            }
        }
    } else {
        // On a FAT 12 or FAT 16 volumes the root directory is at a fixed position immediately after the File Allocation Tables
        info->root_start_sector = info->reserved_sectors + info->fat_count * info->sectors_per_fat;
    }

    info->bytes_per_cluster = info->sectors_per_cluster * info->bytes_per_sector;

    int bcache_size = MIN(16, 64 * info->sectors_per_cluster);

    dprintf(INFO, "FAT: creating bcache of %d entries of %u bytes\n", bcache_size, info->bytes_per_sector);

    fat->bcache_ = bcache_create(fat->dev(), info->bytes_per_sector, bcache_size);

    // we're okay, cancel our cleanup of the fat structure
    ac2.cancel();

#if LOCAL_TRACE
    printf("Mounted FAT volume, some information:\n");
    fat_dump(fat);
#endif

    // Mark the volume as dirty (in-use). Flush so the bit reaches disk promptly
    // and fsck will see it as unclean if we crash before unmounting.
    {
        AutoLock guard(fat->lock);
        fat->mark_volume_dirty_locked();
    }
    bcache_flush(fat->bcache_);

    *cookie = (fscookie *)fat;

    return result;
}

// static
status_t fat_fs::unmount(fscookie *cookie) {
    auto *fat = (fat_fs *)cookie;

    {
        AutoLock guard(fat->lock);

        // TODO: handle unmounting when files/dirs are active
        DEBUG_ASSERT(list_is_empty(&fat->file_list_));

        if (LK_DEBUGLEVEL > INFO) {
            bcache_dump(fat->bcache(), "FAT bcache ");
        }
        fat->mark_volume_clean_locked();
        fat->write_fsinfo_locked();
        bcache_flush(fat->bcache());
        bcache_destroy(fat->bcache());
    }

    delete fat;

    return NO_ERROR;
}

static const struct fs_api fat_api = {
    .format = nullptr,
    .fs_stat = nullptr,

    .mount = fat_fs::mount,
    .unmount = fat_fs::unmount,
    .open = fat_file::open_file,
    .create = fat_file::create_file,
    .remove = fat_dir::remove,
    .rmdir = fat_dir::rmdir,
    .truncate = fat_file::truncate_file,
    .stat = fat_file::stat_file,
    .read = fat_file::read_file,
    .write = fat_file::write_file,
    .close = fat_file::close_file,

    .mkdir = fat_dir::mkdir,
    .opendir = fat_dir::opendir,
    .readdir = fat_dir::readdir,
    .closedir = fat_dir::closedir,

    .file_ioctl = nullptr,
};

STATIC_FS_IMPL(fat, &fat_api);
