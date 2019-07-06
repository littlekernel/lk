/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <lk/compiler.h>
#include <lib/bootimage_struct.h>

typedef struct bootimage bootimage_t;

status_t bootimage_open(const void *ptr, size_t len, bootimage_t **bi) __NONNULL();
status_t bootimage_close(bootimage_t *bi) __NONNULL();
status_t bootimage_get_range(bootimage_t *bi, const void **ptr, size_t *len) __NONNULL((1));

/* ask for a file section of the bootimage, by type */
status_t bootimage_get_file_section(bootimage_t *bi, uint32_t type, const void **ptr, size_t *len) __NONNULL((1));

