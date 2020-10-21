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

__BEGIN_CDECLS

/* Standard console output routines that may be buffered */
void platform_dputc(char c);
int platform_dgetc(char *c, bool wait);

/* Unbuffered versions of above, usually used a panic or crash time */
void platform_pputc(char c);
int platform_pgetc(char *c, bool wait);

__END_CDECLS
