/*
 * Copyright (c) 2016 - 2020, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <nrfx.h>

#if NRFX_CHECK(NRFX_CLOCK_ENABLED)

#include <nrfx_clock.h>
#include <nrf_erratas.h>

#define NRFX_LOG_MODULE CLOCK
#include <nrfx_log.h>

#if NRFX_CHECK(NRFX_POWER_ENABLED)
extern bool nrfx_power_irq_enabled;
#endif

#if defined(CLOCK_LFCLKSRC_SRC_RC) || defined(__NRFX_DOXYGEN__)
    #define LF_SRC_RC CLOCK_LFCLKSRC_SRC_RC
#else
    #define LF_SRC_RC CLOCK_LFCLKSRC_SRC_LFRC
#endif

#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
    #if (NRF_CLOCK_HAS_CALIBRATION == 0)
        #error "Calibration is not available in the SoC that is used."
    #endif
    #if (NRFX_CLOCK_CONFIG_LF_SRC != LF_SRC_RC)
        #error "Calibration can be performed only for the RC Oscillator."
    #endif
#endif

#if !defined(USE_WORKAROUND_FOR_ANOMALY_132) && \
    (defined(NRF52832_XXAA) || defined(NRF52832_XXAB))
    // ANOMALY 132 - LFCLK needs to avoid frame from 66us to 138us after LFCLK stop. This solution
    //               applies delay of 138us before starting LFCLK.
    #define USE_WORKAROUND_FOR_ANOMALY_132 1

    // Convert time to cycles (nRF52832 is clocked with 64 MHz, use delay of 138 us).
    #define ANOMALY_132_DELAY_CYCLES (64UL * 138)
#endif

#if !defined(USE_WORKAROUND_FOR_ANOMALY_192) && \
    (defined(NRF52810_XXAA) || \
     defined(NRF52832_XXAA) || defined(NRF52832_XXAB) || \
     defined(NRF52840_XXAA))
    // Enable workaround for nRF52 anomaly 192 (LFRC oscillator frequency is wrong
    // after calibration, exceeding 500 ppm).
    #define USE_WORKAROUND_FOR_ANOMALY_192 1
#endif

#if !defined(USE_WORKAROUND_FOR_ANOMALY_201) && \
    (defined(NRF52810_XXAA) || \
     defined(NRF52832_XXAA) || defined(NRF52832_XXAB) || \
     defined(NRF52840_XXAA))
    // Enable workaround for nRF52 anomaly 201 (EVENTS_HFCLKSTARTED might be generated twice).
    #define USE_WORKAROUND_FOR_ANOMALY_201 1
#endif

#if defined(CLOCK_LFCLKSRC_SRC_Xtal)
    #define LF_SRC_LFXO CLOCK_LFCLKSRC_SRC_Xtal
#else
    #define LF_SRC_LFXO CLOCK_LFCLKSRC_SRC_LFXO
#endif

#if defined(NRF_CLOCK_USE_EXTERNAL_LFCLK_SOURCES)
    #define LF_SRC_XTAL_LOW  (CLOCK_LFCLKSRC_SRC_Xtal | \
                             (CLOCK_LFCLKSRC_EXTERNAL_Enabled << CLOCK_LFCLKSRC_EXTERNAL_Pos))
    #define LF_SRC_XTAL_FULL (CLOCK_LFCLKSRC_SRC_Xtal | \
                             (CLOCK_LFCLKSRC_BYPASS_Enabled   << CLOCK_LFCLKSRC_BYPASS_Pos) | \
                             (CLOCK_LFCLKSRC_EXTERNAL_Enabled << CLOCK_LFCLKSRC_EXTERNAL_Pos))
#else
    #define LF_SRC_XTAL_LOW  LF_SRC_LFXO
    #define LF_SRC_XTAL_FULL LF_SRC_LFXO
#endif

#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED) && \
    NRFX_CLOCK_CONFIG_LF_SRC != LF_SRC_LFXO && \
    NRFX_CLOCK_CONFIG_LF_SRC != LF_SRC_XTAL_LOW && \
    NRFX_CLOCK_CONFIG_LF_SRC != LF_SRC_XTAL_FULL
    #error "Two-stage LFXO start procedure enabled but LFCLK source is not set to LFXO!"
#endif

#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
typedef enum
{
    CAL_STATE_IDLE,
    CAL_STATE_CAL
} nrfx_clock_cal_state_t;
#endif

/**@brief CLOCK control block. */
typedef struct
{
    nrfx_clock_event_handler_t      event_handler;
    bool                            module_initialized; /*< Indicate the state of module */
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
    bool                            hfclk_started;      /*< Anomaly 201 workaround. */
#endif

#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
    volatile nrfx_clock_cal_state_t cal_state;
#endif
} nrfx_clock_cb_t;

