/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/interrupts.h>
#include <assert.h>
#include <lk/compiler.h>
#include <stdbool.h>

__BEGIN_CDECLS

#define SPIN_LOCK_INITIAL_VALUE (0)

typedef unsigned long spin_lock_t;


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

__END_CDECLS
