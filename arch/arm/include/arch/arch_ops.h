/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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

#ifndef ASSEMBLY

#include <stdbool.h>
#include <compiler.h>
#include <reg.h>
#include <arch/arm.h>

#if ARM_ISA_ARMV7M
#include <arch/arm/cm.h>
#endif

__BEGIN_CDECLS;

#if ARM_ISA_ARMV7 || (ARM_ISA_ARMV6 && !__thumb__)
#define USE_GCC_ATOMICS 0
#define ENABLE_CYCLE_COUNTER 1

// override of some routines
static inline void arch_enable_ints(void)
{
    CF;
    __asm__ volatile("cpsie i");
}

static inline void arch_disable_ints(void)
{
    __asm__ volatile("cpsid i");
    CF;
}

static inline bool arch_ints_disabled(void)
{
    unsigned int state;

#if ARM_ISA_ARMV7M
    __asm__ volatile("mrs %0, primask" : "=r"(state));
    state &= 0x1;
#else
    __asm__ volatile("mrs %0, cpsr" : "=r"(state));
    state &= (1<<7);
#endif

    return !!state;
}

static inline void arch_enable_fiqs(void)
{
    CF;
    __asm__ volatile("cpsie f");
}

static inline void arch_disable_fiqs(void)
{
    __asm__ volatile("cpsid f");
    CF;
}

static inline bool arch_fiqs_disabled(void)
{
    unsigned int state;

    __asm__ volatile("mrs %0, cpsr" : "=r"(state));
    state &= (1<<6);

    return !!state;
}

static inline bool arch_in_int_handler(void)
{
#if ARM_ISA_ARMV7M
    uint32_t ipsr;
    __asm volatile ("MRS %0, ipsr" : "=r" (ipsr) );
    return (ipsr & IPSR_ISR_Msk);
#else
    /* set by the interrupt glue to track that the cpu is inside a handler */
    extern bool __arm_in_handler;

    return __arm_in_handler;
#endif
}

static inline int atomic_add(volatile int *ptr, int val)
{
#if USE_GCC_ATOMICS
    return __atomic_fetch_add(ptr, val, __ATOMIC_RELAXED);
#else
    int old;
    int temp;
    int test;

    do {
        __asm__ volatile(
            "ldrex	%[old], [%[ptr]]\n"
            "adds	%[temp], %[old], %[val]\n"
            "strex	%[test], %[temp], [%[ptr]]\n"
            : [old]"=&r" (old), [temp]"=&r" (temp), [test]"=&r" (test)
            : [ptr]"r" (ptr), [val]"r" (val)
            : "memory", "cc");

    } while (test != 0);

    return old;
#endif
}

static inline int atomic_or(volatile int *ptr, int val)
{
#if USE_GCC_ATOMICS
    return __atomic_fetch_or(ptr, val, __ATOMIC_RELAXED);
#else
    int old;
    int temp;
    int test;

    do {
        __asm__ volatile(
            "ldrex	%[old], [%[ptr]]\n"
            "orrs	%[temp], %[old], %[val]\n"
            "strex	%[test], %[temp], [%[ptr]]\n"
            : [old]"=&r" (old), [temp]"=&r" (temp), [test]"=&r" (test)
            : [ptr]"r" (ptr), [val]"r" (val)
            : "memory", "cc");

    } while (test != 0);

    return old;
#endif
}

static inline int atomic_and(volatile int *ptr, int val)
{
#if USE_GCC_ATOMICS
    return __atomic_fetch_and(ptr, val, __ATOMIC_RELAXED);
#else
    int old;
    int temp;
    int test;

    do {
        __asm__ volatile(
            "ldrex	%[old], [%[ptr]]\n"
            "ands	%[temp], %[old], %[val]\n"
            "strex	%[test], %[temp], [%[ptr]]\n"
            : [old]"=&r" (old), [temp]"=&r" (temp), [test]"=&r" (test)
            : [ptr]"r" (ptr), [val]"r" (val)
            : "memory", "cc");

    } while (test != 0);

    return old;
#endif
}

static inline int atomic_swap(volatile int *ptr, int val)
{
#if USE_GCC_ATOMICS
    return __atomic_exchange_n(ptr, val, __ATOMIC_RELAXED);
#else
    int old;
    int test;

    do {
        __asm__ volatile(
            "ldrex	%[old], [%[ptr]]\n"
            "strex	%[test], %[val], [%[ptr]]\n"
            : [old]"=&r" (old), [test]"=&r" (test)
            : [ptr]"r" (ptr), [val]"r" (val)
            : "memory");

    } while (test != 0);

    return old;
#endif
}

static inline int atomic_cmpxchg(volatile int *ptr, int oldval, int newval)
{
    int old;
    int test;

    do {
        __asm__ volatile(
            "ldrex	%[old], [%[ptr]]\n"
            "mov	%[test], #0\n"
            "teq	%[old], %[oldval]\n"
#if (ARM_ISA_ARMV7M || __thumb__)
            "bne	0f\n"
            "strex	%[test], %[newval], [%[ptr]]\n"
            "0:\n"
#else
            "strexeq %[test], %[newval], [%[ptr]]\n"
#endif
            : [old]"=&r" (old), [test]"=&r" (test)
            : [ptr]"r" (ptr), [oldval]"Ir" (oldval), [newval]"r" (newval)
            : "cc");

    } while (test != 0);

    return old;
}

