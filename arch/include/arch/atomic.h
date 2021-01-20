/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <stdbool.h>

__BEGIN_CDECLS

/* use built in atomic intrinsics if the architecture doesn't otherwise
 * override it. */
#if !defined(USE_BUILTIN_ATOMICS) || USE_BUILTIN_ATOMICS
static inline int atomic_add(volatile int *ptr, int val) {
    return __atomic_fetch_add(ptr, val, __ATOMIC_RELAXED);
}

static inline int atomic_or(volatile int *ptr, int val) {
    return __atomic_fetch_or(ptr, val, __ATOMIC_RELAXED);
}

static inline int atomic_and(volatile int *ptr, int val) {
    return __atomic_fetch_and(ptr, val, __ATOMIC_RELAXED);
}

static inline int atomic_swap(volatile int *ptr, int val) {
    return __atomic_exchange_n(ptr, val, __ATOMIC_RELAXED);
}
static inline int atomic_cmpxchg(volatile int *ptr, int oldval, int newval) {
    // TODO: implement
    return 0;
}

#else
static int atomic_swap(volatile int *ptr, int val);
static int atomic_add(volatile int *ptr, int val);
static int atomic_and(volatile int *ptr, int val);
static int atomic_or(volatile int *ptr, int val);

/* if an implementation wants to implement it themselves */
#include <arch/arch_atomic.h>

#endif

__END_CDECLS

