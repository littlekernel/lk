/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <stdbool.h>

struct riscv_fpu_state {
    double f[32];
    unsigned long fscr;
};

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

#if RISCV_FPU
    bool fpu_dirty;
    struct riscv_fpu_state fpu;
#endif
};

struct arch_thread {
    struct riscv_context_switch_frame cs_frame;
};

void riscv_context_switch(struct riscv_context_switch_frame *oldcs,
                          struct riscv_context_switch_frame *newcs);

#if RISCV_FPU
// save and restore old and new state.
void riscv_fpu_save(struct riscv_fpu_state *state);
void riscv_fpu_restore(struct riscv_fpu_state *state);

// initialize the fpu state to zero
void riscv_fpu_zero(void);

#endif

