/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

struct or1k_context_switch_frame {
    uint32_t r1; // stack pointer
    uint32_t r2; // frame pointer

    uint32_t r9; // link register

    /* callee saved */
    uint32_t r10;
    uint32_t r14;
    uint32_t r16;
    uint32_t r18;
    uint32_t r20;
    uint32_t r22;
    uint32_t r24;
    uint32_t r26;
    uint32_t r28;
    uint32_t r30;
};

struct arch_thread {
    struct or1k_context_switch_frame cs_frame;
};

void or1k_context_switch(struct or1k_context_switch_frame *oldcs,
                         struct or1k_context_switch_frame *newcs);
