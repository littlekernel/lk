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
uint32_t fat_find_last_cluster_in_chain(fat_fs *fat, uint32_t starting_cluster);
status_t fat_allocate_cluster_chain(fat_fs *fat, uint32_t start_cluster, uint32_t count,
                                    uint32_t *first_cluster, uint32_t *last_cluster,
                                    bool zero_new_blocks);

/* general io routines */
uint32_t fat_sector_for_cluster(fat_fs *fat, uint32_t cluster);
ssize_t fat_read_cluster(fat_fs *fat, void *buf, uint32_t cluster);
ssize_t fat_zero_cluster(fat_fs *fat, uint32_t cluster);

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

// walk a path, returning the entry and the location where it was found
status_t fat_dir_walk(fat_fs *fat, const char *path, dir_entry *out_entry, dir_entry_location *loc);

// walk a path, allocating a new entry with the path name.
// returns the dir entry location
status_t fat_dir_allocate(fat_fs *fat, const char *path, fat_attribute attr, uint32_t starting_cluster, uint32_t size, dir_entry_location *loc);

status_t fat_dir_update_entry(fat_fs *fat, const dir_entry_location &loc, uint32_t starting_cluster, uint32_t size);
