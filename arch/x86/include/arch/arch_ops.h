/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>

#ifndef ASSEMBLY

#include <arch/x86.h>

/* override of some routines */
static inline void arch_enable_ints(void) {
    CF;
    __asm__ volatile("sti");
}

static inline void arch_disable_ints(void) {
    __asm__ volatile("cli");
    CF;
}

static inline bool arch_ints_disabled(void) {
    x86_flags_t state;

    __asm__ volatile(
#if ARCH_X86_32
        "pushfl;"
        "popl %%eax"
#elif ARCH_X86_64
        "pushfq;"
        "popq %%rax"
#endif
        : "=a" (state)
        :: "memory");

    return !(state & (1<<9));
}

static inline ulong arch_cycle_count(void) {
#if X86_LEGACY
    return 0;
#elif ARCH_X86_64
    uint32_t low, high;
    rdtsc(low, high);
    return ((ulong)high << 32) | low;
#else
    uint32_t timestamp;
    rdtscl(timestamp);
    return timestamp;
#endif
}

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

#endif // !ASSEMBLY
