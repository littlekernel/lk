/*
 * Copyright (c) 2013 Heather Lee Wilson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <platform/flash_nor_config.h>

#define NORFS_NUM_BLOCKS 8
#define NORFS_BLOCK_HEADER_SIZE 8
#define NORFS_NVRAM_SIZE (FLASH_PAGE_SIZE * NORFS_NUM_BLOCKS)
#define NORFS_MAX_OBJ_LEN (FLASH_PAGE_SIZE/2 - NORFS_OBJ_OFFSET)
#define NORFS_BANK 0

#define NORFS_FLASH_SIZE(obj_size) (uint16_t)(obj_size + NORFS_OBJ_OFFSET)

#define NORFS_AVAILABLE_SPACE ((NORFS_NVRAM_SIZE - NORFS_NUM_BLOCKS * NORFS_BLOCK_HEADER_SIZE) / 2)
#define NORFS_MIN_FREE_BLOCKS 1

#define NORFS_KEY_OFFSET 0
#define NORFS_VERSION_OFFSET 4
#define NORFS_LENGTH_OFFSET 6
#define NORFS_FLAGS_OFFSET 8
#define NORFS_CHECKSUM_OFFSET 10
#define NORFS_OBJ_OFFSET 12

#define NORFS_DELETED_MASK 1

