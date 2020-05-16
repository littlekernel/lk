/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/ops.h>

// define simple implementations of the atomic routines for these cpus
// that do not otherwise have an implementation.
#if !USE_BUILTIN_ATOMICS
#if ARM_ISA_ARMV6M // cortex-m0 cortex-m0+

static inline int atomic_add(volatile int *ptr, int val) {
    int temp;
    bool state;

    state = arch_ints_disabled();
    arch_disable_ints();
    temp = *ptr;
    *ptr = temp + val;
    if (!state)
        arch_enable_ints();
    return temp;
}

static inline  int atomic_and(volatile int *ptr, int val) {
    int temp;
    bool state;

    state = arch_ints_disabled();
    arch_disable_ints();
    temp = *ptr;
    *ptr = temp & val;
    if (!state)
        arch_enable_ints();
    return temp;
}

static inline int atomic_or(volatile int *ptr, int val) {
    int temp;
    bool state;

    state = arch_ints_disabled();
    arch_disable_ints();
    temp = *ptr;
    *ptr = temp | val;
    if (!state)
        arch_enable_ints();
    return temp;
}

static inline int atomic_swap(volatile int *ptr, int val) {
    int temp;
    bool state;

    state = arch_ints_disabled();
    arch_disable_ints();
    temp = *ptr;
    *ptr = val;
    if (!state)
        arch_enable_ints();
    return temp;
}

static inline int atomic_cmpxchg(volatile int *ptr, int oldval, int newval) {
    int temp;
    bool state;

    state = arch_ints_disabled();
    arch_disable_ints();
    temp = *ptr;
    if (temp == oldval) {
        *ptr = newval;
    }
    if (!state)
        arch_enable_ints();
    return temp;
}

#endif // ARM_ISA_ARMV6M
#endif // !USE_BUILTIN_ATOMICS
