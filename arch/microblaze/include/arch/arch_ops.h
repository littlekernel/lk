/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <compiler.h>

#define USE_MSRSET 1

static inline void arch_enable_ints(void)
{
    CF;
    uint32_t temp;
    __asm__ volatile(
#if USE_MSRSET
        "msrset %0, (1<<1)"
#else
        "mfs    %0, rmsr;"
        "ori    %0, %0, (1<<1);"
        "mts    rmsr, %0"
#endif
        : "=r" (temp));
}

static inline void arch_disable_ints(void)
{
    uint32_t temp;
    __asm__ volatile(
#if USE_MSRSET
        "msrclr %0, (1<<1)"
#else
        "mfs    %0, rmsr;"
        "andni  %0, %0, (1<<1);"
        "mts    rmsr, %0"
#endif
        : "=r" (temp));
    CF;
}

static inline bool arch_ints_disabled(void)
{
    uint32_t state;

    __asm__ volatile(
        "mfs    %0, rmsr;"
        : "=r" (state));

    return !(state & (1<<1));
}

static inline int atomic_add(volatile int *ptr, int val)
{
    return __atomic_fetch_add(ptr, val, __ATOMIC_RELAXED);
}

static inline int atomic_or(volatile int *ptr, int val)
{
    return __atomic_fetch_or(ptr, val, __ATOMIC_RELAXED);
}

static inline int atomic_and(volatile int *ptr, int val)
{
    return __atomic_fetch_and(ptr, val, __ATOMIC_RELAXED);
}

static inline int atomic_swap(volatile int *ptr, int val)
{
    return __atomic_exchange_n(ptr, val, __ATOMIC_RELAXED);
}

/* use a global pointer to store the current_thread */
extern struct thread *_current_thread;

static inline struct thread *get_current_thread(void)
{
    return _current_thread;
}

static inline void set_current_thread(struct thread *t)
{
    _current_thread = t;
}

static inline uint32_t arch_cycle_count(void) { return 0; }

static inline uint arch_curr_cpu_num(void)
{
    return 0;
}

