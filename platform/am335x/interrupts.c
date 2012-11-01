/*
 * Copyright (c) 2012 Corey Tabaka
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
#include <err.h>
#include <sys/types.h>
#include <debug.h>
#include <reg.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/am335x.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include "platform_p.h"

#include <soc_AM335x.h>
#include <interrupt.h>

struct int_handler_struct {
	int_handler handler;
	void *arg;
};

#define NUM_INTS 128

static struct int_handler_struct int_handler_table[NUM_INTS];

void platform_init_interrupts(void)
{
	/* Initialize the ARM interrupt control */
	IntAINTCInit();
	IntPrioritySet(SYS_INT_TINT2, 0, AINTC_HOSTINT_ROUTE_IRQ);
}

status_t mask_interrupt(unsigned int vector)
{
	if (vector >= NUM_INTS)
		return ERR_INVALID_ARGS;

//	dprintf("%s: vector %d\n", __PRETTY_FUNCTION__, vector);

	enter_critical_section();

	IntSystemDisable(vector);

	exit_critical_section();

	return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
	if (vector >= NUM_INTS)
		return ERR_INVALID_ARGS;

//	dprintf("%s: vector %d\n", __PRETTY_FUNCTION__, vector);

	enter_critical_section();

	IntSystemEnable(vector);

	exit_critical_section();

	return NO_ERROR;
}

enum handler_return platform_irq(struct arm_iframe *frame)
{
	// get the current vector
	unsigned int vector = IntActiveIrqNumGet();

	THREAD_STATS_INC(interrupts);

//	printf("platform_irq: spsr 0x%x, pc 0x%x, currthread %p, vector %d\n", frame->spsr, frame->pc, current_thread, vector);

	// deliver the interrupt
	enum handler_return ret;

	ret = INT_NO_RESCHEDULE;
	if (int_handler_table[vector].handler)
		ret = int_handler_table[vector].handler(int_handler_table[vector].arg);

	/* enable generation of new IRQ */
	HWREG(SOC_AINTC_REGS + INTC_CONTROL) = INTC_CONTROL_NEWIRQAGR;
	DSB; // wait for write to complete

//	dprintf("platform_irq: exit %d\n", ret);

	return ret;
}

void platform_fiq(struct arm_iframe *frame)
{
	panic("FIQ: unimplemented\n");
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
	if (vector >= NUM_INTS)
		panic("register_int_handler: vector out of range %d\n", vector);

	enter_critical_section();

	int_handler_table[vector].handler = handler;
	int_handler_table[vector].arg = arg;

	exit_critical_section();
}

