/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "platform_p.h"

#include <assert.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/virt.h>
#include <platform/timer.h>

#define LOCAL_TRACE 0

static platform_timer_callback t_callback;

static volatile uint ticks = 0;
static lk_time_t periodic_interval;

void goldfish_rtc_early_init(void) {
}

void goldfish_rtc_init(void) {
}

lk_bigtime_t current_time_hires(void) {
    static lk_bigtime_t bt = 0;
    return ++bt;
}

lk_time_t current_time(void) {
    static lk_time_t time = 0;
    return ++time;
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("callback %p, arg %p, interval %u\n", callback, arg, interval);

    t_callback = callback;

    periodic_interval = interval;

#if 0
    uint32_t ticks = periodic_interval * 1000; /* timer is running close to 1Mhz */
    ASSERT(ticks <= 0xffff);

    TIMREG(IEN(0)) = (1<<0); // interval interrupt
    TIMREG(INTERVAL_VAL(0)) = ticks;
    TIMREG(CNT_CTRL(0)) = (1<<5) | (1<<4) | (1<<1); // no wave, reset, interval mode

    unmask_interrupt(TTC0_A_INT);
#endif

    return NO_ERROR;
}


