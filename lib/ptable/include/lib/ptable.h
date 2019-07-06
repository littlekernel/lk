/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
 * Copyright (c) 2014, Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>
#include <lib/bio.h>

#define MAX_FLASH_PTABLE_NAME_LEN 12
#define FLASH_PTABLE_ALLOC_END 0x1

struct ptable_entry {
    uint64_t offset;
    uint64_t length;
    uint32_t flags;
    uint8_t name[MAX_FLASH_PTABLE_NAME_LEN];
};

bool     ptable_found_valid(void);
bdev_t  *ptable_get_device(void);
status_t ptable_scan(const char *bdev_name, uint64_t offset);
status_t ptable_find(const char *name, struct ptable_entry *entry) __NONNULL((1));
status_t ptable_create_default(const char *bdev_name, uint64_t offset) __NONNULL();
status_t ptable_add(const char *name, uint64_t min_len, uint32_t flags) __NONNULL();
status_t ptable_remove(const char *name) __NONNULL();
void     ptable_dump(void);
