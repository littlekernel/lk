/*
 * Copyright (c) 2013 Heather Lee Wilson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stddef.h>
#include <sys/types.h>
#include <lib/norfs_inode.h>

/* Tools to help test. */

uint32_t write_pointer;
uint32_t norfs_nvram_offset;

status_t find_free_block(uint32_t *ptr);
uint8_t block_num(uint32_t flash_pointer);
void dump_bank(void);
void wipe_fs(void);
status_t collect_block(uint32_t garbage_block, uint32_t *garbage_write_ptr);
bool get_inode(uint32_t key, struct norfs_inode **inode);

