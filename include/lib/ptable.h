/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
 * Copyright (c) 2014, Travis Geiselbrecht
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
#pragma once

#include <compiler.h>
#include <sys/types.h>
#include <lib/bio.h>

status_t ptable_scan(bdev_t *bdev, uint64_t offset);

bool ptable_found_valid(void);
bdev_t *ptable_get_device(void);

#define MAX_FLASH_PTABLE_NAME_LEN 12

struct ptable_entry {
    uint64_t offset;
    uint64_t length;
    uint32_t flags;
    uint8_t name[MAX_FLASH_PTABLE_NAME_LEN];
};

status_t ptable_find(const char *name, struct ptable_entry *entry) __NONNULL((1));

status_t ptable_create_default(bdev_t *bdev, uint64_t offset) __NONNULL();
status_t ptable_add(const char *name, uint64_t offset, uint64_t len, uint32_t flags) __NONNULL();
status_t ptable_remove(const char *name) __NONNULL();
void ptable_dump(void);

#define FLASH_PTABLE_ALLOC_END 0x1
off_t ptable_allocate(uint64_t length, uint flags);
off_t ptable_allocate_at(off_t offset, uint64_t length);

