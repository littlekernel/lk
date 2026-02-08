/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#ifndef ASSEMBLY

#include <arch/ops.h>
#include <stdbool.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

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

static inline bool arch_in_int_handler(void) {
    return false;
}

struct spin_lock_saved_state {
    bool restore_irqs;
};

static inline struct spin_lock_saved_state
arch_interrupt_save(void) {
    struct spin_lock_saved_state state = { .restore_irqs = false };
    if (!arch_ints_disabled()) {
        state.restore_irqs = true;
        arch_disable_ints();
    }

    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved before the arch_disable_ints() call.
    CF;

    return state;
}

static inline void
arch_interrupt_restore(struct spin_lock_saved_state old_state) {
    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved after the arch_enable_ints() call.
    CF;

    if (old_state.restore_irqs) {
        arch_enable_ints();
    }
}

__END_CDECLS

#endif // ASSEMBLY
