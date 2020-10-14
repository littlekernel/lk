/*
 * Copyright (c) 2015 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <platform.h>
#include <nrfx.h>
#include <platform/timer.h>
#include <platform/clock.h>
#include <nrfx_clock.h>
#include <hal/nrf_rtc.h>
#include <sys/types.h>

/*
 * This uses the RTC1 module for the LK system tick instead of using the ARM
 * systick since systick is not available in all low power states.  This is also
 * not utilizing the nrfx rtc driver as it adds unneeded functionality and thus
 * overhead to the irq processing.  It is however using convenience functions
 * in the nrf hal library (nrf hal should not be confused with nrfx driver library)
 */

// compare channel used by our timer.
#define LK_RTC_CC_CHAN (0)

#define MULT_BY_32768(x)  ((x) << 15)
#define DIV_BY_32768(x) ((x) >> 15)

/*
 * base_counter is total number of ms elapsed in 49.15 format.  By choosing this
 * format we take advantage of the fact our oscillator is 32768Hz, allowing the
 * recurring math to be done with  shift by 15.  This preserves some precision in
 * the base_counter for use in current_time_hires.
 *
 * base_counter will overflow every 17,851 years.  Plan accordingly.
 */
static volatile uint64_t base_counter = 0;

//cycles of our clock per tick interval
static uint32_t rtc_counts_per_tick = 0;
static volatile uint32_t last_cc = 0;

//delta to be added to base counter (ms in 49.15 format) each lk tick
static uint32_t ms_scale = 0;

static platform_timer_callback cb;
static void *cb_args;

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    ASSERT(nrfx_clock_lfclk_is_running());
    cb = callback;
    cb_args = arg;
    // RTC clock frequency is always 32768Hz, so we take advantage of shifts vs. multiplicaiton
    rtc_counts_per_tick = MULT_BY_32768(interval) / 1000 ;
    ms_scale = rtc_counts_per_tick * 1000;

    // Stop the counter in case it was already running
    nrf_rtc_task_trigger(NRF_RTC1, NRF_RTC_TASK_STOP);

    // Clear the counter
    nrf_rtc_task_trigger(NRF_RTC1, NRF_RTC_TASK_CLEAR);

    // Set the first timer interval
    nrf_rtc_cc_set(NRF_RTC1, LK_RTC_CC_CHAN, rtc_counts_per_tick);

    // Run at full 32768Hz rate, no prescale
    nrf_rtc_prescaler_set(NRF_RTC1, 0);

    // Allow the CC channel to create interrupts when flagged
    nrf_rtc_int_enable(NRF_RTC1, RTC_CHANNEL_INT_MASK(LK_RTC_CC_CHAN));

    // In order for this event to wake the device from low power/sleep state, the
    // event must be enabled to do so.
    nrf_rtc_event_enable(NRF_RTC1, RTC_CHANNEL_INT_MASK(LK_RTC_CC_CHAN));

    // Clear the event state prior to enabling the irq to prevent spurious irq
    nrf_rtc_event_clear(NRF_RTC1, nrf_rtc_compare_event_get(LK_RTC_CC_CHAN));

    // Start the RTC counter.
    nrf_rtc_task_trigger(NRF_RTC1, NRF_RTC_TASK_START);

    NVIC_EnableIRQ(RTC1_IRQn);

    return NO_ERROR;
}

// time in msec
lk_time_t current_time(void) {
    uint64_t t;

    do {
        t = base_counter;
    } while (base_counter != t);

    return DIV_BY_32768(t);
}

// time in usec
lk_bigtime_t current_time_hires(void) {
    volatile uint64_t t;
    volatile uint32_t delta;

    do {
        // order of operations below is critical to catch if timer is updated
        // during the calculations.
        t = base_counter;
        delta = (nrf_rtc_counter_get(NRF_RTC1) - last_cc);
    } while ((t != base_counter));

    return DIV_BY_32768((t + delta * 1000) * 1000);
}

void nrf52_RTC1_IRQ(void) {
    // update to this point in time
    base_counter += ms_scale;
    arm_cm_irq_entry();

    nrf_rtc_event_clear(NRF_RTC1, nrf_rtc_compare_event_get(LK_RTC_CC_CHAN));
    // calculate time of next interrupt
    last_cc = nrf_rtc_cc_get(NRF_RTC1, LK_RTC_CC_CHAN);
    nrf_rtc_cc_set(NRF_RTC1, LK_RTC_CC_CHAN, last_cc + rtc_counts_per_tick);

    bool resched = false;
    if (cb) {
        lk_time_t now = DIV_BY_32768(base_counter);
        if (cb(cb_args, now) == INT_RESCHEDULE)
            resched = true;
    }
    arm_cm_irq_exit(resched);
}

void arm_cm_systick_init(uint32_t hz) {
    ASSERT(hz == 32768);
    //Try to enable the LF xtal oscillator
    status_t status = nrf52_clock_lfclk_enable(NRF_CLOCK_LFCLK_Xtal);
    if (status != NO_ERROR) {
        // If  xtal won't start (might not be present on target) then
        // use the internal RC oscillator which is always present.
        nrf52_clock_lfclk_enable(NRF_CLOCK_LFCLK_RC);
    }
}
