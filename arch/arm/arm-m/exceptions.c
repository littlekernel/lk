/*
 * Copyright (c) 2012-2013 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <stdio.h>
#include <lk/compiler.h>
#include <stdint.h>
#include <lk/bits.h>
#include <kernel/thread.h>
#include <arch/arm/cm.h>
#include <platform.h>

#pragma GCC diagnostic ignored "-Wmissing-declarations"

static void dump_frame(const struct arm_cm_exception_frame *frame) {

    printf("exception frame at %p\n", frame);
    printf("\tr0  0x%08x r1  0x%08x r2  0x%08x r3 0x%08x r4 0x%08x\n",
           frame->r0, frame->r1, frame->r2, frame->r3, frame->r4);
    printf("\tr5  0x%08x r6  0x%08x r7  0x%08x r8 0x%08x r9 0x%08x\n",
           frame->r5, frame->r6, frame->r7, frame->r8, frame->r9);
    printf("\tr10 0x%08x r11 0x%08x r12 0x%08x\n",
           frame->r10, frame->r11, frame->r12);
    printf("\tlr  0x%08x pc  0x%08x psr 0x%08x\n",
           frame->lr, frame->pc, frame->psr);
}

void hardfault(struct arm_cm_exception_frame *frame) {
    printf("hardfault: ");
    dump_frame(frame);

#if     (__CORTEX_M >= 0X03) || (__CORTEX_SC >= 300)
    printf("HFSR 0x%x\n", SCB->HFSR);
#endif

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

void memmanage(struct arm_cm_exception_frame *frame) {
    printf("memmanage: ");
    dump_frame(frame);

#if     (__CORTEX_M >= 0X03) || (__CORTEX_SC >= 300)
    uint32_t mmfsr = SCB->CFSR & 0xff;

    if (mmfsr & (1<<0)) { // IACCVIOL
        printf("instruction fault\n");
    }
    if (mmfsr & (1<<1)) { // DACCVIOL
        printf("data fault\n");
    }
    if (mmfsr & (1<<3)) { // MUNSTKERR
        printf("fault on exception return\n");
    }
    if (mmfsr & (1<<4)) { // MSTKERR
        printf("fault on exception entry\n");
    }
    if (mmfsr & (1<<5)) { // MLSPERR
        printf("fault on lazy fpu preserve\n");
    }
    if (mmfsr & (1<<7)) { // MMARVALID
        printf("fault address 0x%x\n", SCB->MMFAR);
    }
#endif
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}


void usagefault(struct arm_cm_exception_frame *frame) {
    printf("usagefault: ");
    dump_frame(frame);

#if  (__CORTEX_M >= 0x03)
    uint32_t ufsr = BITS_SHIFT(SCB->CFSR, 31, 16);
    printf("UFSR 0x%x: ", ufsr);

    if (ufsr & (1<<0))
        printf("undefined instruction\n");
    if (ufsr & (1<<1))
        printf("ESPR invalid\n");
    if (ufsr & (1<<2))
        printf("integrity check failed on EXC_RETURN\n");
    if (ufsr & (1<<3))
        printf("coprocessor access error\n");
    if (ufsr & (1<<8))
        printf("unaligned error\n");
    if (ufsr & (1<<9))
        printf("division by zero\n");
#endif

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

void busfault(struct arm_cm_exception_frame *frame) {
    printf("busfault: ");
    dump_frame(frame);

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

/* raw exception vectors */

void _nmi(void) {
    printf("nmi\n");
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

/* Declare two versions of the assembly to push the extra registers
 * not already saved by the exception delivery hardware. For armv6-m
 * based hardware we cannot directly push the higher registers so we
 * need to move them into lower registers before pushing.
 */
#if     (__CORTEX_M >= 0X03) || (__CORTEX_SC >= 300)
#define PUSH_REGS \
        "push   {r4-r11, lr};" /* 9 words on the stack */
#else
#define PUSH_REGS \
        "push   {r4-r7, lr};" /* 5 words */ \
        "mov    r4, r8;" \
        "mov    r5, r9;" \
        "mov    r6, r10;" \
        "mov    r7, r11;" \
        "push   {r4-r7};" /* 4 more words */
#endif

__NAKED void _hardfault(void) {
    __asm__ volatile(
        PUSH_REGS
        "mov    r0, sp;"
        "sub    sp, #4;" /* adjust the stack to be 8 byte aligned */
        "b      hardfault;"
    );
}

__NAKED void _memmanage(void) {
    __asm__ volatile(
        PUSH_REGS
        "mov    r0, sp;"
        "sub    sp, #4;" /* adjust the stack to be 8 byte aligned */
        "b      memmanage;"
    );
}

__NAKED void _busfault(void) {
    __asm__ volatile(
        PUSH_REGS
        "mov    r0, sp;"
        "sub    sp, #4;" /* adjust the stack to be 8 byte aligned */
        "b      busfault;"
    );
}

__NAKED void _usagefault(void) {
    __asm__ volatile(
        PUSH_REGS
        "mov    r0, sp;"
        "sub    sp, #4;" /* adjust the stack to be 8 byte aligned */
        "b      usagefault;"
    );
}

#undef PUSH_REGS

/* declared weak so these can be overridden elsewhere */

/* systick handler */
void __WEAK _systick(void) {
    printf("systick\n");
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

void __WEAK _debugmonitor(void) {
    printf("debugmonitor\n");
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

void __WEAK _svc(void) {
    printf("svc\n");
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}
