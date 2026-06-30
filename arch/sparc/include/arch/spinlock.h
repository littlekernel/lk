#pragma once

#include <arch/ops.h>
#include <stdbool.h>

#if WITH_SMP
#error sparc does not support SMP in this stub
#endif

#define SPIN_LOCK_INITIAL_VALUE (0)

typedef unsigned int spin_lock_t;

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
