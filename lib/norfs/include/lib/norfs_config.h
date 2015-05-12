/*
 * Copyright (c) 2013 Heather Lee Wilson
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __NORFS_CONFIG_H
#define __NORFS_CONFIG_H

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

#endif
