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
#include <compiler.h>
#include <stdint.h>
#include <kernel/thread.h>
#include <arch/arm/cm.h>

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
	inc_critical_section();
	printf("hardfault: ");
	dump_frame(frame);

	printf("HFSR 0x%x\n", SCB->HFSR);

	halt();
}

static void usagefault(struct arm_cm_exception_frame *frame)
{
	inc_critical_section();
	printf("usagefault: ");
	dump_frame(frame);

	halt();
}

static void busfault(struct arm_cm_exception_frame *frame)
{
	inc_critical_section();
	printf("busfault: ");
	dump_frame(frame);

	halt();
}

/* raw exception vectors */

void _nmi(void)
{
	inc_critical_section();
	printf("nmi\n");
	halt();
}

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
	inc_critical_section();
	printf("memmanage\n");
	halt();
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

/* systick handler */
void __WEAK _systick(void)
{
	inc_critical_section();
	printf("systick\n");
	halt();
}


