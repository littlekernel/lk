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
#include <err.h>
#include <reg.h>
#include <arch.h>
#include <arch/avr32.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/at32ap7.h>
#include "platform_p.h"

// XXX make this more efficient
#define INT_VECTORS (INT_GROUP_COUNT << INT_GROUP_SHIFT)

struct int_handler_struct {
	int_handler handler;
	void *arg;
};

static struct int_handler_struct int_handler_table[INT_VECTORS];

void platform_init_interrupts(void)
{
	int i;

	TRACE_ENTRY;

	// enable the clock
	platform_set_clock_enable(CLOCK_INTC, true);

	// set the interrupt vectors to the proper offset, INT0
	for (i = 0; i < INT_GROUP_COUNT; i++) {
		*REG32(INTC_IPR(i)) = (0 << 30) | avr32_get_interrupt_autovector_offset();
//		printf("0x%x\n", *REG32(INTC_IPR(i)));
	}
	
	TRACE_EXIT;
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg)
{
	enter_critical_section();

	int_handler_table[vector].arg = arg;
	int_handler_table[vector].handler = handler;

	exit_critical_section();
}

status_t mask_interrupt(unsigned int vector)
{
	// XXX can't actually mask
	return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector)
{
	// XXX can't actually mask
	return NO_ERROR;
}

int platform_irq(struct avr32_iframe *iframe)
{
//	printf("int sr 0x%x\n", avr32_get_sr());

	uint group = *REG32(INTC_ICR(0)) & 0x3f;
	uint linebits = *REG32(INTC_IRR(group));
	if (unlikely(linebits == 0)) {
		// nothing is set, must be spurious
		return INT_NO_RESCHEDULE;
	}

	uint line = __builtin_ctz(linebits);
	uint vector = INT_VECTOR(group, line);

	int ret = INT_NO_RESCHEDULE;
	if (likely(int_handler_table[vector].handler != NULL)) {
		ret = int_handler_table[vector].handler(int_handler_table[vector].arg);
	}

	return ret;

}

