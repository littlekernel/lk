/*
 * Copyright (c) 2015 Steve White
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/bio.h>
#include <lib/fs.h>

#include "fat_fs.h"

typedef void *fsfilecookie;

status_t fat_mount(bdev_t *dev, fscookie **cookie);
status_t fat_unmount(fscookie *cookie);

/* file api */
status_t fat_open_file(fscookie *cookie, const char *path, filecookie **fcookie);
ssize_t fat_read_file(filecookie *fcookie, void *buf, off_t offset, size_t len);
status_t fat_close_file(filecookie *fcookie);
status_t fat_stat_file(filecookie *fcookie, struct file_stat *stat);

/* file allocation table parsing */
uint32_t fat_next_cluster_in_chain(fat_fs_t *fat, uint32_t cluster);
uint32_t file_offset_to_cluster(fat_fs_t *fat, uint32_t start_cluster, off_t offset);

/* general io routines */
uint32_t fat_sector_for_cluster(fat_fs_t *fat, uint32_t cluster);
ssize_t fat_read_cluster(fat_fs_t *fat, void *buf, uint32_t cluster);

