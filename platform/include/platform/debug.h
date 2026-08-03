/*
 * Copyright (c) 2008 Travis Geiselbrecht
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

/* Standard console output routines that may be buffered */
void platform_dputc(char c);
int platform_dgetc(char *c, bool wait);

/* Batched version of platform_dputc. A weak default implementation simply loops
 * over the string calling platform_dputc, but platforms may override it to push
 * a run of characters into a hardware fifo at once.
 */
void platform_dputs(const char *str, size_t len);

/* Unbuffered versions of above, usually used a panic or crash time */
void platform_pputc(char c);
int platform_pgetc(char *c, bool wait);
void platform_pputs(const char *str, size_t len);

__END_CDECLS
