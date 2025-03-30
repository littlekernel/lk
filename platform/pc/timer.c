/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/timer.h>
#include <platform/pc.h>
#include "platform_p.h"
#include <arch/x86.h>

#define LOCAL_TRACE 0

// Deals with all of the various clock sources and event timers on the PC platform.

static enum clock_source {
    CLOCK_SOURCE_INITIAL,
    CLOCK_SOURCE_PIT,
    CLOCK_SOURCE_TSC,
    CLOCK_SOURCE_HPET,
} clock_source = CLOCK_SOURCE_INITIAL;

static const char *clock_source_name(void) {
    switch (clock_source) {
        case CLOCK_SOURCE_INITIAL:
            return "initial";
        case CLOCK_SOURCE_PIT:
            return "pit";
        case CLOCK_SOURCE_TSC:
            return "tsc";
        case CLOCK_SOURCE_HPET:
            return "hpet";
        default:
            return "unknown";
    }
}

lk_time_t current_time(void) {
    switch (clock_source) {
        case CLOCK_SOURCE_PIT:
            return pit_current_time();
        default:
            return 0;
    }
}

lk_bigtime_t current_time_hires(void) {
    switch (clock_source) {
        case CLOCK_SOURCE_PIT:
            return pit_current_time_hires();
        default:
            return 0;
    }
}

void pc_init_timer(unsigned int level) {
    LTRACE_ENTRY;

    pit_init();
    clock_source = CLOCK_SOURCE_PIT;
}

LK_INIT_HOOK(pc_timer, pc_init_timer, LK_INIT_LEVEL_VM);

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    return pit_set_periodic_timer(callback, arg, interval);
}

status_t platform_set_oneshot_timer(platform_timer_callback callback,
                                    void *arg, lk_time_t interval) {
    return pit_set_oneshot_timer(callback, arg, interval);
}

void platform_stop_timer(void) {
    pit_stop_timer();
}
