/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#include <trace.h>
#include <err.h>
#include <sys/types.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/timer.h>
#include <arch/arm/cm.h>

#include <platform/lpc.h>

#define LOCAL_TRACE 0

static volatile uint64_t ticks;
static uint32_t tick_rate_mhz = 0;
static lk_time_t tick_interval_ms;
static lk_bigtime_t tick_interval_us;

static platform_timer_callback cb;
static void *cb_args;

/* use systick as the kernel tick */
void _systick(void)
{
    arm_cm_irq_entry();

    ticks++;
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
    LTRACEF("callback %p, arg %p, interval %ld\n", callback, arg, interval);

    enter_critical_section();

    cb = callback;
    cb_args = arg;

    tick_interval_ms = interval;
    tick_interval_us = interval * 1000;
    arm_cm_systick_set_periodic(Chip_Clock_GetMainClockRate(), interval);

    exit_critical_section();

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

void lpc_timer_early_init(void)
{
    uint32_t clock_rate = Chip_Clock_GetMainClockRate();
    tick_rate_mhz = clock_rate / 1000000;

#if 0
    SysCtlPeripheralEnable(SYSCTL_PERIPH_WTIMER0);

    /* configure the timer as a 64bit up counting tick at syslock rate */
    TimerConfigure(WTIMER0_BASE, TIMER_CFG_PERIODIC_UP);
    TimerLoadSet64(WTIMER0_BASE, ULONG_MAX);
    TimerEnable(WTIMER0_BASE, TIMER_BOTH);
#endif
}

void lpc_timer_init(void)
{
}


