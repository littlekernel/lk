/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/ops.h>
#include <stdbool.h>

#define SPIN_LOCK_INITIAL_VALUE (0)

typedef volatile unsigned int spin_lock_t;

typedef unsigned long spin_lock_saved_state_t;
typedef unsigned int spin_lock_save_flags_t;

static inline int arch_spin_trylock(spin_lock_t *lock) {
    int tmp = 1, busy;

    __asm__ __volatile__(
        "   amoswap.w %0, %2, %1\n"
        "   fence r , rw\n"
        : "=r"(busy), "+A"(*lock)
        : "r" (tmp)
        : "memory"
    );

    return !busy;
}

static inline void arch_spin_lock(spin_lock_t *lock) {
    while (1) {
        if (*lock) continue;
        if (arch_spin_trylock(lock)) break;
    }
}

static inline void arch_spin_unlock(spin_lock_t *lock) {
    *lock = 0;
}

static inline void arch_spin_lock_init(spin_lock_t *lock) {
    *lock = SPIN_LOCK_INITIAL_VALUE;
}

static inline bool arch_spin_lock_held(spin_lock_t *lock) {
    return *lock != 0;
}

/* default arm flag is to just disable plain irqs */
#define ARCH_DEFAULT_SPIN_LOCK_FLAG_INTERRUPTS  0

static inline void
arch_interrupt_save(spin_lock_saved_state_t *statep, spin_lock_save_flags_t flags) {
    /* disable interrupts by clearing the MIE bit while atomically saving the old state */
    *statep = riscv_csr_read_clear(RISCV_CSR_XSTATUS, RISCV_CSR_XSTATUS_IE) & RISCV_CSR_XSTATUS_IE;
}

static inline void
arch_interrupt_restore(spin_lock_saved_state_t old_state, spin_lock_save_flags_t flags) {
    /* drop the old MIE flag into the status register */
    riscv_csr_set(RISCV_CSR_XSTATUS, old_state);
}

