/*
 * Copyright (c) 2013 Google, Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>

__BEGIN_CDECLS

#define VERSION_STRUCT_VERSION 0x1

typedef struct {
    unsigned int struct_version;
    const char *arch;
    const char *platform;
    const char *target;
    const char *project;
    const char *buildid;
} lk_version_t;

extern const lk_version_t lk_version;

void print_version(void);

__END_CDECLS

