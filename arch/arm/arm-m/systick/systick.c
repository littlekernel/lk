/*
 * Copyright (c) 2012-2014 Travis Geiselbrecht
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

/*
 * Generic systick timer support for providing system time (current_time(), current_time_hires()),
 * and a monotonic timer for the kernel.
 */

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <debug.h>
#include <assert.h>
#include <trace.h>
#include <err.h>
#include <kernel/thread.h>
#include <arch/arm.h>
#include <arch/arm/cm.h>
#include <platform.h>
#include <platform/timer.h>

#define LOCAL_TRACE 0

static volatile uint64_t ticks;
static uint32_t tick_rate = 0;
static uint32_t tick_rate_mhz = 0;
static lk_time_t tick_interval_ms;
static lk_bigtime_t tick_interval_us;

static platform_timer_callback cb;
static void *cb_args;

static void arm_cm_systick_set_periodic(lk_time_t period)
{
	LTRACEF("clk_freq %u, period %u\n", tick_rate, (uint)period);

	uint32_t ticks = tick_rate / (1000 / period);
	LTRACEF("ticks %d\n", ticks);

	SysTick->LOAD = (ticks & SysTick_LOAD_RELOAD_Msk) - 1;
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

static void arm_cm_systick_cancel_periodic(void)
{
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

/* main systick irq handler */
void _systick(void)
{
	ticks++;

	arm_cm_irq_entry();

	bool resched = false;
	if (cb) {
		lk_time_t now = current_time();
		if (cb(cb_args, now) == INT_RESCHEDULE)
			resched = true;
	}

	arm_cm_irq_exit(resched);
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
	LTRACEF("callback %p, arg %p, interval %u\n", callback, arg, interval);

	DEBUG_ASSERT(tick_rate != 0 && tick_rate_mhz != 0);

	cb = callback;
	cb_args = arg;

	tick_interval_ms = interval;
	tick_interval_us = interval * 1000;
	arm_cm_systick_set_periodic(interval);

	return NO_ERROR;
}

lk_time_t current_time(void)
{
	uint32_t reload = SysTick->LOAD  & SysTick_LOAD_RELOAD_Msk;

	uint64_t t;
	uint32_t delta;
	do {
		t = ticks;
		delta = (volatile uint32_t)SysTick->VAL;
	} while (ticks != t);

	/* convert ticks to msec */
	delta = (reload - delta) / (tick_rate_mhz * 1000);
	lk_time_t res = (t * tick_interval_ms) + delta;

	return res;
}

lk_bigtime_t current_time_hires(void)
{
	uint32_t reload = SysTick->LOAD  & SysTick_LOAD_RELOAD_Msk;

	uint64_t t;
	uint32_t delta;
	do {
		t = ticks;
		delta = (volatile uint32_t)SysTick->VAL;
	} while (ticks != t);

	/* convert ticks to usec */
	delta = (reload - delta) / tick_rate_mhz;
	lk_bigtime_t res = (t * tick_interval_us) + delta;

	return res;
}

void arm_cm_systick_init(uint32_t mhz)
{
	tick_rate = mhz;
	tick_rate_mhz = mhz / 1000000;
}

// vim: set ts=4 sw=4 noexpandtab:
