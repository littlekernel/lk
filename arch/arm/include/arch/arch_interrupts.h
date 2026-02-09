/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#ifndef ASSEMBLY

#include <arch/ops.h>
#include <stdbool.h>
#include <lk/compiler.h>
#include <lk/reg.h>
#include <arch/arm.h>

#if ARM_ISA_ARMV7M || ARM_ISA_ARMV6M
#include <arch/arm/cm.h>
#endif

__BEGIN_CDECLS

#if ARM_ISA_ARMV7 || (ARM_ISA_ARMV6 && !__thumb__)

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

#elif ARM_ISA_ARMV6M // cortex-m0 cortex-m0+

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

static inline bool arch_in_int_handler(void) {
    uint32_t ipsr;
    __asm volatile ("MRS %0, ipsr" : "=r" (ipsr) );
    return (ipsr & IPSR_ISR_Msk);
}

#else // pre-armv6 || (armv6 & thumb)

#error pre-armv6 or armv6 + thumb unimplemented

#endif

struct arch_interrupt_saved_state {
    unsigned int state;
};

#if !(ARM_ISA_ARMV7M || ARM_ISA_ARMV6M)

static inline struct arch_interrupt_saved_state
arch_interrupt_save(void) {
    struct arch_interrupt_saved_state state = { .state = 0 };
    if (!arch_ints_disabled()) {
        state.state = 1;
        arch_disable_ints();
    }

    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved before the arch_disable_ints() call.
    CF;

    return state;
}

static inline void
arch_interrupt_restore(struct arch_interrupt_saved_state old_state) {
    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved after the arch_enable_ints() call.
    CF;

    if (old_state.state) {
        arch_enable_ints();
    }
}

#else

/*
 * slightly more optimized version of the interrupt save/restore bits for cortex-m
 * processors.
 */

__ALWAYS_INLINE
static inline struct arch_interrupt_saved_state
arch_interrupt_save(void) {
    struct arch_interrupt_saved_state state = { .state = 0 };

    __asm__ volatile("mrs %0, primask" : "=r"(state.state));
    /* always disable ints, may be faster than testing and branching around it */
    arch_disable_ints();

    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved before the arch_disable_ints() call.
    CF;

    return state;
}

__ALWAYS_INLINE
static inline void
arch_interrupt_restore(struct arch_interrupt_saved_state old_state) {
    // Insert a compiler fence to make sure all code that needs to run with
    // interrupts disabled is not moved after the arch_enable_ints() call.
    CF;

    /* test the PRIMASK's one bit */
    if ((old_state.state & 0x1) == 0) {
        arch_enable_ints();
    }
}

#endif

__END_CDECLS

#endif // ASSEMBLY
