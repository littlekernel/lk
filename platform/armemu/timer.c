/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <lk/err.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <platform/armemu.h>
#include "platform_p.h"

static platform_timer_callback t_callback;

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    t_callback = callback;

    *REG(PIT_CLEAR) = 1;
    *REG(PIT_INTERVAL) = interval;
    *REG(PIT_START_PERIODIC) = 1;

    unmask_interrupt(INT_PIT);

    return NO_ERROR;
}

lk_bigtime_t current_time_hires(void) {
    lk_bigtime_t time;
    *REG(SYSINFO_TIME_LATCH) = 1;
    time = *REG(SYSINFO_TIME_SECS) * 1000000ULL;
    time += *REG(SYSINFO_TIME_USECS);

    return time;
}

lk_time_t current_time(void) {
    lk_time_t time;
    *REG(SYSINFO_TIME_LATCH) = 1;
    time = *REG(SYSINFO_TIME_SECS) * 1000;
    time += *REG(SYSINFO_TIME_USECS) / 1000;

    return time;
}

static enum handler_return platform_tick(void *arg) {
    *REG(PIT_CLEAR_INT) = 1;
    if (t_callback) {
        return t_callback(arg, current_time());
    } else {
        return INT_NO_RESCHEDULE;
    }
}

void platform_init_timer(void) {
    register_int_handler(INT_PIT, &platform_tick, NULL);
}

