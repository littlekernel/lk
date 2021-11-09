/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

struct m68k_context_switch_frame {
    uint32_t sp;
    uint32_t pc;

    /* callee saved */
    uint32_t d2;
    uint32_t d3;
    uint32_t d4;
    uint32_t d5;
    uint32_t d6;
    uint32_t d7;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
};

// NOTE: consider using 'state on stack' instead
struct arch_thread {
    struct m68k_context_switch_frame cs_frame;
};

void m68k_context_switch(struct m68k_context_switch_frame *oldcs, struct m68k_context_switch_frame *newcs);

