/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <lk/debug.h>
#include <arch/riscv.h>

static inline void arch_enable_ints(void) {
    riscv_csr_set(mstatus, RISCV_STATUS_MIE);
}

static inline void arch_disable_ints(void) {
    riscv_csr_clear(mstatus, RISCV_STATUS_MIE);
}

static inline bool arch_ints_disabled(void) {
    ulong val = riscv_csr_read(mstatus);
    return !(val & RISCV_STATUS_MIE);
}

static inline int atomic_add(volatile int *ptr, int val) {
    return __atomic_fetch_add(ptr, val, __ATOMIC_RELAXED);
}

static inline int atomic_or(volatile int *ptr, int val) {
    return __atomic_fetch_or(ptr, val, __ATOMIC_RELAXED);
}

static inline int atomic_and(volatile int *ptr, int val) {
    return __atomic_fetch_and(ptr, val, __ATOMIC_RELAXED);
}

static inline int atomic_swap(volatile int *ptr, int val) {
    return __atomic_exchange_n(ptr, val, __ATOMIC_RELAXED);
}

/* use a global pointer to store the current_thread */
extern struct thread *_current_thread;

static inline struct thread *get_current_thread(void) {
    return _current_thread;
}

static inline void set_current_thread(struct thread *t) {
    _current_thread = t;
}

static inline uint32_t arch_cycle_count(void) {
    uint32_t count;

    //__asm__("rdcycle %0" : "=r"(count));
    count = riscv_csr_read(mcycle);
    return count;
}

static inline uint arch_curr_cpu_num(void) {
    return 0;
}

