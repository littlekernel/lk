/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <arch/ops.h>
#include <stdbool.h>

__BEGIN_CDECLS

#define SPIN_LOCK_INITIAL_VALUE (0)

typedef unsigned long spin_lock_t;

typedef bool spin_lock_saved_state_t;
typedef unsigned int spin_lock_save_flags_t;

#if WITH_SMP
void arch_spin_lock(spin_lock_t *lock);
int arch_spin_trylock(spin_lock_t *lock);
void arch_spin_unlock(spin_lock_t *lock);
#else
static inline void arch_spin_lock(spin_lock_t *lock) {
    *lock = 1;
}

static inline int arch_spin_trylock(spin_lock_t *lock) {
    return 0;
}

static inline void arch_spin_unlock(spin_lock_t *lock) {
    *lock = 0;
}
#endif

static inline void arch_spin_lock_init(spin_lock_t *lock) {
    *lock = SPIN_LOCK_INITIAL_VALUE;
}

static inline bool arch_spin_lock_held(spin_lock_t *lock) {
    return *lock != 0;
}

static inline spin_lock_saved_state_t
arch_interrupt_save(void) {
    spin_lock_saved_state_t state = false;
    if (!arch_ints_disabled()) {
        state = true;
        arch_disable_ints();
    }

    CF;

    return state;
}

static inline void
arch_interrupt_restore(spin_lock_saved_state_t old_state) {
    CF;

    if (old_state) {
        arch_enable_ints();
    }
}

__END_CDECLS
