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
#include <platform/zynq.h>
#include "platform_p.h"

/* unused, arm_cortex_a9_timer does timer duty */

#if 0
/* driver for Cadence triple timer counter (TTC) */

#define LOCAL_TRACE 0

#define TIMREG(reg)   (*REG32(TTC0_BASE + (reg)))
#define TIMREG16(reg) (*REG16(TTC0_BASE + (reg)))
#define TIMREG8(reg)  (*REG8(TTC0_BASE + (reg)))

#define CLK_CTRL(n)     (0x00 + (n) * 4)
#define CNT_CTRL(n)     (0x0c + (n) * 4)
#define CNT_VAL(n)      (0x18 + (n) * 4)
#define INTERVAL_VAL(n) (0x24 + (n) * 4)
#define MATCH_1(n)      (0x30 + (n) * 4)
#define MATCH_2(n)      (0x3c + (n) * 4)
#define MATCH_3(n)      (0x48 + (n) * 4)
#define ISR(n)          (0x54 + (n) * 4)
#define IEN(n)          (0x60 + (n) * 4)
#define EVT_CTRL(n)     (0x6c + (n) * 4)
#define EVT(n)          (0x78 + (n) * 4)

static platform_timer_callback t_callback;

static volatile uint ticks = 0;
static lk_time_t periodic_interval;

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval)
{
    enter_critical_section();

    LTRACEF("callback %p, arg %p, interval %lu\n", callback, arg, interval);

    t_callback = callback;

    periodic_interval = interval;

    uint32_t ticks = periodic_interval * 1000; /* timer is running close to 1Mhz */
    ASSERT(ticks <= 0xffff);

    TIMREG(IEN(0)) = (1<<0); // interval interrupt
    TIMREG(INTERVAL_VAL(0)) = ticks;
    TIMREG(CNT_CTRL(0)) = (1<<5) | (1<<4) | (1<<1); // no wave, reset, interval mode

    unmask_interrupt(TTC0_A_INT);

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

    volatile uint32_t hole = TIMREG(ISR(0)); // ack the irq

    if (t_callback) {
        return t_callback(arg, current_time());
    } else {
        return INT_NO_RESCHEDULE;
    }
}

void platform_init_timer(void)
{
    /* disable timers */
    TIMREG(CNT_CTRL(0)) = 0x1;
    TIMREG(CNT_CTRL(1)) = 0x1;
    TIMREG(CNT_CTRL(2)) = 0x1;

    TIMREG(CLK_CTRL(0)) = (6 << 1) | 1; // prescale 133Mhz/(2^7) == 1039062Hz (close to 1Mhz)

    register_int_handler(TTC0_A_INT, &platform_tick, NULL);
    register_int_handler(TTC0_B_INT, &platform_tick, NULL);
    register_int_handler(TTC0_C_INT, &platform_tick, NULL);
}
#endif
