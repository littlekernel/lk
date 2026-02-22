/*
 * Copyright (c) 2026 Travis Geiselbrecht
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
#include <lk/reg.h>
#include <arch/arm64.h>
#include <arch/arm64/mp.h>


__BEGIN_CDECLS

// override of some routines declared in arch/interrupts.h
static inline void arch_enable_ints(void) {
    CF;
    __asm__ volatile("msr daifclr, #2" ::: "memory");
}

static inline void arch_disable_ints(void) {
    __asm__ volatile("msr daifset, #2" ::: "memory");
    CF;
}

static inline bool arch_ints_disabled(void) {
    unsigned long state;

    __asm__ volatile("mrs %0, daif" : "=r"(state));
    state &= (1<<7);

    return !!state;
}

struct arch_interrupt_saved_state {
    bool restore_irqs;
};

static inline struct arch_interrupt_saved_state
arch_interrupt_save(void) {
    struct arch_interrupt_saved_state state = { .restore_irqs = false };
    if (likely(!arch_ints_disabled())) {
        state.restore_irqs = true;
        arch_disable_ints();
    }

    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved before the arch_disable_ints() call.
    CF;

    return state;
}

static inline void
arch_interrupt_restore(struct arch_interrupt_saved_state old_state) {
    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved after the arch_enable_ints() call.
    CF;

    if (likely(old_state.restore_irqs)) {
        arch_enable_ints();
    }
}

__END_CDECLS

#endif // ASSEMBLY
