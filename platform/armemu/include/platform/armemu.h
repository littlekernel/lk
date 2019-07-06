/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <platform/armemu/memmap.h>

void debug_dump_regs(void);

void debug_dump_memory_bytes(void *mem, int len);
void debug_dump_memory_halfwords(void *mem, int len);
void debug_dump_memory_words(void *mem, int len);

void debug_set_trace_level(int trace_type, int level);

