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
#include <arch/riscv/csr.h>

__BEGIN_CDECLS

static inline void arch_enable_ints(void) {
    riscv_csr_set(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_IE);
}

static inline void arch_disable_ints(void) {
    riscv_csr_clear(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_IE);
}

static inline bool arch_ints_disabled(void) {
    return !(riscv_csr_read(RISCV_CSR_XSTATUS) & RISCV_CSR_XSTATUS_IE);
}

static inline bool arch_in_int_handler(void) {
    return false;
}

struct spin_lock_saved_state {
    unsigned long flags;
};

static inline struct spin_lock_saved_state
arch_interrupt_save(void) {
    struct spin_lock_saved_state state = {
        .flags = riscv_csr_read_clear(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_IE) &
                 RISCV_CSR_XSTATUS_IE
    };

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

    // drop the old MIE flag into the status register
    riscv_csr_set(RISCV_CSR_XSTATUS, old_state.flags);
}

__END_CDECLS

#endif // ASSEMBLY
