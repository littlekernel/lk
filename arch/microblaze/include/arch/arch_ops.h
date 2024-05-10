/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>

#define USE_MSRSET 1

static inline void arch_enable_ints(void) {
    CF;
    uint32_t temp;
    __asm__ volatile(
#if USE_MSRSET
        "msrset %0, (1<<1)"
#else
        "mfs    %0, rmsr;"
        "ori    %0, %0, (1<<1);"
        "mts    rmsr, %0"
#endif
        : "=r" (temp));
}

static inline void arch_disable_ints(void) {
    uint32_t temp;
    __asm__ volatile(
#if USE_MSRSET
        "msrclr %0, (1<<1)"
#else
        "mfs    %0, rmsr;"
        "andni  %0, %0, (1<<1);"
        "mts    rmsr, %0"
#endif
        : "=r" (temp));
    CF;
}

static inline bool arch_ints_disabled(void) {
    uint32_t state;

    __asm__ volatile(
        "mfs    %0, rmsr;"
        : "=r" (state));

    return !(state & (1<<1));
}

/* use a global pointer to store the current_thread */
extern struct thread *_current_thread;

static inline struct thread *arch_get_current_thread(void) {
    return _current_thread;
}

static inline void arch_set_current_thread(struct thread *t) {
    _current_thread = t;
}

static inline ulong arch_cycle_count(void) { return 0; }

static inline uint arch_curr_cpu_num(void) {
    return 0;
}

// Default barriers for architectures that generally don't need them
// TODO: do we need these for microblaze?
#define mb()        CF
#define wmb()       CF
#define rmb()       CF
#define smp_mb()    CF
#define smp_wmb()   CF
#define smp_rmb()   CF

