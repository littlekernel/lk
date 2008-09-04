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
#include <reg.h>
#include <debug.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <platform/omap3.h>
#include "platform_p.h"

static time_t tick_interval;
static platform_timer_callback t_callback;
static void *callback_arg;

/* timer 2 */
static const ulong timer_base = OMAP34XX_GPT2;

#define TIMER_TICK_RATE 32768

#define TIMER_REG(reg) *REG32(timer_base + (reg))

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, time_t interval)
{
	enter_critical_section();

	t_callback = callback;
	callback_arg = arg;
	tick_interval = interval;
	uint32_t ticks_per_interval = (uint64_t)interval * TIMER_TICK_RATE / 1000; // interval is in ms

	TIMER_REG(TCLR) = 0; // stop the timer
	TIMER_REG(TLDR) = -ticks_per_interval;
	TIMER_REG(TTGR) = 1;
	TIMER_REG(TIER) = 0x2;
	TIMER_REG(TCLR) = 0x3; // autoreload, start

	unmask_interrupt(GPT2_IRQ);

	exit_critical_section();

	return NO_ERROR;
}

time_t current_time(void)
{
	uint32_t delta_ticks;
	uint32_t delta_ticks2;
	
retry:
	delta_ticks = *REG32(TIMER32K_CR);
	delta_ticks2 = *REG32(TIMER32K_CR);
	if (delta_ticks2 != delta_ticks)
		goto retry;

	uint64_t longtime = delta_ticks * 1000ULL / 32768ULL;

	return (time_t)longtime;
}

bigtime_t current_time_hires(void)
{
	uint32_t delta_ticks;
	uint32_t delta_ticks2;
	
retry:
	delta_ticks = *REG32(TIMER32K_CR);
	delta_ticks2 = *REG32(TIMER32K_CR);
	if (delta_ticks2 != delta_ticks)
		goto retry;

	uint64_t longtime = delta_ticks * 1000000ULL / 32768ULL;

	return (bigtime_t)longtime;
}
static enum handler_return os_timer_tick(void *arg)
{
	TIMER_REG(TISR) = TIMER_REG(TISR);

	return t_callback(callback_arg, current_time());
}

void platform_init_timer(void)
{
	/* GPT2 */
	RMWREG32(CM_CLKSEL_PER, 0, 1, 1);
	RMWREG32(CM_ICLKEN_PER, 3, 1, 1);
	RMWREG32(CM_FCLKEN_PER, 3, 1, 1);

	// reset the GP timer 
	TIMER_REG(TIOCP_CFG) = 0x2;
	while ((TIMER_REG(TISTAT) & 1) == 0)
		;

	// set GPT2-9 clock inputs over to 32k
	*REG32(CM_CLKSEL_PER) = 0;

	// disable ints
	TIMER_REG(TIER) = 0;
	TIMER_REG(TISR) = 0x7; // clear any pending bits

	// XXX make sure 32K timer is running

	register_int_handler(GPT2_IRQ, &os_timer_tick, NULL);
}

void platform_halt_timers(void)
{
	TIMER_REG(TCLR) = 0;
}

