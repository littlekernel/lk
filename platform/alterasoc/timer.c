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
#include <sys/types.h>
#include <err.h>
#include <stdio.h>
#include <assert.h>
#include <trace.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <platform/alterasoc.h>
#include "platform_p.h"

/* driver for cortex-a9's private timer */
#define LOCAL_TRACE 0

#define TIMREG(reg)   (*REG32(PRIV_TIMER_BASE + (reg)))

#define TIMER_LOAD      (0x00)
#define TIMER_COUNTER   (0x04)
#define TIMER_CONTROL   (0x08)
#define TIMER_ISR       (0x0c)
#define WDOG_LOAD       (0x20)
#define WDOG_COUNTER    (0x24)
#define WDOG_CONTROL    (0x28)
#define WDOG_ISR        (0x2c)

#define GTIMREG(reg)   (*REG32(GLOBAL_TIMER_BASE + (reg)))

#define GTIMER_COUNT_LO (0x00)
#define GTIMER_COUNT_HI (0x04)
#define GTIMER_CONTROL  (0x08)
#define GTIMER_ISR      (0x0c)
#define GTIMER_COMPARE_LO (0x10)
#define GTIMER_COMPARE_HI (0x14)
#define GTIMER_INCREMENT (0x18)

static platform_timer_callback t_callback;

static volatile uint ticks = 0;
static lk_time_t periodic_interval;
static uint32_t timer_freq;

uint64_t get_global_val(void)
{
    uint32_t lo, hi;

retry:
    hi = GTIMREG(GTIMER_COUNT_HI);
    lo = GTIMREG(GTIMER_COUNT_LO);
    if (GTIMREG(GTIMER_COUNT_HI) != hi)
        goto retry;


    return ((uint64_t)hi << 32 | lo);
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
    enter_critical_section();

    LTRACEF("callback %p, arg %p, interval %lu\n", callback, arg, interval);

    t_callback = callback;

    periodic_interval = interval;

    // disable timer
    TIMREG(TIMER_CONTROL) = 0;

    TIMREG(TIMER_LOAD) = (((uint64_t)timer_freq * interval) / 1000);
    TIMREG(TIMER_CONTROL) = (1<<2) | (1<<1) | (1<<0); // irq enable, autoreload, enable

    unmask_interrupt(CPU_PRIV_TIMER);

    exit_critical_section();

    return NO_ERROR;
}

lk_bigtime_t current_time_hires(void)
{
    lk_bigtime_t time;

    time = ticks * periodic_interval * 1000ULL;

    return time;
}

lk_time_t current_time(void)
{
    lk_time_t time;

    time = ticks * periodic_interval;

    return time;
}

static enum handler_return platform_tick(void *arg)
{
    ticks++;

    LTRACE;

    TIMREG(TIMER_ISR) = 1; // ack the irq

    if (t_callback) {
        return t_callback(arg, current_time());
    } else {
        return INT_NO_RESCHEDULE;
    }
}

void platform_init_timer(uint32_t freq)
{
    /* disable timer */
    TIMREG(TIMER_CONTROL) = 0;

    /* kill the watchdog */
    TIMREG(WDOG_CONTROL) = 0;

    /* save the timer frequency for later calculations */
    timer_freq = freq;

    register_int_handler(CPU_PRIV_TIMER, &platform_tick, NULL);
}

/* vim: set ts=4 sw=4 expandtab: */
