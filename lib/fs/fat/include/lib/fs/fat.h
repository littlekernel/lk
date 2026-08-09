/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdint.h>
#include <sys/types.h>

__BEGIN_CDECLS

// Arguments for fs_format_device("fat", device, args).
//
// Passing a NULL args pointer formats the whole device with defaults. A zero in
// any individual field selects the default described for that field, so a caller
// only has to fill in what it cares about.
//
// The layout is computed and then re-derived exactly the way mount() does; if
// the result would not select the requested fat_bits, format fails with
// ERR_INVALID_ARGS rather than producing a volume that mounts at the wrong width.
typedef struct fat_format_args {
    // 12, 16 or 32. Default: whichever the resulting data cluster count selects.
    uint32_t fat_bits;
    // 512, 1024 or 2048. Default: the block device's block size.
    uint32_t bytes_per_sector;
    // Power of two, 1 to 128. Default: chosen from the volume size.
    uint32_t sectors_per_cluster;
    // Volume size in sectors. Default: as many sectors as the device holds.
    uint32_t total_sectors;
    // Number of FAT copies, 1 to 8. Default: 2.
    uint32_t fat_count;
    // Fixed root directory entries, FAT12/16 only, and must be a multiple of
    // bytes_per_sector / 32. Default: 512. Must be 0 for FAT32.
    uint32_t root_entries;
    // Default: 1 on FAT12/16, 32 on FAT32 (which needs room for the FSInfo
    // sector and the backup boot sector).
    uint32_t reserved_sectors;
    // Up to 11 characters, space padded. Default: "NO NAME".
    const char *volume_label;
    // Seed for the volume serial number. Default: a fixed constant, so that
    // formatting the same geometry twice produces an identical image.
    uint32_t volume_id;
} fat_format_args_t;

__END_CDECLS
