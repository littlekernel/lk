/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>


void lk_main(ulong arg0, ulong arg1, ulong arg2, ulong arg3) __NO_RETURN __EXTERNALLY_VISIBLE;
void lk_secondary_cpu_entry(void);
void lk_init_secondary_cpus(uint secondary_cpu_count);
