/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/cbuf.h>

extern cbuf_t console_input_buf;

void platform_init_debug_early(void);
void platform_init_debug(void);
void platform_init_interrupts(void);
void platform_init_timer(void);

