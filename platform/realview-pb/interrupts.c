/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <trace.h>
#include <reg.h>
#include <kernel/thread.h>
#include <kernel/debug.h>
#include <platform/interrupts.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include <platform/realview-pb.h>
#include "platform_p.h"

struct int_handler_struct {
	int_handler handler;
	void *arg;
};

static struct int_handler_struct int_handler_table[MAX_INT];

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
	if (vector >= MAX_INT)
		panic("register_int_handler: vector out of range %d\n", vector);

	enter_critical_section();

	int_handler_table[vector].handler = handler;
	int_handler_table[vector].arg = arg;

	exit_critical_section();
}

/* GIC on cortex-a8 */
#define GICREG(gic, reg) (*REG32(GICBASE(gic) + (reg)))

/* main cpu regs */
#define CONTROL  (0x00)
#define PRIMASK  (0x04)
#define BINPOINT (0x08)
#define INTACK   (0x0c)
#define EOI      (0x10)
#define RUNNING  (0x14)
#define HIGHPEND (0x18)

/* distribution regs */
#define DISTCONTROL (0x1000)
#define SETENABLE0  (0x1100)
#define SETENABLE1  (0x1104)
#define SETENABLE2  (0x1108)
#define CLRENABLE0  (0x1180)
#define CLRENABLE1  (0x1184)
#define CLRENABLE2  (0x1188)
#define SETPEND0    (0x1200)
#define SETPEND1    (0x1204)
#define SETPEND2    (0x1208)
#define CLRPEND0    (0x1280)
#define CLRPEND1    (0x1284)
#define CLRPEND2    (0x1288)
#define ACTIVE0     (0x1300)
#define ACTIVE1     (0x1304)
#define ACTIVE2     (0x1308)

static void gic_set_enable(uint vector, bool enable)
{
	if (enable) {
		uint regoff = (vector < 32) ? SETENABLE0 : ((vector < 64) ? SETENABLE1 : SETENABLE2);
		GICREG(0, regoff) |= (1 << (vector % 32));
	} else {
		uint regoff = (vector < 32) ? CLRENABLE0 : ((vector < 64) ? CLRENABLE1 : CLRENABLE2);
		GICREG(0, regoff) &= ~(1 << (vector % 32));
	}
}

void platform_init_interrupts(void)
{
	GICREG(0, CLRENABLE0) = 0xffffffff;
	GICREG(0, CLRENABLE1) = 0xffffffff;
	GICREG(0, CLRENABLE2) = 0xffffffff;
	GICREG(0, CLRPEND0) = 0xffffffff;
	GICREG(0, CLRPEND1) = 0xffffffff;
	GICREG(0, CLRPEND2) = 0xffffffff;
	GICREG(0, PRIMASK) = 0xf0;

	GICREG(0, CONTROL) = 1; // enable GIC0
	GICREG(0, DISTCONTROL) = 1; // enable GIC0
}

status_t mask_interrupt(unsigned int vector)
{
	if (vector >= MAX_INT)
		return ERR_INVALID_ARGS;

	enter_critical_section();

	gic_set_enable(vector, false);

	exit_critical_section();

	return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
	if (vector >= MAX_INT)
		return ERR_INVALID_ARGS;

	enter_critical_section();

	gic_set_enable(vector, true);

	exit_critical_section();

	return NO_ERROR;
}

enum handler_return platform_irq(struct arm_iframe *frame)
{
	// get the current vector
	unsigned int vector = GICREG(0, INTACK) & 0x3ff;

	// see if it's spurious
	if (vector == 0x3ff) {
		GICREG(0, EOI) = 0x3ff; // XXX is this necessary?
		return INT_NO_RESCHEDULE;
	}

	THREAD_STATS_INC(interrupts);
	KEVLOG_IRQ_ENTER(vector);

//	printf("platform_irq: spsr 0x%x, pc 0x%x, currthread %p, vector %d\n", frame->spsr, frame->pc, current_thread, vector);

	// deliver the interrupt
	enum handler_return ret;

	ret = INT_NO_RESCHEDULE;
	if (int_handler_table[vector].handler)
		ret = int_handler_table[vector].handler(int_handler_table[vector].arg);

	GICREG(0, EOI) = vector;

//	printf("platform_irq: exit %d\n", ret);

	KEVLOG_IRQ_EXIT(vector);

	return ret;
}

void platform_fiq(struct arm_iframe *frame)
{
	PANIC_UNIMPLEMENTED;
}

/* vim: set ts=4 sw=4 noexpandtab: */
