/*
 * Copyright (c) 2009-2010 Travis Geiselbrecht
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
#include <reg.h>
#include <err.h>
#include <kernel/thread.h>
#include <debug.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <platform/at32ap7.h>
#include "platform_p.h"

#define LOCAL_TRACE 0

#define NUM_TMR 6
#define TMR_FREQ (32768)

static time_t system_time = 0;

static time_t tick_interval;
static uint32_t ticks_per_interval;
static platform_timer_callback t_callback;
static void *callback_arg;

static inline addr_t TMR_REG_ADDR(unsigned int tmr, unsigned int reg)
{
	return ((tmr >= 3) ? (TC1_BASE + reg + 0x40 * (tmr - 3)) : (TC0_BASE + reg + 0x40 * (tmr)));
}

#define TMR_REG(tmr, reg) (*REG32(TMR_REG_ADDR(tmr, reg)))

static enum handler_return os_timer_tick(void *arg)
{
	uint32_t hole = TMR_REG(0, TC_SR); // gotta read the status register to clear the state
	hole = hole;

	system_time += tick_interval;
//	printf("os_timer_tick %d\n", system_time);

	if (!t_callback)
		return INT_NO_RESCHEDULE;

	return t_callback(callback_arg, system_time);
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, time_t interval)
{
	enter_critical_section();

	LTRACEF("callback %p, arg %p, interval %d\n", callback, arg, interval);

	t_callback = callback;
	callback_arg = arg;
	tick_interval = interval;
	ticks_per_interval = interval * TMR_FREQ / 1000; // interval is in ms

	TMR_REG(0, TC_CCR) = (1<<1); // CLKDIS
	TMR_REG(0, TC_CMR) = (1<<14); // CPCTRG
	TMR_REG(0, TC_RC) = interval;
	TMR_REG(0, TC_IER) = (1<<4);
	TMR_REG(0, TC_CCR) = (1<<0); // CLKEN
	TMR_REG(0, TC_CCR) |= (1<<2); // SWTRG

	register_int_handler(INT_TC00, &os_timer_tick, NULL);
	unmask_interrupt(INT_TC00);

	exit_critical_section();

	return NO_ERROR;
}

time_t current_time(void)
{
	time_t t;
	uint32_t delta_ticks;
	uint32_t delta_ticks2;

retry:
	delta_ticks = TMR_REG(0, TC_CV);
	t = system_time;
	delta_ticks2 = TMR_REG(0, TC_CV);
	if (delta_ticks2 < delta_ticks)
		goto retry;

	t += (delta_ticks2 * tick_interval) / ticks_per_interval;

	return t;
}

bigtime_t current_time_hires(void)
{
	return current_time() * 1000ULL;
}


void platform_init_timer(void)
{
	TRACE_ENTRY;
	
	platform_set_clock_enable(CLOCK_TC0, true);
	platform_set_clock_enable(CLOCK_TC1, true);

	// disable all the clocks
	unsigned int i;
	for (i = 0; i < NUM_TMR; i++) {
		TMR_REG(i, TC_CCR) = (1<<1); // CLKDIS
		TMR_REG(i, TC_IDR) = 0xf; // disable all interrupts
	}

	TRACE_EXIT;
}

