/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <lk/debug.h>

__BEGIN_CDECLS

#define M68K_SR_IPM_MASK ((0b111 << 8)) // interrupt priority mask

static inline void arch_enable_ints(void) {
    asm volatile("andi %0, %%sr" :: "i" (~M68K_SR_IPM_MASK) : "memory");
}

static inline void arch_disable_ints(void) {
    asm volatile("ori %0, %%sr" :: "i" (M68K_SR_IPM_MASK) : "memory");
}

static inline bool arch_ints_disabled(void) {
    uint16_t sr;
    asm volatile("move %%sr, %0" : "=dm"(sr) :: "memory");

    // if the IPM is != 0, consider interrupts disabled
    return (sr & M68K_SR_IPM_MASK);
}

// use a global pointer to store the current_thread
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
#define mb()        CF
#define wmb()       CF
#define rmb()       CF
#define smp_mb()    CF
#define smp_wmb()   CF
#define smp_rmb()   CF

__END_CDECLS
