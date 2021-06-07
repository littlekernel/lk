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

static inline void arch_enable_ints(void) {
    asm("andi #~(7 << 8), %sr");
}

static inline void arch_disable_ints(void) {
    asm("ori #(7 << 8), %sr");
}

static inline bool arch_ints_disabled(void) {
    uint16_t sr;
    asm("move %%sr, %0" : "=r"(sr));

    return (sr & (7 << 8));
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

// TODO: see if there's a proper (or required) memory barrier on 68k
#define mb()        CF
#define wmb()       CF
#define rmb()       CF
#define smp_mb()    CF
#define smp_wmb()   CF
#define smp_rmb()   CF

__END_CDECLS
