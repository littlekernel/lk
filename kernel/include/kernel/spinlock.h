/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/spinlock.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

/* interrupts should already be disabled */
static inline void spin_lock(spin_lock_t *lock) {
    arch_spin_lock(lock);
}

/* Returns 0 on success, non-0 on failure */
static inline int spin_trylock(spin_lock_t *lock) {
    return arch_spin_trylock(lock);
}

/* interrupts should already be disabled */
static inline void spin_unlock(spin_lock_t *lock) {
    arch_spin_unlock(lock);
}

static inline void spin_lock_init(spin_lock_t *lock) {
    arch_spin_lock_init(lock);
}

static inline bool spin_lock_held(spin_lock_t *lock) {
    return arch_spin_lock_held(lock);
}

/* flags to the arch_interrupt_save routine */
#define SPIN_LOCK_FLAG_INTERRUPTS ARCH_DEFAULT_SPIN_LOCK_FLAG_INTERRUPTS

/* spin lock saved state is just an alias to the arch interrupt saved state */
typedef arch_interrupt_save_state_t spin_lock_saved_state_t;

/* same as spin lock, but save disable and save interrupt state first */
static inline void spin_lock_save(
    spin_lock_t *lock,
    spin_lock_saved_state_t *statep,
    arch_interrupt_save_flags_t flags) {

    *statep = arch_interrupt_save(flags);
    spin_lock(lock);
}

/* restore interrupt state before unlocking */
static inline void spin_unlock_restore(
    spin_lock_t *lock,
    spin_lock_saved_state_t old_state,
    arch_interrupt_save_flags_t flags) {
    spin_unlock(lock);
    arch_interrupt_restore(old_state, flags);
}

/* hand(ier) routines */
#define spin_lock_irqsave(lock, statep) spin_lock_save(lock, &(statep), SPIN_LOCK_FLAG_INTERRUPTS)
#define spin_unlock_irqrestore(lock, statep) spin_unlock_restore(lock, statep, SPIN_LOCK_FLAG_INTERRUPTS)

__END_CDECLS
