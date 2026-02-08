/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/ops.h>
#include <assert.h>
#include <lk/compiler.h>
#include <stdbool.h>

__BEGIN_CDECLS

#define SPIN_LOCK_INITIAL_VALUE (0)

typedef unsigned long spin_lock_t;

typedef unsigned int spin_lock_saved_state_t;

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

/* Non-SMP spinlocks are mostly vestigial to try to catch pending locking problems. */
static inline void arch_spin_lock(spin_lock_t *lock) {
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(*lock == 0);
    *lock = 1;
}

static inline int arch_spin_trylock(spin_lock_t *lock) {
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(*lock == 0);
    return 0;
}

static inline void arch_spin_unlock(spin_lock_t *lock) {
    DEBUG_ASSERT(arch_ints_disabled());
    DEBUG_ASSERT(*lock != 0);
    *lock = 0;
}

#endif

#if !(ARM_ISA_ARMV7M || ARM_ISA_ARMV6M)

static inline spin_lock_saved_state_t
arch_interrupt_save(void) {
    spin_lock_saved_state_t state = 0;
    if (!arch_ints_disabled()) {
        state = true;
        arch_disable_ints();
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

    if (old_state) {
        arch_enable_ints();
    }
}

#else

/*
 * slightly more optimized version of the interrupt save/restore bits for cortex-m
 * processors.
 */

__ALWAYS_INLINE
static inline spin_lock_saved_state_t
arch_interrupt_save(void) {
    unsigned int state = 0;

    __asm__ volatile("mrs %0, primask" : "=r"(state));
    /* always disable ints, may be faster than testing and branching around it */
    arch_disable_ints();

    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved before the arch_disable_ints() call.
    CF;

    return state;
}

__ALWAYS_INLINE
static inline void
arch_interrupt_restore(spin_lock_saved_state_t old_state) {
    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved after the arch_enable_ints() call.
    CF;

    /* test the PRIMASK's one bit */
    if ((old_state & 0x1) == 0) {
        arch_enable_ints();
    }
}

#endif

__END_CDECLS