static inline uint32_t arch_cycle_count(void)
{
#if ARM_ISA_ARMV7M
#if ENABLE_CYCLE_COUNTER
#define DWT_CYCCNT (0xE0001004)
    return *REG32(DWT_CYCCNT);
#else
    return 0;
#endif
#elif ARM_ISA_ARMV7
    uint32_t count;
    __asm__ volatile("mrc		p15, 0, %0, c9, c13, 0"
                     : "=r" (count)
                    );
    return count;
#else
//#warning no arch_cycle_count implementation
    return 0;
#endif
}

#if WITH_SMP && ARM_ISA_ARMV7
static inline uint arch_curr_cpu_num(void)
{
    uint32_t mpidr = arm_read_mpidr();
    return ((mpidr & ((1U << SMP_CPU_ID_BITS) - 1)) >> 8 << SMP_CPU_CLUSTER_SHIFT) | (mpidr & 0xff);
}
#else
static inline uint arch_curr_cpu_num(void)
{
    return 0;
}
#endif

/* defined in kernel/thread.h */

#if !ARM_ISA_ARMV7M
/* use the cpu local thread context pointer to store current_thread */
static inline struct thread *get_current_thread(void)
{
    return (struct thread *)arm_read_tpidrprw();
}

static inline void set_current_thread(struct thread *t)
{
    arm_write_tpidrprw((uint32_t)t);
}
#else // ARM_ISA_ARM7M

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

#endif // !ARM_ISA_ARMV7M

#elif ARM_ISA_ARMV6M // cortex-m0 cortex-m0+


static inline void arch_enable_fiqs(void)
{
    CF;
    __asm__ volatile("cpsie f");
}

static inline void arch_disable_fiqs(void)
{
    __asm__ volatile("cpsid f");
    CF;
}

static inline bool arch_fiqs_disabled(void)
{
    unsigned int state;

    __asm__ volatile("mrs %0, cpsr" : "=r"(state));
    state &= (1<<6);

    return !!state;
}



static inline void arch_enable_ints(void)
{
    CF;
    __asm__ volatile("cpsie i");
}
static inline void arch_disable_ints(void)
{
    __asm__ volatile("cpsid i");
    CF;
}

static inline bool arch_ints_disabled(void)
{
    unsigned int state;

    __asm__ volatile("mrs %0, primask" : "=r"(state));
    state &= 0x1;
    return !!state;
}

static inline int atomic_add(volatile int *ptr, int val)
{
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

static inline  int atomic_and(volatile int *ptr, int val)
{
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

static inline int atomic_or(volatile int *ptr, int val)
{
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

static inline int atomic_swap(volatile int *ptr, int val)
{
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

static inline int atomic_cmpxchg(volatile int *ptr, int oldval, int newval)
{
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

static inline uint32_t arch_cycle_count(void)
{
    return 0;
}

static inline uint arch_curr_cpu_num(void)
{
    return 0;
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

#else // pre-armv6 || (armv6 & thumb)

/* for pre-armv6 the bodies of these are too big to inline, call an assembly stub version */
void _arch_enable_ints(void);
void _arch_disable_ints(void);

int _atomic_add(volatile int *ptr, int val);
int _atomic_and(volatile int *ptr, int val);
int _atomic_or(volatile int *ptr, int val);
int _atomic_add(volatile int *ptr, int val);
int _atomic_swap(volatile int *ptr, int val);
int _atomic_cmpxchg(volatile int *ptr, int oldval, int newval);

uint32_t _arch_cycle_count(void);

static inline int atomic_add(volatile int *ptr, int val) { return _atomic_add(ptr, val); }
static inline int atomic_and(volatile int *ptr, int val) { return _atomic_and(ptr, val); }
static inline int atomic_or(volatile int *ptr, int val) { return _atomic_or(ptr, val); }
static inline int atomic_swap(volatile int *ptr, int val) { return _atomic_swap(ptr, val); }
static inline int atomic_cmpxchg(volatile int *ptr, int oldval, int newval) { return _atomic_cmpxchg(ptr, oldval, newval); }

static inline void arch_enable_ints(void) { _arch_enable_ints(); }
static inline void arch_disable_ints(void) { _arch_disable_ints(); }

static inline uint32_t arch_cycle_count(void) { return _arch_cycle_count(); }

#endif

#define mb()        DSB
#define wmb()       DSB
#define rmb()       DSB

#ifdef WITH_SMP
#define smp_mb()    DMB
#define smp_wmb()   DMB
#define smp_rmb()   DMB
#else
#define smp_mb()    CF
#define smp_wmb()   CF
#define smp_rmb()   CF
#endif

__END_CDECLS;

#endif // ASSEMBLY
