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
#include <debug.h>
#include <sys/types.h>

#include <kernel/thread.h>
#include <platform/timer.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/at91sam7.h>

#define FIXED_1KHZ_TIMER 0

static platform_timer_callback timer_func;

static volatile time_t ticks = 0;

#if FIXED_1KHZ_TIMER
static volatile int timer_interval;
static volatile int timer_downcount;
#else
static int timer_ms_per_tick;
#endif

time_t current_time(void)
{
	return ticks;
}

bigtime_t current_time_hires(void)
{
	return ticks * 1000ULL;
}

static enum handler_return pit_irq_handler(void *arg)
{
    AT91PIT *pit = AT91PIT_ADDR;
    unsigned n = PIT_PICNT(pit->PIVR);

#if FIXED_1KHZ_TIMER
    ticks += n;
    timer_downcount -= n;

    if(timer_downcount <= 0) {
        timer_downcount = timer_interval;
        return timer_func(0, ticks);
    } else {
        return INT_NO_RESCHEDULE;
    }
#else
    ticks += (n * timer_ms_per_tick);
    return timer_func(0, ticks);
#endif
}

status_t platform_set_periodic_timer(platform_timer_callback callback,
                                     void *arg, time_t interval)
{
    unsigned n;
    
    AT91PIT *pit = AT91PIT_ADDR;

    n = AT91_MCK_MHZ / 16 / 1000;
    dprintf(INFO, "timer: MCK=%dKHz, n=%d\n", AT91_MCK_MHZ / 1000, n);

    enter_critical_section();
    
    timer_func = callback;

#if FIXED_1KHZ_TIMER
    timer_interval = interval;
    timer_downcount = interval;
#else
    timer_ms_per_tick = interval;
    n *= interval;
#endif
    
    pit->MR = PIT_PITEN | PIT_PITIEN | (n & 0xfffff);
    
    register_int_handler(PID_SYSIRQ, pit_irq_handler, 0);
    unmask_interrupt(PID_SYSIRQ);

    exit_critical_section();
    
    return NO_ERROR;
}

