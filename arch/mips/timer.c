/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/mips.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <lk/debug.h>
#include <assert.h>
#include <stdint.h>
#include <lk/bits.h>
#include <arch/ops.h>
#include <platform.h>
#include <platform/timer.h>

#define LOCAL_TRACE 0

static volatile uint64_t ticks;
static volatile uint32_t last_compare_set;

static uint32_t tick_rate;
static uint32_t tick_rate_mhz;

static lk_time_t tick_interval_ms;
static lk_bigtime_t tick_interval_us;
static uint32_t tick_interval;

static platform_timer_callback cb;
static void *cb_args;

enum handler_return mips_timer_irq(void) {
    LTRACEF("count   0x%x\n", mips_read_c0_count());
    LTRACEF("compare 0x%x\n", mips_read_c0_compare());

    /* reset it for the next interval */
retry:
    ticks++;
    last_compare_set += tick_interval;
    uint32_t count = mips_read_c0_count();
    if (unlikely(TIME_GT(count, last_compare_set))) {
        /* if it took us too long to get to this irq, make sure it fires immediately */
        //printf("took too long to service timer irq! %u %u\n", count, last_compare_set);
        goto retry;
        //mips_write_c0_compare(mips_read_c0_count() + tick_rate_mhz);
    } else {
        mips_write_c0_compare(last_compare_set);
    }

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (cb) {
        lk_time_t now = current_time();
        ret = cb(cb_args, now);
    }

    return ret;
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    TRACEF("callback %p, arg %p, interval %u\n", callback, arg, interval);

    DEBUG_ASSERT(interval > 0);
    DEBUG_ASSERT(tick_rate != 0 && tick_rate_mhz != 0);

    cb = callback;
    cb_args = arg;

    tick_interval_ms = interval;
    tick_interval_us = interval * 1000;
    tick_interval = interval * (tick_rate / 1000);

    uint32_t now = mips_read_c0_count();
    last_compare_set = now + tick_interval;
    mips_write_c0_compare(last_compare_set);

    // enable the counter
    mips_write_c0_cause(mips_read_c0_cause() & ~(1<<27));

    return NO_ERROR;
}

lk_time_t current_time(void) {
    uint64_t t;
    uint32_t last_compare;
    uint32_t delta;

    /* sample the tick counter, the last compare register set, and the current count atomically */
    do {
        t = ticks;
        last_compare = last_compare_set;
        delta = mips_read_c0_count();
    } while (ticks != t || last_compare_set != last_compare);

    /* convert ticks to msec */
    delta = (delta - last_compare - tick_interval) / (tick_rate_mhz * 1000);
    lk_time_t res = (t * tick_interval_ms) + delta;

    return res;
}

lk_bigtime_t current_time_hires(void) {
    uint64_t t;
    uint32_t last_compare;
    uint32_t delta;

    /* sample the tick counter, the last compare register set, and the current count atomically */
    do {
        t = ticks;
        last_compare = last_compare_set;
        delta = mips_read_c0_count();
    } while (ticks != t);

    /* convert ticks to usec */
    delta = (delta - last_compare - tick_interval) / tick_rate_mhz;
    lk_bigtime_t res = (t * tick_interval_us) + delta;

    return res;
}

void mips_init_timer(uint32_t freq) {
    tick_rate = freq;
    tick_rate_mhz = freq / 1000000;

    // disable the counter
    mips_write_c0_cause(mips_read_c0_cause() | (1<<27));

    // figure out which interrupt the timer is set to
    uint32_t ipti = BITS_SHIFT(mips_read_c0_intctl(), 31, 29);
    if (ipti >= 2) {
        mips_enable_irq(ipti);
    }
}

