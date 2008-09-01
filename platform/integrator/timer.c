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
#include <err.h>
#include <kernel/thread.h>
#include <debug.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <platform/integrator.h>
#include "platform_p.h"

static time_t system_time = 0;

static time_t tick_interval;
static uint32_t ticks_per_interval;
static platform_timer_callback t_callback;
static void *callback_arg;

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, time_t interval)
{
#if 0
	enter_critical_section();

	t_callback = callback;
	callback_arg = arg;
	tick_interval = interval;
	ticks_per_interval = interval * 32768 / 1000; // interval is in ms

	OS_TIMER_CTRL_REG = 0; // stop it
	OS_TIMER_TICK_VALUE_REG = ticks_per_interval;
	OS_TIMER_CTRL_REG = (1<<3) | (1<<2) | (1<<1) | (1<<0);

	exit_critical_section();
#endif

	return NO_ERROR;
}

time_t current_time(void)
{
#if 0
	time_t t;
	uint32_t delta_ticks;
	uint32_t delta_ticks2;
	
retry:
	delta_ticks = OS_TIMER_TICK_COUNTER_REG;
	t = system_time;
	delta_ticks2 = OS_TIMER_TICK_COUNTER_REG;
	if (delta_ticks2 > delta_ticks)
		goto retry;

	t += ((ticks_per_interval - delta_ticks2) * tick_interval) / ticks_per_interval;

	return t;
#else
	static time_t time = 0;
	return time++;
#endif

}

static enum handler_return os_timer_tick(void *arg)
{
	system_time += tick_interval;
//	dprintf("os_timer_tick %d\n", system_time);

	return t_callback(callback_arg, system_time);
}

void platform_init_timer(void)
{
#if 0
	OS_TIMER_CTRL_REG = 0; // stop the timer if it's already running

	register_int_handler(IRQ_OS_TIMER, &os_timer_tick, NULL);
	unmask_interrupt(IRQ_OS_TIMER, NULL);
#endif
}

