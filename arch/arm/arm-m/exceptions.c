/*
 * Copyright (c) 2012-2013 Travis Geiselbrecht
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
#include <debug.h>
#include <stdio.h>
#include <compiler.h>
#include <stdint.h>
#include <bits.h>
#include <kernel/thread.h>
#include <arch/arm/cm.h>
#include <platform.h>

static void dump_frame(const struct arm_cm_exception_frame *frame)
{

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

static void hardfault(struct arm_cm_exception_frame *frame)
{
    printf("hardfault: ");
    dump_frame(frame);

#if     (__CORTEX_M >= 0X03) || (__CORTEX_SC >= 300)
    printf("HFSR 0x%x\n", SCB->HFSR);
#endif

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

static void memmanage(struct arm_cm_exception_frame *frame)
{
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


static void usagefault(struct arm_cm_exception_frame *frame)
{
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

static void busfault(struct arm_cm_exception_frame *frame)
{
    printf("busfault: ");
    dump_frame(frame);

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

/* raw exception vectors */

void _nmi(void)
{
    printf("nmi\n");
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}
#if     (__CORTEX_M >= 0X03) || (__CORTEX_SC >= 300)

__NAKED void _hardfault(void)
{
    __asm__ volatile(
        "push	{r4-r11};"
        "mov	r0, sp;"
        "b		%0;"
        :: "i" (hardfault)
    );
    __UNREACHABLE;
}

void _memmanage(void)
{
    __asm__ volatile(
        "push	{r4-r11};"
        "mov	r0, sp;"
        "b		%0;"
        :: "i" (memmanage)
    );
    __UNREACHABLE;
}

void _busfault(void)
{
    __asm__ volatile(
        "push	{r4-r11};"
        "mov	r0, sp;"
        "b		%0;"
        :: "i" (busfault)
    );
    __UNREACHABLE;
}

void _usagefault(void)
{
    __asm__ volatile(
        "push	{r4-r11};"
        "mov	r0, sp;"
        "b		%0;"
        :: "i" (usagefault)
    );
    __UNREACHABLE;
}
#else

__NAKED void _hardfault(void)
{
    struct arm_cm_exception_frame *frame;
    __asm__ volatile(
        "push	{r4-r7};"
        "mov   r4, r8;"
        "mov   r5, r9;"
        "mov   r6, r10;"
        "mov   r7, r11;"
        "push   {r4-r7};"
        "mov	%0, sp;"
        : "=r" (frame):
    );

    printf("hardfault: ");
    dump_frame(frame);

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
    __UNREACHABLE;
}

void _memmanage(void)
{
    struct arm_cm_exception_frame *frame;
    __asm__ volatile(
        "push	{r4-r7};"
        "mov   r4, r8;"
        "mov   r5, r9;"
        "mov   r6, r10;"
        "mov   r7, r11;"
        "push   {r4-r7};"
        "mov	%0, sp;"
        : "=r" (frame):
    );
    printf("memmanage: ");
    dump_frame(frame);

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
    __UNREACHABLE;
}

void _busfault(void)
{
    struct arm_cm_exception_frame *frame;
    __asm__ volatile(
        "push	{r4-r7};"
        "mov   r4, r8;"
        "mov   r5, r9;"
        "mov   r6, r10;"
        "mov   r7, r11;"
        "push   {r4-r7};"
        "mov	%0, sp;"
        : "=r" (frame):
    );
    printf("busfault: ");
    dump_frame(frame);

    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
    __UNREACHABLE;
}

void _usagefault(void)
{
    struct arm_cm_exception_frame *frame;
    __asm__ volatile(
        "push	{r4-r7};"
        "mov   r4, r8;"
        "mov   r5, r9;"
        "mov   r6, r10;"
        "mov   r7, r11;"
        "push   {r4-r7};"
        "mov	%0, sp;"
        : "=r" (frame):
    );
    printf("usagefault: ");
    dump_frame(frame);
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
    __UNREACHABLE;
}
#endif
/* systick handler */
void __WEAK _systick(void)
{
    printf("systick\n");
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}

void __WEAK _debugmonitor(void)
{
    printf("debugmonitor\n");
    platform_halt(HALT_ACTION_HALT, HALT_REASON_SW_PANIC);
}
