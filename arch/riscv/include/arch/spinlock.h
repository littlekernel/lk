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
#include <lk/compiler.h>

__BEGIN_CDECLS

#define SPIN_LOCK_INITIAL_VALUE (0)

typedef volatile uint32_t spin_lock_t;


void riscv_spin_lock(spin_lock_t *lock);
void riscv_spin_unlock(spin_lock_t *lock);
int riscv_spin_trylock(spin_lock_t *lock);

static inline int arch_spin_trylock(spin_lock_t *lock) {
    return riscv_spin_trylock(lock);
}

static inline void arch_spin_lock(spin_lock_t *lock) {
    riscv_spin_lock(lock);
}

static inline void arch_spin_unlock(spin_lock_t *lock) {
    riscv_spin_unlock(lock);
}

static inline void arch_spin_lock_init(spin_lock_t *lock) {
    *lock = SPIN_LOCK_INITIAL_VALUE;
}

static inline bool arch_spin_lock_held(spin_lock_t *lock) {
    return *lock != 0;
}


__END_CDECLS
