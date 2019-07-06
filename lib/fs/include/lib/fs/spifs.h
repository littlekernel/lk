/*
 * Copyright (c) 2015 Gurjant Kalsi <me@gurjantkalsi.com>
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/fs.h>

#define DEAULT_SPIFS_MOUNT_POINT "/spifs"
#define DEAULT_SPIFS_NAME        "spifs"

typedef struct {
    uint32_t toc_pages;
} spifs_format_args_t;

