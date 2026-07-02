//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <lk/reg.h>
#include <lk/err.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <sys/types.h>

#include "platform_p.h"


void platform_early_init(void) {
    sun4m_intc_early_init();
    sun4m_timer_early_init();
}

void platform_init(void) {
    sun4m_intc_init();
    sun4m_timer_init();
}

/* timer stubs */
status_t platform_set_oneshot_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    return NO_ERROR;
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    return NO_ERROR;
}

void platform_stop_timer(void) {
}

lk_time_t current_time(void) {
    return 0;
}

lk_bigtime_t current_time_hires(void) {
    return 0;
}