static nrfx_clock_cb_t m_clock_cb;

/**
 * This variable is used to check whether common POWER_CLOCK common interrupt
 * should be disabled or not if @ref nrfx_power tries to disable the interrupt.
 */
#if NRFX_CHECK(NRFX_POWER_ENABLED)
bool nrfx_clock_irq_enabled;
#endif

#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_132)
/**
 * @brief Function for applying delay of 138us before starting LFCLK.
 */
static void nrfx_clock_anomaly_132(void)
{
    uint32_t cyccnt_inital;
    uint32_t core_debug;
    uint32_t dwt_ctrl;

    // Preserve DEMCR register to do not influence into its configuration. Enable the trace and
    // debug blocks. It is required to read and write data to DWT block.
    core_debug = CoreDebug->DEMCR;
    CoreDebug->DEMCR = core_debug | CoreDebug_DEMCR_TRCENA_Msk;

    // Preserve CTRL register in DWT block to do not influence into its configuration. Make sure
    // that cycle counter is enabled.
    dwt_ctrl = DWT->CTRL;
    DWT->CTRL = dwt_ctrl | DWT_CTRL_CYCCNTENA_Msk;

    // Store start value of cycle counter.
    cyccnt_inital = DWT->CYCCNT;

    // Delay required time.
    while ((DWT->CYCCNT - cyccnt_inital) < ANOMALY_132_DELAY_CYCLES)
    {}

    // Restore preserved registers.
    DWT->CTRL = dwt_ctrl;
    CoreDebug->DEMCR = core_debug;
}
#endif // NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_132)

nrfx_err_t nrfx_clock_init(nrfx_clock_event_handler_t event_handler)
{
    NRFX_ASSERT(event_handler);

    nrfx_err_t err_code = NRFX_SUCCESS;
    if (m_clock_cb.module_initialized)
    {
        err_code = NRFX_ERROR_ALREADY_INITIALIZED;
    }
    else
    {
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
        m_clock_cb.cal_state = CAL_STATE_IDLE;
#endif
        m_clock_cb.event_handler = event_handler;
        m_clock_cb.module_initialized = true;
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
        m_clock_cb.hfclk_started = false;
#endif
    }

    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

void nrfx_clock_enable(void)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);
    nrfx_power_clock_irq_init();
#if !NRFX_CHECK(NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED)
    nrf_clock_lf_src_set(NRF_CLOCK, (nrf_clock_lfclk_t)NRFX_CLOCK_CONFIG_LF_SRC);
#endif
#if NRF_CLOCK_HAS_HFCLKSRC
    nrf_clock_hf_src_set(NRF_CLOCK, NRF_CLOCK_HFCLK_HIGH_ACCURACY);
#endif
#if NRF_CLOCK_HAS_HFCLK192M
    nrf_clock_hfclk192m_src_set(NRF_CLOCK, (nrf_clock_hfclk_t)NRFX_CLOCK_CONFIG_HFCLK192M_SRC);
#endif
#if NRFX_CHECK(NRFX_POWER_ENABLED)
    nrfx_clock_irq_enabled = true;
#endif

    NRFX_LOG_INFO("Module enabled.");
}

void nrfx_clock_disable(void)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);
#if NRFX_CHECK(NRFX_POWER_ENABLED)
    NRFX_ASSERT(nrfx_clock_irq_enabled);
    if (!nrfx_power_irq_enabled)
#endif
    {
        NRFX_IRQ_DISABLE(nrfx_get_irq_number(NRF_CLOCK));
    }
    nrf_clock_int_disable(NRF_CLOCK, CLOCK_INTENSET_HFCLKSTARTED_Msk |
                                     CLOCK_INTENSET_LFCLKSTARTED_Msk |
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
                                     CLOCK_INTENSET_DONE_Msk |
#if NRF_HAS_CALIBRATION_TIMER
                                     CLOCK_INTENSET_CTTO_Msk |
#endif
#endif // NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
                          0);
