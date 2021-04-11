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

static inline struct thread *arch_get_current_thread(void) {
    return riscv_get_current_thread();
}

static inline void arch_set_current_thread(struct thread *t) {
    riscv_set_current_thread(t);
}

static inline ulong arch_cycle_count(void) {
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
    return riscv_get_percpu()->cpu_num;
#else
    return 0;
#endif
}

#define mb()        __asm__ volatile("fence iorw,iorw" ::: "memory");
#define wmb()       __asm__ volatile("fence ow,ow" ::: "memory");
#define rmb()       __asm__ volatile("fence ir,ir" ::: "memory");

#ifdef WITH_SMP
#define smp_mb()    __asm__ volatile("fence rw,rw" ::: "memory");
#define smp_wmb()   __asm__ volatile("fence w,w" ::: "memory");
#define smp_rmb()   __asm__ volatile("fence r,r" ::: "memory");
#else
#define smp_mb()    CF
#define smp_wmb()   CF
#define smp_rmb()   CF
#endif


