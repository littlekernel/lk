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
#include <err.h>
#include <reg.h>
#include <debug.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/console.h>
#include <platform/timer.h>
#include <platform/pc.h>
#include "platform_p.h"
#include <arch/x86.h>

static platform_timer_callback t_callback;
static void *callback_arg;

static uint64_t next_trigger_time;
static uint64_t next_trigger_delta;

static uint64_t timer_delta_time;
static uint64_t timer_current_time;

static uint16_t divisor;

#define INTERNAL_FREQ 1193182ULL
#define INTERNAL_FREQ_3X 3579546ULL

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, time_t interval)
{
	enter_critical_section();

	t_callback = callback;
	callback_arg = arg;
	
	next_trigger_delta = (uint64_t) interval << 32;
	next_trigger_time = timer_current_time + next_trigger_delta;

	exit_critical_section();

	return NO_ERROR;
}

time_t current_time(void)
{
	time_t time;
	
	enter_critical_section();
	time = (time_t) (timer_current_time >> 32);
	exit_critical_section();
	
	return time;
}

bigtime_t current_time_hires(void)
{
	bigtime_t time;
	
	enter_critical_section();
	time = (bigtime_t) ((timer_current_time >> 22) * 1000) >> 10;
	exit_critical_section();
	
	return time;
}
static enum handler_return os_timer_tick(void *arg)
{
	uint64_t delta;
	
	timer_current_time += timer_delta_time;
	
	time_t time = current_time();
	//bigtime_t btime = current_time_hires();
	//printf_xy(71, 0, WHITE, "%08u", (uint32_t) time);
	//printf_xy(63, 1, WHITE, "%016llu", (uint64_t) btime);
	
	if (t_callback && timer_current_time >= next_trigger_time) {
		delta = timer_current_time - next_trigger_time;
		next_trigger_time = timer_current_time + next_trigger_delta - delta;
		
		return t_callback(callback_arg, time);
	} else {
		return INT_NO_RESCHEDULE;
	}
}

static void set_pit_frequency(uint32_t frequency)
{
	uint32_t count, remainder;
	
	/* figure out the correct divisor for the desired frequency */
	if (frequency <= 18) {
		count = 0xffff;
	} else if (frequency >= INTERNAL_FREQ) {
		count = 1;
	} else {
		count = INTERNAL_FREQ_3X / frequency;
		remainder = INTERNAL_FREQ_3X % frequency;
		
		if (remainder >= INTERNAL_FREQ_3X / 2) {
			count += 1;
		}
		
		count /= 3;
		remainder = count % 3;
		
		if (remainder >= 1) {
			count += 1;
		}
	}
		
	divisor = count & 0xffff;
	
	/*
	 * funky math that i don't feel like explaining. essentially 32.32 fixed
	 * point representation of the configured timer delta.
	 */
	timer_delta_time = (3685982306ULL * count) >> 10;
	
	//dprintf(DEBUG, "set_pit_frequency: dt=%016llx\n", timer_delta_time);
	//dprintf(DEBUG, "set_pit_frequency: divisor=%04x\n", divisor);
	
	/*
	 * setup the Programmable Interval Timer
	 * timer 0, mode 2, binary counter, LSB followed by MSB
	 */
	outp(I8253_CONTROL_REG, 0x34);
	outp(I8253_DATA_REG, divisor & 0xff); // LSB
	outp(I8253_DATA_REG, divisor >> 8); // MSB
}

void platform_init_timer(void)
{
	timer_current_time = 0;
	
	set_pit_frequency(1000); // ~1ms granularity

	register_int_handler(INT_PIT, &os_timer_tick, NULL);
	unmask_interrupt(INT_PIT);
}

void platform_halt_timers(void)
{
	mask_interrupt(INT_PIT);
}

