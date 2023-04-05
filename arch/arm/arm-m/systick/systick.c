/*
 * Copyright (c) 2012-2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/*
 * Generic systick timer support for providing system time (current_time(), current_time_hires()),
 * and a monotonic timer for the kernel.
 */

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <lk/debug.h>
#include <assert.h>
#include <lk/trace.h>
#include <lk/err.h>
#include <kernel/thread.h>
#include <arch/arm.h>
#include <arch/arm/cm.h>
#include <platform.h>
#include <platform/timer.h>

#define LOCAL_TRACE 0

static volatile uint64_t current_ticks;
static uint32_t tick_rate = 0;
static uint32_t tick_rate_mhz = 0;
static lk_time_t tick_interval_ms;
static lk_bigtime_t tick_interval_us;
static bool systick_configured;
static uint32_t reload;

static platform_timer_callback cb;
static void *cb_args;

static void arm_cm_systick_set_periodic(lk_time_t period) {
    LTRACEF("clk_freq %u, period %u\n", tick_rate, (uint)period);

    uint32_t ticks = tick_rate / (1000 / period);
    LTRACEF("ticks %d\n", ticks);

    SysTick->LOAD = reload = (ticks - 1) & SysTick_LOAD_RELOAD_Msk;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    systick_configured = true;
}

static void arm_cm_systick_cancel_periodic(void) {
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

/* main systick irq handler */
void _systick(void) {
    arm_cm_irq_entry();

    if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) {
        current_ticks++;
    }

    bool resched = false;
    if (cb) {
        lk_time_t now = current_time();
        if (cb(cb_args, now) == INT_RESCHEDULE)
            resched = true;
    }

    arm_cm_irq_exit(resched);
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("callback %p, arg %p, interval %u\n", callback, arg, interval);

    DEBUG_ASSERT(tick_rate != 0 && tick_rate_mhz != 0);

    cb = callback;
    cb_args = arg;

    tick_interval_ms = interval;
    tick_interval_us = interval * 1000;
    arm_cm_systick_set_periodic(interval);

    return NO_ERROR;
}

/* Helper to atomically get the global ticks and the current counter value */
static void systick_get_ticks_val(uint64_t *ticks, uint32_t *val) {
    spin_lock_saved_state_t state;
    arch_interrupt_save(&state, SPIN_LOCK_FLAG_INTERRUPTS);

    *val = SysTick->VAL;
    if (unlikely(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)) {
        current_ticks++;
        *val = SysTick->VAL;
    }

    *ticks = current_ticks;

    arch_interrupt_restore(state, SPIN_LOCK_FLAG_INTERRUPTS);
}

lk_time_t current_time(void) {
    if (!systick_configured)
        return 0;

    uint32_t val;
    uint64_t ticks;
    systick_get_ticks_val(&ticks, &val);

    /* convert ticks to msec */
    uint32_t delta = (reload - val) / (tick_rate_mhz * 1000);
    lk_time_t res = (ticks * tick_interval_ms) + delta;

    return res;
}

lk_bigtime_t current_time_hires(void) {
    if (!systick_configured)
        return 0;

    uint32_t val;
    uint64_t ticks;
    systick_get_ticks_val(&ticks, &val);

    /* convert ticks to usec */
    uint32_t delta = (reload - val) / tick_rate_mhz;
    lk_bigtime_t res = (ticks * tick_interval_us) + delta;

    return res;
}

void arm_cm_systick_init(uint32_t mhz) {
    tick_rate = mhz;
    tick_rate_mhz = mhz / 1000000;
}
