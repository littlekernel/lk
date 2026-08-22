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

#if WITH_SMP
// The contended path, a wfe loop in spinlock.S.
void arch_spin_lock_contended(spin_lock_t *lock);

// The uncontended acquire and the release are inlined: a lock operation is a
// single ldaxr/stxr pair or a single stlr, and the call and return around it
// were costing as much as the operation itself at the thousand or so sites in
// the kernel. The inline attempt is the same ldaxr/stxr sequence the assembly
// used, with the same acquire semantics; the release is a single stlr.
// One ldaxr/stxr attempt. Returns 0 if the lock was taken. Written as asm
// rather than __atomic_compare_exchange_n because gcc outlines that into a
// call (-moutline-atomics, for LSE dispatch at runtime), which would put the
// call back.
static inline int arch_spin_lock_try_once(spin_lock_t *lock) {
    spin_lock_t old;
    uint32_t fail;
    __asm__ volatile(
        "mov    %w1, #1\n"
        "ldaxr  %0, [%2]\n"
        "cbnz   %0, 1f\n"
        "stxr   %w1, %3, [%2]\n"
        "1:"
        : "=&r"(old), "=&r"(fail)
        : "r"(lock), "r"(1UL)
        : "memory");
    return (old | fail) != 0;
}

static inline void arch_spin_lock(spin_lock_t *lock) {
    if (likely(arch_spin_lock_try_once(lock) == 0)) {
        return;
    }
    arch_spin_lock_contended(lock);
}

static inline int arch_spin_trylock(spin_lock_t *lock) {
    return arch_spin_lock_try_once(lock);
}

static inline void arch_spin_unlock(spin_lock_t *lock) {
    __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
}
#else
static inline void arch_spin_lock(spin_lock_t *lock) {
    *lock = 1;
}

static inline int arch_spin_trylock(spin_lock_t *lock) {
    *lock = 1;
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

__END_CDECLS
