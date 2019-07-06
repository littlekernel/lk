/*
 * Copyright (c) 2013 Google, Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stddef.h>
#include <sys/types.h>
#include <lib/version.h>

/* routines for searching for and finding a build signature */
status_t buildsig_search(const void *ptr, size_t search_len, size_t max_len,
                         const lk_version_t **version);

#define DEFAULT_BUILDSIG_SEARCH_LEN 1024

