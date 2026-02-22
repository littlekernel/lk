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
#include <arch/mips.h>

__BEGIN_CDECLS

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

static inline bool arch_in_int_handler(void) {
    return false;
}

struct arch_interrupt_saved_state {
    bool restore_irqs;
};

static inline struct arch_interrupt_saved_state
arch_interrupt_save(void) {
    struct arch_interrupt_saved_state state = { .restore_irqs = false };
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
arch_interrupt_restore(struct arch_interrupt_saved_state old_state) {
    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved after the arch_enable_ints() call.
    CF;

    if (old_state.restore_irqs) {
        arch_enable_ints();
    }
}

__END_CDECLS

#endif // ASSEMBLY
