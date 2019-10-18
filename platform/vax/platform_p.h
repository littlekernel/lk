/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/cbuf.h>

// super simple cheesy system detection mechanism
// mostly cribbed from netbsd
extern uint32_t vax_cputype;
extern uint32_t vax_boardtype;

void platform_early_console_init(void);
