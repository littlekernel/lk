/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#ifndef ASSEMBLY

#include <stdbool.h>
#include <lk/compiler.h>
#include <lk/reg.h>
#include <arch/arm.h>

#if ARM_ISA_ARMV7M
#include <arch/arm/cm.h>
#endif

__BEGIN_CDECLS

#if ARM_ISA_ARMV7 || (ARM_ISA_ARMV6 && !__thumb__)
#define ENABLE_CYCLE_COUNTER 1

// override of some routines
static inline void arch_enable_ints(void) {
    CF;
    __asm__ volatile("cpsie i");
}

static inline void arch_disable_ints(void) {
    __asm__ volatile("cpsid i");
    CF;
}

static inline bool arch_ints_disabled(void) {
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

static inline void arch_enable_fiqs(void) {
    CF;
    __asm__ volatile("cpsie f");
}

static inline void arch_disable_fiqs(void) {
    __asm__ volatile("cpsid f");
    CF;
}

static inline bool arch_fiqs_disabled(void) {
    unsigned int state;

    __asm__ volatile("mrs %0, cpsr" : "=r"(state));
    state &= (1<<6);

    return !!state;
}

static inline bool arch_in_int_handler(void) {
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

static inline uint32_t arch_cycle_count(void) {
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
static inline uint arch_curr_cpu_num(void) {
    uint32_t mpidr = arm_read_mpidr();
    return ((mpidr & ((1U << SMP_CPU_ID_BITS) - 1)) >> 8 << SMP_CPU_CLUSTER_SHIFT) | (mpidr & 0xff);
}
#else
static inline uint arch_curr_cpu_num(void) {
    return 0;
}
#endif

/* defined in kernel/thread.h */

#if !ARM_ISA_ARMV7M
/* use the cpu local thread context pointer to store current_thread */
static inline struct thread *arch_get_current_thread(void) {
    return (struct thread *)arm_read_tpidrprw();
}

static inline void arch_set_current_thread(struct thread *t) {
    arm_write_tpidrprw((uint32_t)t);
}
#else // ARM_ISA_ARM7M

/* use a global pointer to store the current_thread */
extern struct thread *_current_thread;

static inline struct thread *arch_get_current_thread(void) {
    return _current_thread;
}

static inline void arch_set_current_thread(struct thread *t) {
    _current_thread = t;
}

#endif // !ARM_ISA_ARMV7M

#elif ARM_ISA_ARMV6M // cortex-m0 cortex-m0+


static inline void arch_enable_fiqs(void) {
    CF;
    __asm__ volatile("cpsie f");
}

static inline void arch_disable_fiqs(void) {
    __asm__ volatile("cpsid f");
    CF;
}

static inline bool arch_fiqs_disabled(void) {
    unsigned int state;

    __asm__ volatile("mrs %0, cpsr" : "=r"(state));
    state &= (1<<6);

    return !!state;
}



static inline void arch_enable_ints(void) {
    CF;
    __asm__ volatile("cpsie i");
}
static inline void arch_disable_ints(void) {
    __asm__ volatile("cpsid i");
    CF;
}

static inline bool arch_ints_disabled(void) {
    unsigned int state;

    __asm__ volatile("mrs %0, primask" : "=r"(state));
    state &= 0x1;
    return !!state;
}

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

static inline uint32_t arch_cycle_count(void) {
    return 0;
}

static inline uint arch_curr_cpu_num(void) {
    return 0;
}

/* use a global pointer to store the current_thread */
extern struct thread *_current_thread;

static inline struct thread *arch_get_current_thread(void) {
    return _current_thread;
}

static inline void arch_set_current_thread(struct thread *t) {
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

__END_CDECLS

#endif // ASSEMBLY