#if NRFX_CHECK(NRFX_POWER_ENABLED)
    nrfx_clock_irq_enabled = false;
#endif
    NRFX_LOG_INFO("Module disabled.");
}

void nrfx_clock_uninit(void)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);
    nrfx_clock_stop(NRF_CLOCK_DOMAIN_LFCLK);
    nrfx_clock_stop(NRF_CLOCK_DOMAIN_HFCLK);
#if NRF_CLOCK_HAS_HFCLK192M
    nrfx_clock_stop(NRF_CLOCK_DOMAIN_HFCLK192M);
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
    nrfx_clock_stop(NRF_CLOCK_DOMAIN_HFCLKAUDIO);
#endif
    m_clock_cb.module_initialized = false;
    NRFX_LOG_INFO("Uninitialized.");
}

void nrfx_clock_start(nrf_clock_domain_t domain)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);
    switch (domain)
    {
        case NRF_CLOCK_DOMAIN_LFCLK:
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED)
            {
                nrf_clock_lfclk_t lfclksrc;
                if (nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK, &lfclksrc) &&
                    lfclksrc == NRFX_CLOCK_CONFIG_LF_SRC)
                {
                    // If the two-stage LFXO procedure has finished already
                    // use the configured LF clock source.
                    nrf_clock_lf_src_set(NRF_CLOCK, (nrf_clock_lfclk_t)NRFX_CLOCK_CONFIG_LF_SRC);
                }
                else
                {
                    // If the two-stage LFXO procedure hasn't started yet
                    // or the RC stage is in progress,
                    // use the RC oscillator as LF clock source.
                    nrf_clock_lf_src_set(NRF_CLOCK, NRF_CLOCK_LFCLK_RC);
                }
            }
#endif // NRFX_CHECK(NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED)
            nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_LFCLKSTARTED);
            nrf_clock_int_enable(NRF_CLOCK, NRF_CLOCK_INT_LF_STARTED_MASK);
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_132)
            nrfx_clock_anomaly_132();
#endif
            nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_LFCLKSTART);
            break;
        case NRF_CLOCK_DOMAIN_HFCLK:
            nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
            nrf_clock_int_enable(NRF_CLOCK, NRF_CLOCK_INT_HF_STARTED_MASK);
            nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTART);
            break;
#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLK192MSTARTED);
            nrf_clock_int_enable(NRF_CLOCK, NRF_CLOCK_INT_HF192M_STARTED_MASK);
            nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLK192MSTART);
            break;
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
        case NRF_CLOCK_DOMAIN_HFCLKAUDIO:
            nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKAUDIOSTARTED);
            nrf_clock_int_enable(NRF_CLOCK, NRF_CLOCK_INT_HFAUDIO_STARTED_MASK);
            nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKAUDIOSTART);
            break;
#endif
        default:
            NRFX_ASSERT(0);
            break;
    }
}

void nrfx_clock_stop(nrf_clock_domain_t domain)
{
    NRFX_ASSERT(m_clock_cb.module_initialized);
    switch (domain)
    {
        case NRF_CLOCK_DOMAIN_LFCLK:
            nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_LF_STARTED_MASK);
            nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_LFCLKSTARTED);
            nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_LFCLKSTOP);
            break;
        case NRF_CLOCK_DOMAIN_HFCLK:
            nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_HF_STARTED_MASK);
            nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
            nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKSTOP);
            break;
#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_HF192M_STARTED_MASK);
            nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLK192MSTARTED);
            nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLK192MSTOP);
            break;
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
        case NRF_CLOCK_DOMAIN_HFCLKAUDIO:
            nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_HFAUDIO_STARTED_MASK);
            nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKAUDIOSTARTED);
            nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_HFCLKAUDIOSTOP);
            break;
#endif
        default:
            NRFX_ASSERT(0);
            return;
    }

    bool stopped;
    if (domain == NRF_CLOCK_DOMAIN_HFCLK)
    {
        nrf_clock_hfclk_t clk_src = NRF_CLOCK_HFCLK_HIGH_ACCURACY;
        NRFX_WAIT_FOR((!nrfx_clock_is_running(domain, &clk_src) ||
                       (clk_src != NRF_CLOCK_HFCLK_HIGH_ACCURACY)), 10000, 1, stopped);
    }
    else
    {
        NRFX_WAIT_FOR(!nrfx_clock_is_running(domain, NULL), 10000, 1, stopped);
    }

    if (!stopped)
    {
        NRFX_LOG_ERROR("Failed to stop clock domain: %d.", domain);
    }

