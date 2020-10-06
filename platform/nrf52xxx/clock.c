/*
 * Copyright (c) 2020 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <sys/types.h>
#include <lk/err.h>
#include <kernel/event.h>
#include <nrfx.h>
#include <nrfx_clock.h>
#include <hal/nrf_clock.h>
#include <platform/clock.h>


event_t hf_clk_evt = EVENT_INITIAL_VALUE(hf_clk_evt, false, 0);
event_t lf_clk_evt = EVENT_INITIAL_VALUE(lf_clk_evt, false, 0);

void nrf52_POWER_CLOCK_IRQ(void) {
    arm_cm_irq_entry();
    nrfx_clock_irq_handler();
    arm_cm_irq_exit(true);
}

// Handler runs in interrupt context
void nrf52_clock_handler(nrfx_clock_evt_type_t event) {
    switch (event) {
        case NRFX_CLOCK_EVT_HFCLK_STARTED :
            event_signal(&hf_clk_evt, false);
            break;
        case NRFX_CLOCK_EVT_LFCLK_STARTED :
            event_signal(&lf_clk_evt, false);
            break;
        default:
            break;
    }
}

status_t nrf52_clock_init(void) {
    status_t status = nrfx_clock_init(nrf52_clock_handler);
    if (status == NO_ERROR) {
        nrfx_clock_enable();
    }
    return status;
}

status_t nrf52_clock_hf_use_xtal_source(void) {
    // Check if already running on hfclk xtal
    if (nrfx_clock_hfclk_is_running()) {
        return NO_ERROR;
    }
    event_unsignal(&hf_clk_evt);
    nrfx_clock_hfclk_start();
    if (event_wait_timeout(&hf_clk_evt, 100) != NO_ERROR) {
        dprintf(CRITICAL, "Timeout waiting for hf source\n");
        return ERR_TIMED_OUT;
    }
    return NO_ERROR;
}

// Switch to using the internal 64MHz oscillator for HF clock
void nrf52_clock_hf_use_internal_source(void) {
    //The clock controller will automatically switch to the internal
    // oscillator as the source when the external xtal is stopped.
    // this nrfx function is poorly named as it does not stop the hf clock
    // but only stops the external xtal oscillator.
    nrfx_clock_hfclk_stop();
}

status_t nrf52_clock_lfclk_enable(nrf_clock_lfclk_t src) {
    if (nrfx_clock_lfclk_is_running()) {
        nrf_clock_lfclk_t current_src = nrf_clock_lf_actv_src_get(NRF_CLOCK);
        if (current_src == src) {
            //Already running on the requested source, do nothing.
            return NO_ERROR;
        }
        // If we make it here, it is running, but not right source, so shut it down to change
        nrfx_clock_lfclk_stop();
    }
    nrf_clock_lf_src_set(NRF_CLOCK, src);
    event_unsignal(&lf_clk_evt);
    nrfx_clock_lfclk_start();
    if (event_wait_timeout(&lf_clk_evt, 100) != NO_ERROR) {
        dprintf(CRITICAL, "Timeout waiting for lf clock start\n");
        return ERR_TIMED_OUT;
    }
    return NO_ERROR;
}

