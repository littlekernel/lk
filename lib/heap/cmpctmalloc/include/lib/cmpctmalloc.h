/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdbool.h>
#include <stddef.h>

__BEGIN_CDECLS

void *cmpct_alloc(size_t);
void *cmpct_realloc(void *, size_t);
void cmpct_free(void *);
void *cmpct_memalign(size_t size, size_t alignment);

void cmpct_init(void);
// panic_time skips the heap lock, which the crashed thread may hold
void cmpct_dump(bool panic_time);
void cmpct_trim(void);

__END_CDECLS
