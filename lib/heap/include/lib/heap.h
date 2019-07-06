/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stddef.h>
#include <sys/types.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

/* standard heap definitions */
void *malloc(size_t size) __MALLOC;
void *memalign(size_t boundary, size_t size) __MALLOC;
void *calloc(size_t count, size_t size) __MALLOC;
void *realloc(void *ptr, size_t size) __MALLOC;
void free(void *ptr);

void heap_init(void);

/* critical section time delayed free */
void heap_delayed_free(void *);

/* tell the heap to return any free pages it can find */
void heap_trim(void);

__END_CDECLS
