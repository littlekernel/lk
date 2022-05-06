/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <lib/bio.h>
#include <lib/fs.h>

#include "fat_fs.h"

// Individual files should
// #define LOCAL_TRACE FAT_GLOBAL_TRACE(0)
// can override here for all fat files
#define FAT_GLOBAL_TRACE(local) (local | 0)

typedef void *fsfilecookie;

/* file allocation table parsing */
uint32_t fat_next_cluster_in_chain(fat_fs *fat, uint32_t cluster);
uint32_t file_offset_to_cluster(fat_fs *fat, uint32_t start_cluster, off_t offset);

/* general io routines */
uint32_t fat_sector_for_cluster(fat_fs *fat, uint32_t cluster);
ssize_t fat_read_cluster(fat_fs *fat, void *buf, uint32_t cluster);

// general directory apis outside of an object
struct dir_entry {
    fat_attribute attributes;
    uint32_t length;
    uint32_t start_cluster;
    // TODO time
};

// used as a key for a file/dir in the open file table
struct dir_entry_location {
    uint32_t starting_dir_cluster;
    uint32_t dir_offset;
};

inline bool operator==(const dir_entry_location &a, const dir_entry_location &b) {
    return (a.starting_dir_cluster == b.starting_dir_cluster && a.dir_offset == b.dir_offset);
}

status_t fat_walk(fat_fs *fat, const char *path, dir_entry *out_entry, dir_entry_location *loc);
