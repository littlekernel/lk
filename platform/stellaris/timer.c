/*
 * Copyright (c) 2012 Ian McKellar
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
#include <sys/types.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/timer.h>
#include <arch/arm/cm3.h>

#include <inc/hw_types.h>
#include <driverlib/systick.h>
#include <driverlib/sysctl.h>

#define MCLK 84000000 /* XXX read this */
#define TICK_RATE (MCLK / 2)

#define LOCAL_TRACE 0

static volatile uint64_t ticks = 0;

static platform_timer_callback cb;
static void *cb_args;

/* use systick as the kernel tick */
void _systick(void)
{
	inc_critical_section();

	bool resched = false;
	if (cb) {
		lk_time_t now = current_time();
		if (cb(cb_args, now) == INT_RESCHEDULE)
			resched = true;
	}

	if (resched) {
		// have the cortex-m3 queue a preemption
		cm3_trigger_preempt();
	}

	dec_critical_section();
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
	LTRACEF("callback %p, arg %p, interval %ld\n", callback, arg, interval);

	cb = callback;
	cb_args = arg;

	cm3_systick_set_periodic(/* XXX */ MCLK, interval);

	return NO_ERROR;
}


lk_time_t current_time(void)
{
	return current_time_hires() / 1000;
}

lk_bigtime_t current_time_hires(void)
{
	uint64_t t;
	do {
		t = ticks;
		uint16_t delta = SysTickValueGet();

		if (ticks != t)
			continue;

		t += delta;
	} while (0);

	/* convert ticks to usec */
	lk_bigtime_t res = t / (TICK_RATE / 1000000);

	return res;
}

void stellaris_timer_early_init(void)
{
  SysTickPeriodSet(SysCtlClockGet() / TICK_RATE);
}

void stellaris_timer_init(void)
{
  SysTickIntEnable();
  SysTickEnable();
}