#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
    if (domain == NRF_CLOCK_DOMAIN_HFCLK)
    {
            m_clock_cb.hfclk_started = false;
    }
#endif
}

nrfx_err_t nrfx_clock_calibration_start(void)
{
    nrfx_err_t err_code = NRFX_SUCCESS;

#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
    nrf_clock_hfclk_t clk_src;
    if (!nrfx_clock_is_running(NRF_CLOCK_DOMAIN_HFCLK, &clk_src))
    {
        return NRFX_ERROR_INVALID_STATE;
    }

    if (clk_src != NRF_CLOCK_HFCLK_HIGH_ACCURACY)
    {
        return NRFX_ERROR_INVALID_STATE;
    }

    if (!nrfx_clock_is_running(NRF_CLOCK_DOMAIN_LFCLK, NULL))
    {
        return NRFX_ERROR_INVALID_STATE;
    }

    if (m_clock_cb.cal_state == CAL_STATE_IDLE)
    {
        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_DONE);
        nrf_clock_int_enable(NRF_CLOCK, NRF_CLOCK_INT_DONE_MASK);
        m_clock_cb.cal_state = CAL_STATE_CAL;
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_192)
        *(volatile uint32_t *)0x40000C34 = 0x00000002;
#endif
        nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_CAL);
    }
    else
    {
        err_code = NRFX_ERROR_BUSY;
    }
#endif // NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)

    NRFX_LOG_WARNING("Function: %s, error code: %s.",
                     __func__,
                     NRFX_LOG_ERROR_STRING_GET(err_code));
    return err_code;
}

nrfx_err_t nrfx_clock_is_calibrating(void)
{
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
    if (m_clock_cb.cal_state == CAL_STATE_CAL)
    {
        return NRFX_ERROR_BUSY;
    }
#endif
    return NRFX_SUCCESS;
}

void nrfx_clock_calibration_timer_start(uint8_t interval)
{
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED) && NRF_CLOCK_HAS_CALIBRATION_TIMER
    nrf_clock_cal_timer_timeout_set(NRF_CLOCK, interval);
    nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_CTTO);
    nrf_clock_int_enable(NRF_CLOCK, NRF_CLOCK_INT_CTTO_MASK);
    nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_CTSTART);
#endif
}

void nrfx_clock_calibration_timer_stop(void)
{
#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED) && NRF_CLOCK_HAS_CALIBRATION_TIMER
    nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_CTTO_MASK);
    nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_CTSTOP);
#endif
}

#if NRF_CLOCK_HAS_HFCLK_DIV || NRF_CLOCK_HAS_HFCLK192M
nrfx_err_t nrfx_clock_divider_set(nrf_clock_domain_t domain,
                                  nrf_clock_hfclk_div_t div)
{
    switch(domain)
    {
#if NRF_CLOCK_HAS_HFCLK_DIV
        case NRF_CLOCK_DOMAIN_HFCLK:
            switch (div)
            {
                case NRF_CLOCK_HFCLK_DIV_2:
                    NRFX_CRITICAL_SECTION_ENTER();
                    if (nrf53_errata_4())
                    {
                        __DSB();
                    }
                    nrf_clock_hfclk_div_set(NRF_CLOCK, div);
                    if (nrf53_errata_4())
                    {
                        *(volatile uint32_t *)0x5084450C = 0x0;
                        *(volatile uint32_t *)0x50026548 = 0x0;
                        *(volatile uint32_t *)0x50081EE4 = 0x0D;
                    }
                    NRFX_CRITICAL_SECTION_EXIT();
                    break;
                case NRF_CLOCK_HFCLK_DIV_1:
                    NRFX_CRITICAL_SECTION_ENTER();
                    if (nrf53_errata_4())
                    {
                        __DSB();
                        *(volatile uint32_t *)0x5084450C = 0x4040;
                        *(volatile uint32_t *)0x50026548 = 0x40;
                        *(volatile uint32_t *)0x50081EE4 = 0x4D;
                    }
                    nrf_clock_hfclk_div_set(NRF_CLOCK, div);
                    NRFX_CRITICAL_SECTION_EXIT();
                    break;
                default:
                    return NRFX_ERROR_INVALID_PARAM;
            }
            SystemCoreClockUpdate();
            return NRFX_SUCCESS;
#endif
#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            if (div > NRF_CLOCK_HFCLK_DIV_4)
            {
                return NRFX_ERROR_INVALID_PARAM;
            }
            else
            {
                nrf_clock_hfclk192m_div_set(NRF_CLOCK, div);
            }
            return NRFX_SUCCESS;
#endif
        default:
            NRFX_ASSERT(0);
            return NRFX_ERROR_NOT_SUPPORTED;
    }
}
#endif

