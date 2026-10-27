/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#ifndef ASSEMBLY

/* before arch/ops.h so it leaves out its empty arch_spinloop_pause() */
#define ARCH_HAS_SPINLOOP_PAUSE 1
#include <arch/ops.h>
#include <arch/x86.h>
#include <lk/compiler.h>

static inline void arch_spinloop_pause(void) {
    __asm__ volatile("pause" ::: "memory");
}

static inline ulong arch_cycle_count(void) {
#if X86_LEGACY
    return 0;
#else
    return __builtin_ia32_rdtsc();
#endif
}

#if WITH_SMP
#include <arch/x86/mp.h>
static inline struct thread *arch_get_current_thread(void) {
    return x86_get_current_thread();
}

static inline void arch_set_current_thread(struct thread *t) {
    x86_set_current_thread(t);
}

static inline uint arch_curr_cpu_num(void) {
    return x86_get_cpu_num();
}
#else
/* use a global pointer to store the current_thread */
extern struct thread *_current_thread;

static inline struct thread *arch_get_current_thread(void) {
    return _current_thread;
}

static inline void arch_set_current_thread(struct thread *t) {
    _current_thread = t;
}

static inline uint arch_curr_cpu_num(void) {
    return 0;
}
#endif

#if ARCH_X86_64
// relies on SSE2
#define mb()  __asm__ volatile("mfence" : : : "memory")
#define rmb() __asm__ volatile("lfence" : : : "memory")
#define wmb() __asm__ volatile("sfence" : : : "memory")
#else
// Store to the top of the stack as a load/store barrier. Cannot
// rely on SSE2 being intrinsically available for older i386 class hardware.
#define __storeload_barrier __asm__ volatile("lock; addl $0, (%%esp)" : : : "memory", "cc")
#define mb()                __storeload_barrier
#define rmb()               __storeload_barrier
#define wmb()               __storeload_barrier
#endif

#if WITH_SMP
// Normal memory is TSO: only a load can pass an earlier store, so smp_mb() is the one that
// needs an instruction. A locked add is cheaper than mfence and enough for normal memory,
// which is all the smp variants order (not WC or non-temporal accesses).
#if ARCH_X86_64
// below rsp to stay off the line the latest push wrote; harmless since it adds 0
#define smp_mb()  __asm__ volatile("lock; addl $0, -4(%%rsp)" : : : "memory", "cc")
#else
#define smp_mb()  __storeload_barrier
#endif
#define smp_rmb() CF
#define smp_wmb() CF
#else
#define smp_mb()  CF
#define smp_wmb() CF
#define smp_rmb() CF
#endif

#endif // !ASSEMBLY
