/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

struct riscv_context_switch_frame {
    unsigned long ra; // return address (x1)
    unsigned long sp; // stack pointer (x2)

    unsigned long s0; // x8-x9
    unsigned long s1;

    unsigned long s2; // x18-x27
    unsigned long s3;
    unsigned long s4;
    unsigned long s5;
    unsigned long s6;
    unsigned long s7;
    unsigned long s8;
    unsigned long s9;
    unsigned long s10;
    unsigned long s11;
};

struct arch_thread {
    struct riscv_context_switch_frame cs_frame;
};

void riscv_context_switch(struct riscv_context_switch_frame *oldcs,
                          struct riscv_context_switch_frame *newcs);

