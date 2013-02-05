/*
 * Copyright (c) 2009 Corey Tabaka
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
#include <arch/x86.h>
#include "platform_p.h"
#include <platform/pc.h>

void x86_gpf_handler(struct x86_iframe *frame);
void x86_invop_handler(struct x86_iframe *frame);
void x86_unhandled_exception(struct x86_iframe *frame);

#define PIC1 0x20
#define PIC2 0xA0

#define ICW1 0x11
#define ICW4 0x01

struct int_handler_struct {
	int_handler handler;
	void *arg;
};

static struct int_handler_struct int_handler_table[INT_VECTORS];

/*
 * Cached IRQ mask (enabled/disabled)
 */
static uint8_t irqMask[2];

/*
 * init the PICs and remap them
 */
static void map(uint32_t pic1, uint32_t pic2)
{
	/* send ICW1 */
	outp(PIC1, ICW1);
	outp(PIC2, ICW1);

	/* send ICW2 */
	outp(PIC1 + 1, pic1);   /* remap */
	outp(PIC2 + 1, pic2);   /*  pics */

	/* send ICW3 */
	outp(PIC1 + 1, 4);  /* IRQ2 -> connection to slave */
	outp(PIC2 + 1, 2);

	/* send ICW4 */
	outp(PIC1 + 1, 5);
	outp(PIC2 + 1, 1);

	/* disable all IRQs */
	outp(PIC1 + 1, 0xff);
	outp(PIC2 + 1, 0xff);

	irqMask[0] = 0xff;
	irqMask[1] = 0xff;
}

static void enable(unsigned int vector, bool enable)
{
	if (vector >= PIC1_BASE && vector < PIC1_BASE + 8) {
		vector -= PIC1_BASE;

		uint8_t bit = 1 << vector;

		if (enable && (irqMask[0] & bit)) {
			irqMask[0] = inp(PIC1 + 1);
			irqMask[0] &= ~bit;
			outp(PIC1 + 1, irqMask[0]);
			irqMask[0] = inp(PIC1 + 1);
		} else if (!enable && !(irqMask[0] & bit)) {
			irqMask[0] = inp(PIC1 + 1);
			irqMask[0] |= bit;
			outp(PIC1 + 1, irqMask[0]);
			irqMask[0] = inp(PIC1 + 1);
		}
	} else if (vector >= PIC2_BASE && vector < PIC2_BASE + 8) {
		vector -= PIC2_BASE;

		uint8_t bit = 1 << vector;

		if (enable && (irqMask[1] & bit)) {
			irqMask[1] = inp(PIC2 + 1);
			irqMask[1] &= ~bit;
			outp(PIC2 + 1, irqMask[1]);
			irqMask[1] = inp(PIC2 + 1);
		} else if (!enable && !(irqMask[1] & bit)) {
			irqMask[1] = inp(PIC2 + 1);
			irqMask[1] |= bit;
			outp(PIC2 + 1, irqMask[1]);
			irqMask[1] = inp(PIC2 + 1);
		}

		bit = 1 << (INT_PIC2 - PIC1_BASE);

		if (irqMask[1] != 0xff && (irqMask[0] & bit)) {
			irqMask[0] = inp(PIC1 + 1);
			irqMask[0] &= ~bit;
			outp(PIC1 + 1, irqMask[0]);
			irqMask[0] = inp(PIC1 + 1);
		} else if (irqMask[1] == 0 && !(irqMask[0] & bit)) {
			irqMask[0] = inp(PIC1 + 1);
			irqMask[0] |= bit;
			outp(PIC1 + 1, irqMask[0]);
			irqMask[0] = inp(PIC1 + 1);
		}
	} else {
		//dprintf(DEBUG, "Invalid PIC interrupt: %02x\n", vector);
	}
}

void issueEOI(unsigned int vector)
{
	if (vector >= PIC1_BASE && vector <= PIC1_BASE + 7) {
		outp(PIC1, 0x20);
	} else if (vector >= PIC2_BASE && vector <= PIC2_BASE + 7) {
		outp(PIC2, 0x20);
		outp(PIC1, 0x20);   // must issue both for the second PIC
	}
}

void platform_init_interrupts(void)
{
	// rebase the PIC out of the way of processor exceptions
	map(PIC1_BASE, PIC2_BASE);
}

status_t mask_interrupt(unsigned int vector)
{
	if (vector >= INT_VECTORS)
		return ERR_INVALID_ARGS;

//	dprintf(DEBUG, "%s: vector %d\n", __PRETTY_FUNCTION__, vector);

	enter_critical_section();

	enable(vector, false);

	exit_critical_section();

	return NO_ERROR;
}


void platform_mask_irqs(void)
{
	irqMask[0] = inp(PIC1 + 1);
	irqMask[1] = inp(PIC2 + 1);

	outp(PIC1 + 1, 0xff);
	outp(PIC2 + 1, 0xff);

	irqMask[0] = inp(PIC1 + 1);
	irqMask[1] = inp(PIC2 + 1);
}

status_t unmask_interrupt(unsigned int vector)
{
	if (vector >= INT_VECTORS)
		return ERR_INVALID_ARGS;

//	dprintf("%s: vector %d\n", __PRETTY_FUNCTION__, vector);

	enter_critical_section();

	enable(vector, true);

	exit_critical_section();

	return NO_ERROR;
}

enum handler_return platform_irq(struct x86_iframe *frame)
{
	// get the current vector
	unsigned int vector = frame->vector;

	THREAD_STATS_INC(interrupts);

	// deliver the interrupt
	enum handler_return ret = INT_NO_RESCHEDULE;

	switch (vector) {
		case INT_GP_FAULT:
			x86_gpf_handler(frame);
			break;

		case INT_INVALID_OP:
			x86_invop_handler(frame);
			break;

		case INT_DIVIDE_0:
		case INT_DEBUG_EX:
		case INT_DEV_NA_EX:
		case INT_PAGE_FAULT:
		case INT_STACK_FAULT:
		case 3:
			x86_unhandled_exception(frame);
			break;

		default:
			if (int_handler_table[vector].handler)
				ret = int_handler_table[vector].handler(int_handler_table[vector].arg);
	}

	// ack the interrupt
	issueEOI(vector);

	return ret;
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


