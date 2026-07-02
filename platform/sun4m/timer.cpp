//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <assert.h>
#include <platform/time.h>
#include <platform/timer.h>
#include <platform/interrupts.h>
#include <sys/types.h>

#include "platform_p.h"

#define LOCAL_TRACE 1

// sun4m timer

namespace {

// each cpu offset is 0x1000, only handle UP for now
constexpr uint64_t TIMER_PERCPU_PHYS = 0xff1300000ULL;

// free running timer, running in 500nS increments, so 2MHz
constexpr uint32_t TIMER_FREQ = 2000000;
constexpr lk_time_t TIMER_PERIOD = 10;
constexpr uint32_t TIMER_TICKS_PER_PERIOD = TIMER_FREQ / (1000 / TIMER_PERIOD); // 10mS period

uint32_t timer_ticks = 0;
platform_timer_callback timer_callback = nullptr;
void *timer_callback_arg = nullptr;

enum {
    TIMER_LIMIT_REG = 0,
    TIMER_COUNTER_REG = 4,
    TIMER_LIMIT_NO_RESET_REG = 8,
    TIMER_USER_START_STOP_REG = 0xc,
};

// read the current counter register
uint32_t read_timer_counter() {
    uint32_t cnt = sparc_read_physical_32(TIMER_PERCPU_PHYS + TIMER_COUNTER_REG);
    bool wrap = false;
    if (cnt & 0x80000000) {
        // counter has wrapped, read the limit register to clear the wrap
        wrap = true;
    }
    cnt = (cnt >> 8);
    if (wrap) {
        // add one more interval to the ticks, since the counter has wrapped
        cnt += TIMER_TICKS_PER_PERIOD;
    }
    return cnt;
}

} // anonymous namespace

void sun4m_timer_early_init(void) {
    // set the timer limit to 10mS, and start the timer
    sparc_write_physical_32(TIMER_PERCPU_PHYS + TIMER_LIMIT_REG, TIMER_FREQ / 100);

    // clear the counter
#if 0
    for (;;) {
        printf("TIMER_COUNTER_REG: 0x%x\n",
               sparc_read_physical_32(TIMER_PERCPU_PHYS + TIMER_COUNTER_REG));
        printf("current_time_hires: %llu\n", current_time_hires());
    }
#endif
}

void sun4m_timer_init(void) {
    // nothing to do here
}

handler_return sun4m_timer_irq() {
    handler_return ret = INT_NO_RESCHEDULE;

    // read the current limit register and check if the wrap bit is set,
    // if so, increment the tick count and call our callback if one is registered.
    uint32_t limit = sparc_read_physical_32(TIMER_PERCPU_PHYS + TIMER_LIMIT_REG);
    if (limit & 0x80000000) {
        timer_ticks++;
        if (timer_callback) {
            if (timer_callback(timer_callback_arg, current_time()) == INT_RESCHEDULE) {
                ret = INT_RESCHEDULE;
            }
        }
    }
    return ret;
}

status_t platform_set_oneshot_timer(platform_timer_callback callback, void *arg,
                                    lk_time_t interval) {
    PANIC_UNIMPLEMENTED;
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg,
                                     lk_time_t interval) {
    // we are currently hard coded to to a 10S period.
    ASSERT(interval == TIMER_PERIOD);
    TRACEF("callback %p, arg %p, interval %u\n", callback, arg, interval);

    // remember the callback

    return NO_ERROR;
}

void platform_stop_timer(void) {}

lk_time_t current_time(void) {
    lk_time_t time = timer_ticks * TIMER_TICKS_PER_PERIOD;
    uint32_t counter = read_timer_counter();
    time += counter / 200; // 500nS increments
    return time;
}

lk_bigtime_t current_time_hires(void) {
    lk_time_t time = timer_ticks * TIMER_TICKS_PER_PERIOD * 1000;

    uint32_t counter = read_timer_counter();
    time += counter * 2;
    return time;
}
