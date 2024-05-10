/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <arch/mips.h>

static inline void arch_enable_ints(void) {
    CF;
#if 0
    uint32_t status = mips_read_c0_status();
    status |= 0x1;
    mips_write_c0_status(status);
#else
    __asm__ volatile("ei");
#endif
}

static inline void arch_disable_ints(void) {
#if 0
    uint32_t status = mips_read_c0_status();
    status &= ~0x1;
    mips_write_c0_status(status);
#else
    __asm__ volatile("di");
#endif
    CF;
}

static inline bool arch_ints_disabled(void) {
    uint32_t state;

    state = mips_read_c0_status();

    return (state & (1<<1)) || !(state & (1<<0)); // check if EXL or IE is set
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
// TODO: do we need these for mips?
#define mb()        CF
#define wmb()       CF
#define rmb()       CF
#define smp_mb()    CF
#define smp_wmb()   CF
#define smp_rmb()   CF


