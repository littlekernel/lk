/*
 * Copyright (c) 2010 Travis Geiselbrecht
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
#include <arch.h>
#include <arch/avr32.h>
#include <kernel/thread.h>
#include <platform.h>

static void dump_iframe(struct avr32_iframe *iframe)
{
	printf("iframe %p\n", iframe);
	printf("\trar 0x%x, rsr 0x%x\n", iframe->rar, iframe->rsr);
	printf("\tr0  0x%08x r1  0x%08x r2  0x%08x r3  0x%08x\n", iframe->r0, iframe->r1, iframe->r2, iframe->r3);
	printf("\tr4  0x%08x r5  0x%08x r6  0x%08x r7  0x%08x\n", iframe->r4, iframe->r5, iframe->r6, iframe->r7);
	printf("\tr8  0x%08x r9  0x%08x r10 0x%08x r11 0x%08x\n", iframe->r8, iframe->r9, iframe->r10, iframe->r11);
	printf("\tr12 0x%08x usp 0x%08x r14 0x%08x\n", iframe->r12, iframe->usp, iframe->r14);
	hexdump(iframe, 32*4);
}

void avr32_syscall(void)
{
	printf("syscall entry\n");
	printf("sr 0x%x\n", avr32_get_sr());
	printf("rar_sup 0x%x\n", avr32_get_rar_sup());
	printf("rsr_sup 0x%x\n", avr32_get_rsr_sup());
	panic("unhandled syscall\n");
}

void avr32_unhandled(void)
{
	printf("unhandled exception %d\n", avr32_get_ecr());
	panic("unhandled\n");
}

void avr32_irq(struct avr32_iframe *iframe)
{
	inc_critical_section();

//	printf("irq: 0x%x 0x%x\n", avr32_get_rar_int0(), avr32_get_rsr_int0());
//	dump_iframe(iframe);

	if (platform_irq(iframe) == INT_RESCHEDULE) {
		thread_preempt();
	}

	dec_critical_section();
}
