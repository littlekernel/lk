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
#error m68k does not support SMP
#endif

#define SPIN_LOCK_INITIAL_VALUE (0)

typedef unsigned int spin_lock_t;

typedef unsigned int spin_lock_saved_state_t;

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

static inline spin_lock_saved_state_t
arch_interrupt_save(void) {
    spin_lock_saved_state_t statep = arch_ints_disabled();
    arch_disable_ints();
    return statep;
}

static inline void
arch_interrupt_restore(spin_lock_saved_state_t old_state) {
    if (!old_state) {
        arch_enable_ints();
    }
}




