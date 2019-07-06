/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

struct microblaze_context_switch_frame {
    uint32_t r1; // stack pointer
    uint32_t r2; // read-only small data base pointer

    uint32_t r13; // read-write small data base pointer
    uint32_t r14;
    uint32_t r15; // link register
    uint32_t r16;
    uint32_t r17;
    uint32_t r18;

    /* callee saved */
    uint32_t r19;
    uint32_t r20;
    uint32_t r21;
    uint32_t r22;
    uint32_t r23;
    uint32_t r24;
    uint32_t r25;
    uint32_t r26;
    uint32_t r27;
    uint32_t r28;
    uint32_t r29;
    uint32_t r30;
    uint32_t r31;
};

struct arch_thread {
    struct microblaze_context_switch_frame cs_frame;
};

void microblaze_context_switch(struct microblaze_context_switch_frame *oldcs,
                               struct microblaze_context_switch_frame *newcs);

