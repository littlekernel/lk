/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/timer.h>
#include <arch/vax.h>

static uint32_t ticks;
static platform_timer_callback cb;
static void *cb_arg;

// stubbed out timer
status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    // for the moment, assume the kernel is asking for 10ms
    DEBUG_ASSERT(interval == 10);

    cb = callback;
    cb_arg = arg;

    return NO_ERROR;
}

// fires once every 10ms
void vax_interval_timer(void) {
    ticks++;

    enum handler_return ret = cb(cb_arg, current_time());
    if (ret == INT_RESCHEDULE) {
        thread_preempt();
    }
}

void platform_early_timer_init(void) {
    // we can only assume there is a single 10ms interval timer available,
    // but go ahead and configure it using the full register interface, since it
    // wont hurt.

    // load the next count register with -10000 usecs
    mtpr(-10000, PR_NICR);

    // start the timer and clear any old state
    // clear error, clear interrupt, enable interrupt, load count, run
    mtpr((1<<31) | (1<<7) | (1<<6) | (1<<4) | (1<<0), PR_ICCS);
}

void platform_timer_init(void) {
}

lk_time_t current_time() {
    return ticks * 10;
}

lk_bigtime_t current_time_hires() {
    return ticks * 10000;
}

