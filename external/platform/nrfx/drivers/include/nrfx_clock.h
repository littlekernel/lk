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

#ifndef NRFX_CLOCK_H__
#define NRFX_CLOCK_H__

#include <nrfx.h>
#include <hal/nrf_clock.h>
#include <nrfx_power_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_clock CLOCK driver
 * @{
 * @ingroup nrf_clock
 * @brief   CLOCK peripheral driver.
 */

/** @brief Clock events. */
typedef enum
{
    NRFX_CLOCK_EVT_HFCLK_STARTED,      ///< HFCLK has been started.
    NRFX_CLOCK_EVT_LFCLK_STARTED,      ///< LFCLK has been started.
    NRFX_CLOCK_EVT_CTTO,               ///< Calibration timeout.
    NRFX_CLOCK_EVT_CAL_DONE,           ///< Calibration has been done.
    NRFX_CLOCK_EVT_HFCLKAUDIO_STARTED, ///< HFCLKAUDIO has been started.
    NRFX_CLOCK_EVT_HFCLK192M_STARTED,  ///< HFCLK192M has been started.
} nrfx_clock_evt_type_t;

/**
 * @brief Clock event handler.
 *
 * @param[in] event Event.
 */
typedef void (*nrfx_clock_event_handler_t)(nrfx_clock_evt_type_t event);

/**
 * @brief Function for initializing internal structures in the nrfx_clock module.
 *
 * After initialization, the module is in power off state (clocks are not started).
 *
 * @param[in] event_handler Event handler provided by the user.
 *                          Must not be NULL.
 *
 * @retval NRFX_SUCCESS                   The procedure is successful.
 * @retval NRFX_ERROR_ALREADY_INITIALIZED The driver is already initialized.
 */
nrfx_err_t nrfx_clock_init(nrfx_clock_event_handler_t  event_handler);

/** @brief Function for enabling interrupts in the clock module. */
void nrfx_clock_enable(void);

/** @brief Function for disabling interrupts in the clock module. */
void nrfx_clock_disable(void);

/** @brief Function for uninitializing the clock module. */
void nrfx_clock_uninit(void);

/**
 * @brief Function for starting the specified clock domain.
 *
 * @param[in] domain Clock domain.
 */
void nrfx_clock_start(nrf_clock_domain_t domain);

/**
 * @brief Function for stopping the specified clock domain.
 *
 * @param[in] domain Clock domain.
 */
void nrfx_clock_stop(nrf_clock_domain_t domain);

/**
 * @brief Function for checking the specified clock domain state.
 *
 * XTAL source is assumed for domains with multiple sources.
 *
 * @param[in]  domain    Clock domain.
 * @param[out] p_clk_src Pointer to a clock source that is running. Set to NULL if not needed.
 *                       Ignored for HFCLKAUDIO domain. Variable pointed by @p p_clk_src
 *                       must be of either @ref nrf_clock_lfclk_t type for LFCLK
 *                       or @ref nrf_clock_hfclk_t type for HFCLK and HFCLK192M.
 *
 * @retval true  The clock domain is running.
 * @retval false The clock domain is not running.
 */
NRFX_STATIC_INLINE bool nrfx_clock_is_running(nrf_clock_domain_t domain, void * p_clk_src);

#if NRF_CLOCK_HAS_HFCLK_DIV || NRF_CLOCK_HAS_HFCLK_192M
/**
 * @brief Function for setting the specified clock domain divider.
 *
 * @param[in] domain Clock domain.
 * @param[in] div    New divider for the clock domain.
 *
 * @retval NRFX_SUCCESS             Divider successfully set.
 * @retval NRFX_ERROR_NOT_SUPPORTED Domain does not support setting the divider.
 * @retval NRFX_ERROR_INVALID_PARAM Divider not supported by the specified domain.
 */
nrfx_err_t nrfx_clock_divider_set(nrf_clock_domain_t    domain,
                                  nrf_clock_hfclk_div_t div);

/**
 * @brief Function for getting the specified clock domain divider.
 *
 * @param[in] domain Clock domain.
 *
 * @return Current divider for the specified clock domain.
 */

NRFX_STATIC_INLINE nrf_clock_hfclk_div_t nrfx_clock_divider_get(nrf_clock_domain_t domain);
#endif

/**
 * @brief Function for starting the LFCLK.
 *
 * @note This function is deprecated. Use @ref nrfx_clock_start instead.
 */
NRFX_STATIC_INLINE void nrfx_clock_lfclk_start(void);

/**
 * @brief Function for stopping the LFCLK.
 *
 * @note This function is deprecated. Use @ref nrfx_clock_stop instead.
 */
NRFX_STATIC_INLINE void nrfx_clock_lfclk_stop(void);

/**
 * @brief Function for checking the LFCLK state.
 *
 * @note This function is deprecated. Use @ref nrfx_clock_is_running instead.
 *
 * @retval true  The LFCLK is running.
 * @retval false The LFCLK is not running.
 */
NRFX_STATIC_INLINE bool nrfx_clock_lfclk_is_running(void);

/**
 * @brief Function for starting the high-accuracy source HFCLK.
 *
 * @note This function is deprecated. Use @ref nrfx_clock_start instead.
 */
NRFX_STATIC_INLINE void nrfx_clock_hfclk_start(void);

/**
 * @brief Function for stopping the external high-accuracy source HFCLK.
 *
 * @note This function is deprecated. Use @ref nrfx_clock_stop instead.
 */
NRFX_STATIC_INLINE void nrfx_clock_hfclk_stop(void);

/**
 * @brief Function for checking the HFCLK state.
 *
 * @note This function is deprecated. Use @ref nrfx_clock_is_running instead.
 *
 * @retval true  The HFCLK is running (XTAL source).
 * @retval false The HFCLK is not running.
 */
NRFX_STATIC_INLINE bool nrfx_clock_hfclk_is_running(void);


#if NRF_CLOCK_HAS_HFCLKAUDIO
/**
 * @brief Function for setting the HFCLKAUDIO configuration.
 *
 * The frequency of HFCLKAUDIO ranges from 10.666 MHz to 13.333 MHz in 40.7 Hz steps.
 * To calculate @p freq_value corresponding to the chosen frequency, use the following equation:
 * FREQ_VALUE = 2^16 * ((12 * f_out / 32M) - 4)
 *
 * @warning Chosen frequency must fit in 11.176 MHz - 11.402 MHz or 12.165 MHz - 12.411 MHz
 *          frequency bands.
 *
 * @param[in] freq_value New FREQ_VALUE for HFCLKAUDIO.
 */
NRFX_STATIC_INLINE void nrfx_clock_hfclkaudio_config_set(uint16_t freq_value);

/**
 * @brief Function for getting the HFCLKAUDIO configuration.
 *
 * The frequency of HFCLKAUDIO ranges from 10.666 MHz to 13.333 MHz in 40.7 Hz steps.
 * To calculate frequency corresponding to the returned FREQ_VALUE, use the following equation:
 * f_out = 32M * (4 + FREQ_VALUE * 2^(-16))/12
 *
 * @return Current value of FREQ_VALUE for HFCLKAUDIO.
 */
NRFX_STATIC_INLINE uint16_t nrfx_clock_hfclkaudio_config_get(void);

#endif

/**
 * @brief Function for starting the calibration of internal LFCLK.
 *
 * This function starts the calibration process. The process cannot be aborted. LFCLK and HFCLK
 * must be running before this function is called.
 *
 * @retval NRFX_SUCCESS             The procedure is successful.
 * @retval NRFX_ERROR_INVALID_STATE The low-frequency of high-frequency clock is off.
 * @retval NRFX_ERROR_BUSY          Clock is in the calibration phase.
 */
nrfx_err_t nrfx_clock_calibration_start(void);

/**
 * @brief Function for checking if calibration is in progress.
 *
 * This function indicates that the system is in calibration phase.
 *
 * @retval NRFX_SUCCESS    The procedure is successful.
 * @retval NRFX_ERROR_BUSY Clock is in the calibration phase.
 */
nrfx_err_t nrfx_clock_is_calibrating(void);

/**
 * @brief Function for starting calibration timer.
 *
 * @param[in] interval Time after which the CTTO event and interrupt will be generated (in 0.25 s units).
 */
void nrfx_clock_calibration_timer_start(uint8_t interval);

/** @brief Function for stopping the calibration timer. */
void nrfx_clock_calibration_timer_stop(void);

/**@brief Function for returning a requested task address for the clock driver module.
 *
 * @param[in] task One of the peripheral tasks.
 *
 * @return Task address.
 */
NRFX_STATIC_INLINE uint32_t nrfx_clock_ppi_task_addr(nrf_clock_task_t task);

/**@brief Function for returning a requested event address for the clock driver module.
 *
 * @param[in] event One of the peripheral events.
 *
 * @return Event address.
 */
NRFX_STATIC_INLINE uint32_t nrfx_clock_ppi_event_addr(nrf_clock_event_t event);

#ifndef NRFX_DECLARE_ONLY

#if NRF_CLOCK_HAS_HFCLK_DIV || NRF_CLOCK_HAS_HFCLK_192M
NRFX_STATIC_INLINE nrf_clock_hfclk_div_t nrfx_clock_divider_get(nrf_clock_domain_t domain)
{
    switch (domain)
    {
#if NRF_CLOCK_HAS_HFCLK_DIV
        case NRF_CLOCK_DOMAIN_HFCLK:
            return nrf_clock_hfclk_div_get(NRF_CLOCK);
#endif
#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            return nrf_clock_hfclk192m_div_get(NRF_CLOCK);
#endif
        default:
            NRFX_ASSERT(0);
            return 0;
    }
}
#endif // NRF_CLOCK_HAS_HFCLK_DIV || NRF_CLOCK_HAS_HFCLK_192M

NRFX_STATIC_INLINE void nrfx_clock_lfclk_start(void)
{
    nrfx_clock_start(NRF_CLOCK_DOMAIN_LFCLK);
}

NRFX_STATIC_INLINE void nrfx_clock_lfclk_stop(void)
{
    nrfx_clock_stop(NRF_CLOCK_DOMAIN_LFCLK);
}

NRFX_STATIC_INLINE void nrfx_clock_hfclk_start(void)
{
    nrfx_clock_start(NRF_CLOCK_DOMAIN_HFCLK);
}

NRFX_STATIC_INLINE void nrfx_clock_hfclk_stop(void)
{
    nrfx_clock_stop(NRF_CLOCK_DOMAIN_HFCLK);
}

NRFX_STATIC_INLINE uint32_t nrfx_clock_ppi_task_addr(nrf_clock_task_t task)
{
    return nrf_clock_task_address_get(NRF_CLOCK, task);
}

NRFX_STATIC_INLINE uint32_t nrfx_clock_ppi_event_addr(nrf_clock_event_t event)
{
    return nrf_clock_event_address_get(NRF_CLOCK, event);
}

NRFX_STATIC_INLINE bool nrfx_clock_is_running(nrf_clock_domain_t domain, void * p_clk_src)
{
    return nrf_clock_is_running(NRF_CLOCK, domain, p_clk_src);
}

NRFX_STATIC_INLINE bool nrfx_clock_hfclk_is_running(void)
{
    nrf_clock_hfclk_t clk_src;
    bool ret = nrfx_clock_is_running(NRF_CLOCK_DOMAIN_HFCLK, &clk_src);
    return (ret && (clk_src == NRF_CLOCK_HFCLK_HIGH_ACCURACY));
}

NRFX_STATIC_INLINE bool nrfx_clock_lfclk_is_running(void)
{
    return nrfx_clock_is_running(NRF_CLOCK_DOMAIN_LFCLK, NULL);
}

#if NRF_CLOCK_HAS_HFCLKAUDIO

NRFX_STATIC_INLINE void nrfx_clock_hfclkaudio_config_set(uint16_t freq_value)
{
    nrf_clock_hfclkaudio_config_set(NRF_CLOCK, freq_value);
}

NRFX_STATIC_INLINE uint16_t nrfx_clock_hfclkaudio_config_get(void)
{
    return nrf_clock_hfclkaudio_config_get(NRF_CLOCK);
}

#endif

#endif // NRFX_DECLARE_ONLY

/** @} */


void nrfx_clock_irq_handler(void);


#ifdef __cplusplus
}
#endif

#endif // NRFX_CLOCK_H__
