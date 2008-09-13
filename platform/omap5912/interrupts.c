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
#include <err.h>
#include <sys/types.h>
#include <debug.h>
#include <reg.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include "platform_p.h"
#include <platform/omap5912.h>

struct int_handler_struct {
	int_handler handler;
	void *arg;
};

static struct int_handler_struct int_handler_table[INT_VECTORS];

static const uint32_t icBase[5] = {
	INTCON0_BASE, INTCON1_BASE, INTCON2_BASE, INTCON3_BASE, INTCON4_BASE };

/* a bitmap of the level triggered interrupt vectors */
static uint32_t level_trigger[5] = {
	0xb3fefe8f,	// level 1 0-31
	0xfdb3c1fd,	// level 2 0-31
	0xfffff7ff, // level 2 32-63
	0xbfffffff, // level 2 64-95
	0xffffffff // level 2 96-128
};

static inline volatile uint32_t *ICReg(uint controller, uint reg)
{
	return (volatile uint32_t *)(icBase[controller] + reg);
}

static inline uint32_t readICReg(uint controller, uint reg)
{
	return *ICReg(controller, reg);
}
static inline void writeICReg(uint controller, uint reg, uint val)
{
	*ICReg(controller, reg) = val;
}

static inline uint vectorToController(uint vector)
{
	return vector / 32;
}

void platform_init_interrupts(void)
{
	unsigned int i;

	// mask all interrupts
	*ICReg(0, INTCON_MIR) = 0xfffffffa;
	*ICReg(1, INTCON_MIR) = 0xffffffff;
	*ICReg(2, INTCON_MIR) = 0xffffffff;
	*ICReg(3, INTCON_MIR) = 0xffffffff;
	*ICReg(4, INTCON_MIR) = 0xffffffff;

	// set up each of the interrupts
	for (i = 0; i < INT_VECTORS; i++) {
		// set each vector up as high priority, IRQ, and default edge/level sensitivity
		*ICReg(i / 32, INTCON_ILR_BASE + 4*(i%32)) = ((level_trigger[i/32] & (1<<(i%32))) ? (1<<1) : (0<<1)) | 0;
	}

	// clear any pending interrupts
	*ICReg(0, INTCON_ITR) = 0;
	*ICReg(1, INTCON_ITR) = 0;
	*ICReg(2, INTCON_ITR) = 0;
	*ICReg(3, INTCON_ITR) = 0;
	*ICReg(4, INTCON_ITR) = 0;

	// globally unmask interrupts
	*ICReg(1, INTCON_CONTROL) = 3;
	*ICReg(0, INTCON_CONTROL) = 3;
	*ICReg(0, INTCON_GMR) = 0;
}

status_t mask_interrupt(unsigned int vector)
{
	if (vector >= INT_VECTORS)
		return ERR_INVALID_ARGS;

//	dprintf("%s: vector %d\n", __PRETTY_FUNCTION__, vector);

	enter_critical_section();

	volatile uint32_t *mir = ICReg(vectorToController(vector), INTCON_MIR);
	*mir = *mir | (1<<(vector % 32));

	exit_critical_section();

	return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
	if (vector >= INT_VECTORS)
		return ERR_INVALID_ARGS;

//	dprintf("%s: vector %d\n", __PRETTY_FUNCTION__, vector);

	enter_critical_section();

	volatile uint32_t *mir = ICReg(vectorToController(vector), INTCON_MIR);
	*mir = *mir & ~(1<<(vector % 32));

	exit_critical_section();

	return NO_ERROR;
}

enum handler_return platform_irq(struct arm_iframe *frame)
{
	// get the current vector
	unsigned int vector;
   
#if THREAD_STATS
	thread_stats.interrupts++;
#endif

	// read from the first level int handler
	vector = *ICReg(0, INTCON_SIR_IRQ);
	
	// see if it's coming from the second level handler
	if (vector == 0) {
		vector = *ICReg(1, INTCON_SIR_IRQ) + 32;
	}

//	dprintf("platform_irq: spsr 0x%x, pc 0x%x, currthread %p, vector %d\n", frame->spsr, frame->pc, current_thread, vector);

	// deliver the interrupt
	enum handler_return ret; 

	ret = INT_NO_RESCHEDULE;
	if (int_handler_table[vector].handler)
		ret = int_handler_table[vector].handler(int_handler_table[vector].arg);

	// ack the interrupt
	if (vector >= 32) {
		// interrupt is chained, so ack the second level first, and then the first
		*ICReg(vector / 32, INTCON_ITR) = ~(1 << (vector % 32));
		*ICReg(1, INTCON_CONTROL) |= 1;
		vector = 0; // force the following code to ack the chained first level vector
	} 

	*ICReg(0, INTCON_ITR) = ~(1 << vector);
	*ICReg(0, INTCON_CONTROL) = 1;

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

	int_handler_table[vector].handler = handler;
	int_handler_table[vector].arg = arg;

	exit_critical_section();
}

