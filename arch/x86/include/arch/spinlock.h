/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <arch/ops.h>
#include <arch/x86.h>
#include <stdbool.h>

#define SPIN_LOCK_INITIAL_VALUE (0)

__BEGIN_CDECLS

typedef unsigned int spin_lock_t;

typedef x86_flags_t spin_lock_saved_state_t;
typedef uint spin_lock_save_flags_t;

/* simple implementation of spinlocks for no smp support */
static inline void arch_spin_lock_init(spin_lock_t *lock) {
    *lock = SPIN_LOCK_INITIAL_VALUE;
}

static inline bool arch_spin_lock_held(spin_lock_t *lock) {
    return *lock != 0;
}

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

/* flags are unused on x86 */
#define ARCH_DEFAULT_SPIN_LOCK_FLAG_INTERRUPTS  0

static inline void
arch_interrupt_save(spin_lock_saved_state_t *statep, spin_lock_save_flags_t flags) {
    *statep = x86_save_flags();
    arch_disable_ints();
}

static inline void
arch_interrupt_restore(spin_lock_saved_state_t old_state, spin_lock_save_flags_t flags) {
    x86_restore_flags(old_state);
}

__END_CDECLS
