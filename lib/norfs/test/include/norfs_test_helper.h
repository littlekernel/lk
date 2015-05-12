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
#ifndef __NORFS_TEST_HELPER_H
#define __NORFS_TEST_HELPER_H

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

#endif
