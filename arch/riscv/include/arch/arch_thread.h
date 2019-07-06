/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

struct riscv32_context_switch_frame {
    uint32_t ra; // return address (x1)
    uint32_t sp; // stack pointer (x2)
    uint32_t tp; // thread pointer (x4)

    uint32_t s0; // x8-x9
    uint32_t s1;

    uint32_t s2; // x18-x27
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
};

struct arch_thread {
    struct riscv32_context_switch_frame cs_frame;
};

void riscv32_context_switch(struct riscv32_context_switch_frame *oldcs,
                            struct riscv32_context_switch_frame *newcs);

