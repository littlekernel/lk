/*
 * Copyright (c) 2015 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <arch/arm/cm.h>
#include <platform.h>
#include <platform/nrf51.h>
#include <platform/system_nrf51.h>
#include <platform/timer.h>
#include <sys/types.h>


static volatile uint64_t current_ticks;
static uint32_t tick_rate = 0;
static uint32_t tick_rate_mhz = 0;
static lk_time_t tick_interval_ms;

static platform_timer_callback cb;
static void *cb_args;

typedef enum handler_return (*platform_timer_callback)(void *arg, lk_time_t now);

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {

    cb = callback;
    cb_args = arg;

    tick_interval_ms = interval;

    uint32_t ticks = tick_rate / ( 1000 / interval );

    NRF_CLOCK->LFCLKSRC =  CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;

    NRF_RTC1->PRESCALER = ticks;
    NRF_RTC1->INTENSET  = RTC_INTENSET_TICK_Enabled << RTC_INTENSET_TICK_Pos;

    NRF_RTC1->EVENTS_TICK = 0;
    NRF_RTC1->TASKS_START = 1;
    NVIC_EnableIRQ(RTC1_IRQn);

    return NO_ERROR;
}

lk_time_t current_time(void) {
    uint64_t t;

    do {
        t = current_ticks;
    } while (current_ticks != t);

    return t * tick_interval_ms;
}

lk_bigtime_t current_time_hires(void) {
    return current_time() * 1000;
}

void nrf51_RTC1_IRQ(void) {
    current_ticks++;
    arm_cm_irq_entry();

    NRF_RTC1->EVENTS_TICK = 0;

    bool resched = false;
    if (cb) {
        lk_time_t now = current_time();
        if (cb(cb_args, now) == INT_RESCHEDULE)
            resched = true;
    }
    arm_cm_irq_exit(resched);
}

void arm_cm_systick_init(uint32_t mhz) {
    tick_rate = mhz;
    tick_rate_mhz = mhz / 1000000;
}

