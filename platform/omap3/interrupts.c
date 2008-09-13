/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <sys/types.h>
#include <debug.h>
#include <err.h>
#include <reg.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include "platform_p.h"
#include <platform/omap3.h>

struct int_handler_struct {
	int_handler handler;
	void *arg;
};

static struct int_handler_struct int_handler_table[INT_VECTORS];

#define vectorToController(vector) ((vector) / 32)

void platform_init_interrupts(void)
{
	unsigned int i;

	// reset the controller
	*REG32(INTC_SYSCONFIG) = 0x2; // start a reset
	while ((*REG32(INTC_SYSSTATUS) & 0x1) == 0)
		;

	// mask all interrupts
	*REG32(INTC_MIR(0)) = 0xffffffff;
	*REG32(INTC_MIR(1)) = 0xffffffff;
	*REG32(INTC_MIR(2)) = 0xffffffff;

	// set up each of the interrupts
	for (i = 0; i < INT_VECTORS; i++) {
		// set each vector up as high priority IRQ
		*REG32(INTC_ILR(i)) = 0;
		//*ICReg(i / 32, INTCON_ILR_BASE + 4*(i%32)) = ((level_trigger[i/32] & (1<<(i%32))) ? (1<<1) : (0<<1)) | 0;
	}

	// disable the priority threshold
	*REG32(INTC_THRESHOLD) = 0xff;

	// clear any pending sw interrupts
	*REG32(INTC_ISR_CLEAR(0)) = 0xffffffff;
	*REG32(INTC_ISR_CLEAR(1)) = 0xffffffff;
	*REG32(INTC_ISR_CLEAR(2)) = 0xffffffff;

	// globally unmask interrupts
	*REG32(INTC_CONTROL) = 3; // reset and enable the controller
}

status_t mask_interrupt(unsigned int vector)
{
	if (vector >= INT_VECTORS)
		return ERR_INVALID_ARGS;

//	dprintf("%s: vector %d\n", __PRETTY_FUNCTION__, vector);

	enter_critical_section();

	*REG32(INTC_MIR_SET(vectorToController(vector))) = 1 << (vector % 32);

	exit_critical_section();

	return NO_ERROR;
}


void platform_mask_irqs(void)
{
	int i;
	for (i=0; i<INT_VECTORS; i++)
		mask_interrupt(i);
}

status_t unmask_interrupt(unsigned int vector)
{
	if (vector >= INT_VECTORS)
		return ERR_INVALID_ARGS;

//	dprintf("%s: vector %d\n", __PRETTY_FUNCTION__, vector);

	enter_critical_section();

	*REG32(INTC_MIR_CLEAR(vectorToController(vector))) = 1 << (vector % 32);

	exit_critical_section();

	return NO_ERROR;
}

enum handler_return platform_irq(struct arm_iframe *frame)
{
	// get the current vector
	unsigned int vector;
   
	// read the currently active IRQ
	vector = *REG32(INTC_SIR_IRQ) & 0x7f;

//	TRACEF("spsr 0x%x, pc 0x%x, currthread %p, vector %d, handler %p\n", frame->spsr, frame->pc, current_thread, vector, int_handler_table[vector].handler);

#if THREAD_STATS
	thread_stats.interrupts++;
#endif

	// deliver the interrupt
	enum handler_return ret; 

	ret = INT_NO_RESCHEDULE;
	if (int_handler_table[vector].handler)
		ret = int_handler_table[vector].handler(int_handler_table[vector].arg);

	// ack the interrupt
	*REG32(INTC_CONTROL) = 0x1;

	return ret;
}

void platform_fiq(struct arm_iframe *frame)
{
	PANIC_UNIMPLEMENTED;
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
	if (vector >= INT_VECTORS)
		panic("register_int_handler: vector out of range %d\n", vector);

	enter_critical_section();

	int_handler_table[vector].arg = arg;
	int_handler_table[vector].handler = handler;

	exit_critical_section();
}


