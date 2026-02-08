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

static inline spin_lock_saved_state_t
arch_interrupt_save(void) {
    spin_lock_saved_state_t state = x86_save_flags();
    if (state & X86_FLAGS_IF) {
        x86_cli();
    }

    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved before the arch_disable_ints() call.
    CF;

    return state;
}

static inline void
arch_interrupt_restore(spin_lock_saved_state_t old_state) {
    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved after the arch_enable_ints() call.
    CF;

    if (old_state & X86_FLAGS_IF) {
        x86_sti();
    }
}

__END_CDECLS
