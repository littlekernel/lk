/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <stdarg.h>
#include <lk/reg.h>
#include <stdio.h>
#include <kernel/thread.h>
#include <platform/armemu/memmap.h>
#include <platform/debug.h>

void platform_dputc(char c) {
    *REG8(DEBUG_STDOUT) = c;
}

int platform_dgetc(char *c, bool wait) {
    for (;;) {
        int8_t result = (int8_t)*REG8(DEBUG_STDIN);

        if (result == -1) {
            if (wait)
                continue;
            else
                return -1;
        }

        *c = (char)result;
        return 0;
    }
}

void debug_dump_regs(void) {
    *REG32(DEBUG_REGDUMP) = 1;
}

void platform_halt(void) {
    *REG32(DEBUG_HALT) = 1;
    for (;;);
}

void debug_dump_memory_bytes(void *mem, int len) {
    *REG32(DEBUG_MEMDUMPADDR) = (unsigned int)mem;
    *REG32(DEBUG_MEMDUMPLEN) = len;
    *REG32(DEBUG_MEMDUMP_BYTE) = 1;
}

void debug_dump_memory_halfwords(void *mem, int len) {
    len /= 2;

    *REG32(DEBUG_MEMDUMPADDR) = (unsigned int)mem;
    *REG32(DEBUG_MEMDUMPLEN) = len;
    *REG32(DEBUG_MEMDUMP_HALFWORD) = 1;
}

void debug_dump_memory_words(void *mem, int len) {
    len /= 4;

    *REG32(DEBUG_MEMDUMPADDR) = (unsigned int)mem;
    *REG32(DEBUG_MEMDUMPLEN) = len;
    *REG32(DEBUG_MEMDUMP_WORD) = 1;
}

void debug_set_trace_level(int trace_type, int level) {
    if (trace_type < 0 || trace_type >= 4)
        return;

    *REG32(DEBUG_SET_TRACELEVEL_CPU + trace_type * 4) = level;
}

uint32_t debug_cycle_count(void) {
    return *REG32(DEBUG_CYCLE_COUNT);
}