void nrfx_clock_irq_handler(void)
{
    if (nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED))
    {
        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKSTARTED);
        NRFX_LOG_DEBUG("Event: NRF_CLOCK_EVENT_HFCLKSTARTED");
        nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_HF_STARTED_MASK);

#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_201)
        if (!m_clock_cb.hfclk_started)
        {
            m_clock_cb.hfclk_started = true;
            m_clock_cb.event_handler(NRFX_CLOCK_EVT_HFCLK_STARTED);
        }
#else
        m_clock_cb.event_handler(NRFX_CLOCK_EVT_HFCLK_STARTED);
#endif
    }
    if (nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_LFCLKSTARTED))
    {
        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_LFCLKSTARTED);
        NRFX_LOG_DEBUG("Event: NRF_CLOCK_EVENT_LFCLKSTARTED");

#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LFXO_TWO_STAGE_ENABLED)
        nrf_clock_lfclk_t lfclksrc;
        (void)nrf_clock_is_running(NRF_CLOCK, NRF_CLOCK_DOMAIN_LFCLK, &lfclksrc);
        if (lfclksrc == NRF_CLOCK_LFCLK_RC)
        {
            // After the LFRC oscillator start switch to external source.
            nrf_clock_lf_src_set(NRF_CLOCK, (nrf_clock_lfclk_t)NRFX_CLOCK_CONFIG_LF_SRC);
            nrf_clock_task_trigger(NRF_CLOCK, NRF_CLOCK_TASK_LFCLKSTART);
        }
        else
#endif
        {
            // After the LF clock external source start invoke user callback.
            nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_LF_STARTED_MASK);
            m_clock_cb.event_handler(NRFX_CLOCK_EVT_LFCLK_STARTED);
        }
    }

#if NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)
#if NRF_CLOCK_HAS_CALIBRATION_TIMER
    if (nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_CTTO))
    {
        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_CTTO);
        NRFX_LOG_DEBUG("Event: NRF_CLOCK_EVENT_CTTO");
        nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_CTTO_MASK);

        m_clock_cb.event_handler(NRFX_CLOCK_EVT_CTTO);
    }
#endif // NRF_CLOCK_HAS_CALIBRATION_TIMER

    if (nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_DONE))
    {
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_192)
        *(volatile uint32_t *)0x40000C34 = 0x00000000;
#endif
        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_DONE);
        NRFX_LOG_DEBUG("Event: NRF_CLOCK_EVENT_DONE");
        nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_DONE_MASK);
        m_clock_cb.cal_state = CAL_STATE_IDLE;
        m_clock_cb.event_handler(NRFX_CLOCK_EVT_CAL_DONE);
    }
#endif // NRFX_CHECK(NRFX_CLOCK_CONFIG_LF_CAL_ENABLED)

#if NRF_CLOCK_HAS_HFCLKAUDIO
    if (nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKAUDIOSTARTED))
    {
        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLKAUDIOSTARTED);
        NRFX_LOG_DEBUG("Event: NRF_CLOCK_EVENT_HFCLKAUDIOSTARTED");
        nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_HFAUDIO_STARTED_MASK);

        m_clock_cb.event_handler(NRFX_CLOCK_EVT_HFCLKAUDIO_STARTED);
    }
#endif

#if NRF_CLOCK_HAS_HFCLK192M
    if (nrf_clock_event_check(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLK192MSTARTED))
    {
        nrf_clock_event_clear(NRF_CLOCK, NRF_CLOCK_EVENT_HFCLK192MSTARTED);
        NRFX_LOG_DEBUG("Event: NRF_CLOCK_EVENT_HFCLK192MSTARTED");
        nrf_clock_int_disable(NRF_CLOCK, NRF_CLOCK_INT_HF192M_STARTED_MASK);

        m_clock_cb.event_handler(NRFX_CLOCK_EVT_HFCLK192M_STARTED);
    }
#endif
}

#endif // NRFX_CHECK(NRFX_CLOCK_ENABLED)
