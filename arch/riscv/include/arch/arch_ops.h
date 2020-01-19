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
#include <arch/riscv/csr.h>
#include <arch/riscv/clint.h>

static inline void arch_enable_ints(void) {
    riscv_csr_set(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_IE);
}

static inline void arch_disable_ints(void) {
    riscv_csr_clear(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_IE);
}

static inline bool arch_ints_disabled(void) {
    return !(riscv_csr_read(RISCV_CSR_XSTATUS) & RISCV_CSR_XSTATUS_IE);
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

static inline struct thread *get_current_thread(void) {
    struct thread *current_thread;
    __asm__ volatile("mv %0,tp" : "=r"(current_thread));
    return current_thread;
}

static inline void set_current_thread(struct thread *t) {
    __asm__ volatile("mv tp,%0" : : "r"(t) : "tp");
}

static inline uint32_t arch_cycle_count(void) {
#if RISCV_M_MODE
    // use M version of the cycle if we're in machine mode. Some
    // cpus dont have a U mode alias for this.
    return riscv_csr_read(RISCV_CSR_MCYCLE);
#else
    return riscv_csr_read(RISCV_CSR_CYCLE);
#endif
}

static inline uint arch_curr_cpu_num(void) {
#if WITH_SMP
    const uint hart = riscv_current_hart();
    for (size_t i = 0; i < SMP_MAX_CPUS; i++) {
        if (hart_cpu_map[i] == (int)hart)
            return i;
        else if (unlikely(hart_cpu_map[i] == -1)) {
            if (i != 0 || hart == BOOT_HART) {
                hart_cpu_map[i] = hart;
                return i;
            }
        }
    }
    return -1;
#else
    return 0;
#endif
}

