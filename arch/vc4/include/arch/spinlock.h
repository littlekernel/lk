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

#if WITH_SMP
#error vc4 does not support SMP
#endif

#define SPIN_LOCK_INITIAL_VALUE (0)

typedef unsigned int spin_lock_t;

typedef unsigned int spin_lock_saved_state_t;
typedef unsigned int spin_lock_save_flags_t;

static inline void arch_spin_lock(spin_lock_t *lock) {
    *lock = 1;
}

static inline int arch_spin_trylock(spin_lock_t *lock) {
    return 0;
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

enum {
    /* private */
    SPIN_LOCK_STATE_RESTORE_IRQ = 1,
};

static inline void
arch_interrupt_save(spin_lock_saved_state_t *statep, spin_lock_save_flags_t flags) {
    spin_lock_saved_state_t state = 0;
    if (!arch_ints_disabled()) {
        state |= SPIN_LOCK_STATE_RESTORE_IRQ;
        arch_disable_ints();
    }
    *statep = state;
}

static inline void
arch_interrupt_restore(spin_lock_saved_state_t old_state, spin_lock_save_flags_t flags) {
    if (old_state & SPIN_LOCK_STATE_RESTORE_IRQ) {
        arch_enable_ints();
    }
}




