#pragma once

#include <arch/interrupts.h>

__BEGIN_CDECLS

static inline int atomic_add(volatile int *ptr, int val) {
    int temp;
    bool state = arch_ints_disabled();
    arch_disable_ints();
    temp = *ptr;
    *ptr = temp + val;
    if (!state)
        arch_enable_ints();
    return temp;
}

static inline int atomic_and(volatile int *ptr, int val) {
    int temp;
    bool state = arch_ints_disabled();
    arch_disable_ints();
    temp = *ptr;
    *ptr = temp & val;
    if (!state)
        arch_enable_ints();
    return temp;
}

static inline int atomic_or(volatile int *ptr, int val) {
    int temp;
    bool state = arch_ints_disabled();
    arch_disable_ints();
    temp = *ptr;
    *ptr = temp | val;
    if (!state)
        arch_enable_ints();
    return temp;
}

static inline int atomic_swap(volatile int *ptr, int val) {
    int temp;
    bool state = arch_ints_disabled();
    arch_disable_ints();
    temp = *ptr;
    *ptr = val;
    if (!state)
        arch_enable_ints();
    return temp;
}

static inline int atomic_cmpxchg(volatile int *ptr, int oldval, int newval) {
    int temp;
    bool state = arch_ints_disabled();
    arch_disable_ints();
    temp = *ptr;
    if (temp == oldval) {
        *ptr = newval;
    }
    if (!state)
        arch_enable_ints();
    return temp;
}

__END_CDECLS
