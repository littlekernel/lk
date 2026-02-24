/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>

// Simple interface for the VGA text mode console.

__BEGIN_CDECLS

void platform_init_console(void);

void cputc(char c);

__END_CDECLS

