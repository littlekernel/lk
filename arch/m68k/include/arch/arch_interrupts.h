/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#ifndef ASSEMBLY

#include <stdint.h>
#include <stdbool.h>
#include <lk/compiler.h>

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

static inline bool arch_in_int_handler(void) {
    return false;
}

struct arch_interrupt_saved_state {
    bool restore_irqs;
};

static inline struct arch_interrupt_saved_state
arch_interrupt_save(void) {
    struct arch_interrupt_saved_state state = { .restore_irqs = !arch_ints_disabled() };
    arch_disable_ints();

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
