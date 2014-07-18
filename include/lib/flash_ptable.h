/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
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
#ifndef __LIB_FLASH_PTABLE_H
#define __LIB_FLASH_PTABLE_H

#include <sys/types.h>
#include <dev/flash_nor.h>

status_t flash_ptable_init(void);
status_t flash_ptable_scan(const struct flash_nor_bank *bank);

bool flash_ptable_found_valid(void);
const struct flash_nor_bank *flash_ptable_get_flash_bank(void);

#define MAX_FLASH_PTABLE_NAME_LEN 12

struct ptable_entry {
    uint32_t offset;
    uint32_t length;
    uint32_t flags;
    uint8_t name[MAX_FLASH_PTABLE_NAME_LEN];
};

status_t flash_ptable_find(struct ptable_entry *entry, const char *name);

status_t flash_ptable_create_default(const struct flash_nor_bank *bank);
status_t flash_ptable_add(const char *name, uint32_t offset, uint32_t len, uint32_t flags);
status_t flash_ptable_remove(const char *name);
void flash_ptable_dump(void);

#define FLASH_PTABLE_ALLOC_END 0x1
ssize_t flash_ptable_allocate(size_t length, uint flags);
ssize_t flash_ptable_allocate_at(size_t offset, size_t length);

#endif

