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

#include <arch/ops.h>
#include <stdbool.h>
#include <lk/compiler.h>
#include <arch/x86.h>

__BEGIN_CDECLS

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

static inline bool arch_in_int_handler(void) {
    return false;
}

struct arch_interrupt_saved_state {
    x86_flags_t flags;
};

static inline struct arch_interrupt_saved_state
arch_interrupt_save(void) {
    struct arch_interrupt_saved_state state = { .flags = x86_save_flags() };
    if (state.flags & X86_FLAGS_IF) {
        x86_cli();
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

    if (old_state.flags & X86_FLAGS_IF) {
        x86_sti();
    }
}

__END_CDECLS

#endif // ASSEMBLY
