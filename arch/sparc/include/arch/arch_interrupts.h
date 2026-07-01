//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#pragma once

#ifndef ASSEMBLY

#include <arch/ops.h>
#include <stdbool.h>
#include <stdint.h>
#include <lk/compiler.h>
#include <arch/sparc.h>

__BEGIN_CDECLS

static inline void arch_enable_ints(void) {
    uint32_t psr = sparc_read_psr();
    psr &= ~SPARC_PSR_PIL_MASK; // Clear PIL bits (enable all interrupts)
    sparc_write_psr(psr);
    CF;
}

static inline void arch_disable_ints(void) {
    uint32_t psr = sparc_read_psr();
    psr |= SPARC_PSR_PIL_MASK;  // Set PIL to 15 (disable all maskable interrupts)
    sparc_write_psr(psr);
    CF;
}

static inline bool arch_ints_disabled(void) {
    uint32_t psr = sparc_read_psr();
    return (psr & SPARC_PSR_PIL_MASK) == SPARC_PSR_PIL_MASK;
}

static inline bool arch_in_int_handler(void) {
    return false;
}

struct arch_interrupt_saved_state {
    uint32_t psr;
};

static inline struct arch_interrupt_saved_state arch_interrupt_save(void) {
    uint32_t psr = sparc_read_psr();
    struct arch_interrupt_saved_state state = { .psr = psr };
    sparc_write_psr(psr | SPARC_PSR_PIL_MASK);
    CF;
    return state;
}

static inline void arch_interrupt_restore(struct arch_interrupt_saved_state old_state) {
    CF;
    uint32_t psr = sparc_read_psr();
    psr &= ~SPARC_PSR_PIL_MASK; // Clear PIL bits
    psr |= (old_state.psr & SPARC_PSR_PIL_MASK);
    sparc_write_psr(psr);
}

__END_CDECLS

#endif // ASSEMBLY
