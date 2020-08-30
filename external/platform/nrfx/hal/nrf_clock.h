/*
 * Copyright (c) 2015 - 2020, Nordic Semiconductor ASA
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

#ifndef NRF_CLOCK_H__
#define NRF_CLOCK_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_clock_hal Clock HAL
 * @{
 * @ingroup nrf_clock
 * @brief   Hardware access layer for managing the CLOCK peripheral.
 *
 * This code can be used to manage low-frequency clock (LFCLK), high-frequency clock (HFCLK),
 * high-frequency 192 MHz clock (HFCLK192M) and high-frequency audio clock (HFCLKAUDIO)
 * settings.
 */

#if defined(CLOCK_LFCLKSRC_BYPASS_Msk) && defined(CLOCK_LFCLKSRC_EXTERNAL_Msk)
// Enable support for external LFCLK sources. Read more in the Product Specification.
#define NRF_CLOCK_USE_EXTERNAL_LFCLK_SOURCES
#endif

#if defined(CLOCK_INTENSET_DONE_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the Low Frequency Clock calibration. */
#define NRF_CLOCK_HAS_CALIBRATION 1
#else
#define NRF_CLOCK_HAS_CALIBRATION 0
#endif

#if defined(CLOCK_CTIV_CTIV_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the Low Frequency Clock calibration timer. */
#define NRF_CLOCK_HAS_CALIBRATION_TIMER 1
#else
#define NRF_CLOCK_HAS_CALIBRATION_TIMER 0
#endif

#if (defined(CLOCK_INTENSET_HFCLK192MSTARTED_Msk) && !defined(NRF5340_XXAA_NETWORK)) \
    || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the 192 MHz clock. */
#define NRF_CLOCK_HAS_HFCLK192M 1
#else
#define NRF_CLOCK_HAS_HFCLK192M 0
#endif

#if (defined(CLOCK_INTENSET_HFCLKAUDIOSTARTED_Msk) && !defined(NRF5340_XXAA_NETWORK)) \
    || defined(__NRFX_DOXYGEN__)
/** @brief Presence of the Audio clock. */
#define NRF_CLOCK_HAS_HFCLKAUDIO 1
#else
#define NRF_CLOCK_HAS_HFCLKAUDIO 0
#endif

#if (defined(CLOCK_HFCLKCTRL_HCLK_Msk) && !defined(NRF5340_XXAA_NETWORK)) \
    || defined(__NRFX_DOXYGEN__)
/** @brief Presence of HFCLK frequency configuration. */
#define NRF_CLOCK_HAS_HFCLK_DIV 1
#else
#define NRF_CLOCK_HAS_HFCLK_DIV 0
#endif

#if defined(CLOCK_LFCLKALWAYSRUN_ALWAYSRUN_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of ALWAYSRUN registers. */
#define NRF_CLOCK_HAS_ALWAYSRUN 1
#else
#define NRF_CLOCK_HAS_ALWAYSRUN 0
#endif

#if defined(CLOCK_HFCLKSRC_SRC_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Presence of HFCLKSRC register. */
#define NRF_CLOCK_HAS_HFCLKSRC 1
#else
#define NRF_CLOCK_HAS_HFCLKSRC 0
#endif

/**
 * @brief Low-frequency clock sources.
 * @details Used by LFCLKSRC, LFCLKSTAT, and LFCLKSRCCOPY registers.
 */
typedef enum
{
#if defined(CLOCK_LFCLKSRC_SRC_LFULP) || defined(__NRFX_DOXYGEN__)
    NRF_CLOCK_LFCLK_LFULP = CLOCK_LFCLKSRC_SRC_LFULP,  /**< Internal 32 kHz Ultra-low power oscillator. */
#endif

#if defined(CLOCK_LFCLKSRC_SRC_RC) || defined(__NRFX_DOXYGEN__)
    NRF_CLOCK_LFCLK_RC    = CLOCK_LFCLKSRC_SRC_RC,     /**< Internal 32 kHz RC oscillator. */
#else
    NRF_CLOCK_LFCLK_RC    = CLOCK_LFCLKSRC_SRC_LFRC,   /**< Internal 32 kHz RC oscillator. */
#endif

#if defined(CLOCK_LFCLKSRC_SRC_Xtal) || defined(__NRFX_DOXYGEN__)
    NRF_CLOCK_LFCLK_Xtal  = CLOCK_LFCLKSRC_SRC_Xtal,   /**< External 32 kHz crystal. */
#else
    NRF_CLOCK_LFCLK_Xtal  = CLOCK_LFCLKSRC_SRC_LFXO,   /**< External 32 kHz crystal. */
#endif

#if defined(CLOCK_LFCLKSRC_SRC_Synth) || defined(__NRFX_DOXYGEN__)
    NRF_CLOCK_LFCLK_Synth = CLOCK_LFCLKSRC_SRC_Synth,  /**< Internal 32 kHz synthesized from HFCLK system clock. */
#elif defined(CLOCK_LFCLKSRC_SRC_LFSYNT)
    NRF_CLOCK_LFCLK_Synth = CLOCK_LFCLKSRC_SRC_LFSYNT, /**< Internal 32 kHz synthesized from HFCLK system clock. */
#endif

#if defined(NRF_CLOCK_USE_EXTERNAL_LFCLK_SOURCES) || defined(__NRFX_DOXYGEN__)
    /**
     * External 32 kHz low swing signal. Used only with the LFCLKSRC register.
     * For the others @ref NRF_CLOCK_LFCLK_Xtal is returned for this setting.
     */
    NRF_CLOCK_LFCLK_Xtal_Low_Swing = (CLOCK_LFCLKSRC_SRC_Xtal |
        (CLOCK_LFCLKSRC_EXTERNAL_Enabled << CLOCK_LFCLKSRC_EXTERNAL_Pos)),
    /**
     * External 32 kHz full swing signal. Used only with the LFCLKSRC register.
     * For the others @ref NRF_CLOCK_LFCLK_Xtal is returned for this setting.
     */
    NRF_CLOCK_LFCLK_Xtal_Full_Swing = (CLOCK_LFCLKSRC_SRC_Xtal |
        (CLOCK_LFCLKSRC_BYPASS_Enabled   << CLOCK_LFCLKSRC_BYPASS_Pos) |
        (CLOCK_LFCLKSRC_EXTERNAL_Enabled << CLOCK_LFCLKSRC_EXTERNAL_Pos)),
#endif // defined(NRF_CLOCK_USE_EXTERNAL_LFCLK_SOURCES) || defined(__NRFX_DOXYGEN__)
} nrf_clock_lfclk_t;

/**
 * @brief High-frequency clock sources.
 * @details Used by HFCLKSTAT and HFCLK192MSTAT registers.
 */
typedef enum
{
#if defined(CLOCK_HFCLKSTAT_SRC_RC) || defined(__NRFX_DOXYGEN__)
    NRF_CLOCK_HFCLK_LOW_ACCURACY  = CLOCK_HFCLKSTAT_SRC_RC,  /**< Internal 16 MHz RC oscillator. */
#elif defined(CLOCK_HFCLKSTAT_SRC_HFINT)
    NRF_CLOCK_HFCLK_LOW_ACCURACY  = CLOCK_HFCLKSTAT_SRC_HFINT,  /**< Internal 16 MHz RC oscillator. */
#endif
#if defined(CLOCK_HFCLKSTAT_SRC_Xtal) || defined(__NRFX_DOXYGEN__)
    NRF_CLOCK_HFCLK_HIGH_ACCURACY = CLOCK_HFCLKSTAT_SRC_Xtal /**< External 16 MHz/32 MHz crystal oscillator. */
#else
    NRF_CLOCK_HFCLK_HIGH_ACCURACY = CLOCK_HFCLKSTAT_SRC_HFXO /**< External 32 MHz crystal oscillator. */
#endif
} nrf_clock_hfclk_t;

/** @brief Clock domains. */
typedef enum
{
    NRF_CLOCK_DOMAIN_LFCLK,
    NRF_CLOCK_DOMAIN_HFCLK,
#if NRF_CLOCK_HAS_HFCLK192M
    NRF_CLOCK_DOMAIN_HFCLK192M,
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
    NRF_CLOCK_DOMAIN_HFCLKAUDIO,
#endif
} nrf_clock_domain_t;

#if NRF_CLOCK_HAS_HFCLK_DIV || NRF_CLOCK_HAS_HFCLK192M
/**
 * @brief High-frequency clock frequency configuration.
 * @details Used by HFCLKCTRL and HFCLK192MCTRL registers.
 */
typedef enum
{
    NRF_CLOCK_HFCLK_DIV_1 = CLOCK_HFCLKCTRL_HCLK_Div1,         /**< Divide HFCLK/HFCLK192M by 1 */
    NRF_CLOCK_HFCLK_DIV_2 = CLOCK_HFCLKCTRL_HCLK_Div2,         /**< Divide HFCLK/HFCLK192M by 2 */
#if NRF_CLOCK_HAS_HFCLK192M
    NRF_CLOCK_HFCLK_DIV_4 = CLOCK_HFCLK192MCTRL_HCLK192M_Div4, /**< Divide HFCLK192M by 4 */
#endif
} nrf_clock_hfclk_div_t;
#endif // NRF_CLOCK_HAS_HFCLK_DIV || NRF_CLOCK_HAS_HFCLK192M

/**
 * @brief Trigger status of task LFCLKSTART/HFCLKSTART.
 *
 * @note This enum is deprecated.
 *
 * @details Used by LFCLKRUN and HFCLKRUN registers.
 */
typedef enum
{
    NRF_CLOCK_START_TASK_NOT_TRIGGERED = CLOCK_LFCLKRUN_STATUS_NotTriggered, /**< Task LFCLKSTART/HFCLKSTART has not been triggered. */
    NRF_CLOCK_START_TASK_TRIGGERED     = CLOCK_LFCLKRUN_STATUS_Triggered     /**< Task LFCLKSTART/HFCLKSTART has been triggered. */
} nrf_clock_start_task_status_t;

/** @brief Interrupts. */
typedef enum
{
    NRF_CLOCK_INT_HF_STARTED_MASK      = CLOCK_INTENSET_HFCLKSTARTED_Msk,      /**< Interrupt on HFCLKSTARTED event. */
    NRF_CLOCK_INT_LF_STARTED_MASK      = CLOCK_INTENSET_LFCLKSTARTED_Msk,      /**< Interrupt on LFCLKSTARTED event. */
#if NRF_CLOCK_HAS_CALIBRATION
    NRF_CLOCK_INT_DONE_MASK            = CLOCK_INTENSET_DONE_Msk,              /**< Interrupt on DONE event. */
#endif
#if NRF_CLOCK_HAS_CALIBRATION_TIMER
    NRF_CLOCK_INT_CTTO_MASK            = CLOCK_INTENSET_CTTO_Msk,              /**< Interrupt on CTTO event. */
#endif
#if defined(CLOCK_INTENSET_CTSTARTED_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_CLOCK_INT_CTSTARTED_MASK       = CLOCK_INTENSET_CTSTARTED_Msk,         /**< Interrupt on CTSTARTED event. */
    NRF_CLOCK_INT_CTSTOPPED_MASK       = CLOCK_INTENSET_CTSTOPPED_Msk          /**< Interrupt on CTSTOPPED event. */
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
    NRF_CLOCK_INT_HFAUDIO_STARTED_MASK = CLOCK_INTENSET_HFCLKAUDIOSTARTED_Msk, /**< Interrupt on HFCLKAUDIOSTARTED event. */
#endif

#if NRF_CLOCK_HAS_HFCLK192M
    NRF_CLOCK_INT_HF192M_STARTED_MASK  = CLOCK_INTENSET_HFCLK192MSTARTED_Msk,  /**< Interrupt on HFCLK192MSTARTED event. */
#endif
} nrf_clock_int_mask_t;

/**
 * @brief Tasks.
 *
 * @details The NRF_CLOCK_TASK_LFCLKSTOP task cannot be set when the low-frequency clock is not running.
 * The NRF_CLOCK_TASK_HFCLKSTOP task cannot be set when the high-frequency clock is not running.
 */
typedef enum
{
    NRF_CLOCK_TASK_HFCLKSTART      = offsetof(NRF_CLOCK_Type, TASKS_HFCLKSTART),      /**< Start HFCLK clock source. */
    NRF_CLOCK_TASK_HFCLKSTOP       = offsetof(NRF_CLOCK_Type, TASKS_HFCLKSTOP),       /**< Stop HFCLK clock source. */
    NRF_CLOCK_TASK_LFCLKSTART      = offsetof(NRF_CLOCK_Type, TASKS_LFCLKSTART),      /**< Start LFCLK clock source. */
    NRF_CLOCK_TASK_LFCLKSTOP       = offsetof(NRF_CLOCK_Type, TASKS_LFCLKSTOP),       /**< Stop LFCLK clock source. */
#if NRF_CLOCK_HAS_CALIBRATION
    NRF_CLOCK_TASK_CAL             = offsetof(NRF_CLOCK_Type, TASKS_CAL),             /**< Start calibration of LFCLK RC oscillator. */
#endif
#if NRF_CLOCK_HAS_CALIBRATION_TIMER
    NRF_CLOCK_TASK_CTSTART         = offsetof(NRF_CLOCK_Type, TASKS_CTSTART),         /**< Start calibration timer. */
    NRF_CLOCK_TASK_CTSTOP          = offsetof(NRF_CLOCK_Type, TASKS_CTSTOP)           /**< Stop calibration timer. */
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
    NRF_CLOCK_TASK_HFCLKAUDIOSTART = offsetof(NRF_CLOCK_Type, TASKS_HFCLKAUDIOSTART), /**< Start HFCLKAUDIO clock source. */
    NRF_CLOCK_TASK_HFCLKAUDIOSTOP  = offsetof(NRF_CLOCK_Type, TASKS_HFCLKAUDIOSTOP),  /**< Stop HFCLKAUDIO clock source. */
#endif
#if NRF_CLOCK_HAS_HFCLK192M
    NRF_CLOCK_TASK_HFCLK192MSTART  = offsetof(NRF_CLOCK_Type, TASKS_HFCLK192MSTART),  /**< Start HFCLK192M clock source. */
    NRF_CLOCK_TASK_HFCLK192MSTOP   = offsetof(NRF_CLOCK_Type, TASKS_HFCLK192MSTOP),   /**< Stop HFCLK192M clock source. */
#endif
} nrf_clock_task_t;

/** @brief Events. */
typedef enum
{
    NRF_CLOCK_EVENT_HFCLKSTARTED      = offsetof(NRF_CLOCK_Type, EVENTS_HFCLKSTARTED),      /**< HFCLK oscillator started. */
    NRF_CLOCK_EVENT_LFCLKSTARTED      = offsetof(NRF_CLOCK_Type, EVENTS_LFCLKSTARTED),      /**< LFCLK oscillator started. */
#if NRF_CLOCK_HAS_CALIBRATION
    NRF_CLOCK_EVENT_DONE              = offsetof(NRF_CLOCK_Type, EVENTS_DONE),              /**< Calibration of LFCLK RC oscillator completed. */
#endif
#if NRF_CLOCK_HAS_CALIBRATION_TIMER
    NRF_CLOCK_EVENT_CTTO              = offsetof(NRF_CLOCK_Type, EVENTS_CTTO),              /**< Calibration timer time-out. */
#endif
#if defined(CLOCK_INTENSET_CTSTARTED_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_CLOCK_EVENT_CTSTARTED         = offsetof(NRF_CLOCK_Type, EVENTS_CTSTARTED),         /**< Calibration timer started. */
    NRF_CLOCK_EVENT_CTSTOPPED         = offsetof(NRF_CLOCK_Type, EVENTS_CTSTOPPED)          /**< Calibration timer stopped. */
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
    NRF_CLOCK_EVENT_HFCLKAUDIOSTARTED = offsetof(NRF_CLOCK_Type, EVENTS_HFCLKAUDIOSTARTED), /**< HFCLKAUDIO oscillator started. */
#endif
#if NRF_CLOCK_HAS_HFCLK192M
    NRF_CLOCK_EVENT_HFCLK192MSTARTED  = offsetof(NRF_CLOCK_Type, EVENTS_HFCLK192MSTARTED),  /**< HFCLK192M oscillator started. */
#endif
} nrf_clock_event_t;

/**
 * @brief Function for enabling the specified interrupt.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
NRF_STATIC_INLINE void nrf_clock_int_enable(NRF_CLOCK_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling the specified interrupt.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
NRF_STATIC_INLINE void nrf_clock_int_disable(NRF_CLOCK_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_clock_int_enable_check(NRF_CLOCK_Type const * p_reg, uint32_t mask);

/**
 * @brief Function for retrieving the address of the specified task.
 * @details This function can be used by the PPI module.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  CLOCK Task.
 *
 * @return Address of the requested task register.
 */
NRF_STATIC_INLINE uint32_t nrf_clock_task_address_get(NRF_CLOCK_Type const * p_reg,
                                                      nrf_clock_task_t       task);

/**
 * @brief Function for setting the specified task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task to be activated.
 */
NRF_STATIC_INLINE void nrf_clock_task_trigger(NRF_CLOCK_Type * p_reg, nrf_clock_task_t task);

/**
 * @brief Function for retrieving the address of the specified event.
 * @details This function can be used by the PPI module.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event CLOCK Event.
 *
 * @return Address of the specified event register.
 */
NRF_STATIC_INLINE uint32_t nrf_clock_event_address_get(NRF_CLOCK_Type const * p_reg,
                                                       nrf_clock_event_t      event);

/**
 * @brief Function for clearing the specified event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to clear.
 */
NRF_STATIC_INLINE void nrf_clock_event_clear(NRF_CLOCK_Type * p_reg, nrf_clock_event_t event);

/**
 * @brief Function for retrieving the state of the specified event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_clock_event_check(NRF_CLOCK_Type const * p_reg, nrf_clock_event_t event);

/**
 * @brief Function for retrieving the trigger status of the task START for given domain.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] domain Clock domain.
 *
 * @retval false The task START for the given domain has not been triggered.
 * @retval true  The task START for the given domain has been triggered.
 */
NRF_STATIC_INLINE bool nrf_clock_start_task_check(NRF_CLOCK_Type const * p_reg,
                                                  nrf_clock_domain_t     domain);

/**
 * @brief Function for retrieving the state of the clock.
 *
 * @param[in]  p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in]  domain    Clock domain.
 * @param[out] p_clk_src Pointer to clock source that is running. Set to NULL if not needed.
 *                       Ignored for HFCLKAUDIO domain. Variable pointed by @p p_clk_src
 *                       must be of either @ref nrf_clock_lfclk_t type for LFCLK
 *                       or @ref nrf_clock_hfclk_t type for HFCLK and HFCLK192M.
 *
 * @retval false The clock is not running.
 * @retval true  The clock is running.
 */
NRF_STATIC_INLINE bool nrf_clock_is_running(NRF_CLOCK_Type const * p_reg,
                                            nrf_clock_domain_t     domain,
                                            void *                 p_clk_src);

/**
 * @brief Function for changing the low-frequency clock source.
 * @details Check in Product Specification if this function can be called when
 *          the low-frequency clock is running.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] source New low-frequency clock source.
 */
NRF_STATIC_INLINE void nrf_clock_lf_src_set(NRF_CLOCK_Type * p_reg, nrf_clock_lfclk_t source);

/**
 * @brief Function for retrieving the selected source for the low-frequency clock.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval NRF_CLOCK_LFCLK_RC    The internal 32 kHz RC oscillator
 *                               is the selected source for the low-frequency clock.
 * @retval NRF_CLOCK_LFCLK_Xtal  An external 32 kHz crystal oscillator
 *                               is the selected source for the low-frequency clock.
 * @retval NRF_CLOCK_LFCLK_Synth The internal 32 kHz synthesized from
 *                               the HFCLK is the selected source for the low-frequency clock.
 */
NRF_STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_src_get(NRF_CLOCK_Type const * p_reg);

/**
 * @brief Function for retrieving the active source of the low-frequency clock.
 *
 * @note This function is deprecated. Use @ref nrf_clock_is_running instead.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval NRF_CLOCK_LFCLK_RC    The internal 32 kHz RC oscillator
 *                               is the active source of the low-frequency clock.
 * @retval NRF_CLOCK_LFCLK_Xtal  An external 32 kHz crystal oscillator
 *                               is the active source of the low-frequency clock.
 * @retval NRF_CLOCK_LFCLK_Synth The internal 32 kHz synthesized from
 *                               the HFCLK is the active source of the low-frequency clock.
 */
NRF_STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_actv_src_get(NRF_CLOCK_Type const * p_reg);

/**
 * @brief Function for retrieving the clock source for the LFCLK clock when
 *        the task LKCLKSTART is triggered.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval NRF_CLOCK_LFCLK_RC    The internal 32 kHz RC oscillator
 *                               is running and generating the LFCLK clock.
 * @retval NRF_CLOCK_LFCLK_Xtal  An external 32 kHz crystal oscillator
 *                               is running and generating the LFCLK clock.
 * @retval NRF_CLOCK_LFCLK_Synth The internal 32 kHz synthesized from
 *                               the HFCLK is running and generating the LFCLK clock.
 */
NRF_STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_srccopy_get(NRF_CLOCK_Type const * p_reg);

/**
 * @brief Function for retrieving the state of the LFCLK clock.
 *
 * @note This function is deprecated. Use @ref nrf_clock_is_running instead.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval false The LFCLK clock is not running.
 * @retval true  The LFCLK clock is running.
 */
NRF_STATIC_INLINE bool nrf_clock_lf_is_running(NRF_CLOCK_Type const * p_reg);

/**
 * @brief Function for retrieving the trigger status of the task LFCLKSTART.
 *
 * @note This function is deprecated. Use @ref nrf_clock_start_task_check instead.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval NRF_CLOCK_START_TASK_NOT_TRIGGERED The task LFCLKSTART has not been triggered.
 * @retval NRF_CLOCK_START_TASK_TRIGGERED     The task LFCLKSTART has been triggered.
 */
NRF_STATIC_INLINE
nrf_clock_start_task_status_t nrf_clock_lf_start_task_status_get(NRF_CLOCK_Type const * p_reg);

#if NRF_CLOCK_HAS_HFCLKSRC
/**
 * @brief Function for changing the high-frequency clock source.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] source New high-frequency clock source.
 */
NRF_STATIC_INLINE void nrf_clock_hf_src_set(NRF_CLOCK_Type * p_reg, nrf_clock_hfclk_t source);
#endif

/**
 * @brief Function for retrieving the selected source of the high-frequency clock.
 *
 * For SoCs not featuring the HFCLKSRC register, this is always also the active source
 * of the high-frequency clock.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval NRF_CLOCK_HFCLK_LOW_ACCURACY  The internal RC oscillator is the selected
 *                                       source of the high-frequency clock.
 * @retval NRF_CLOCK_HFCLK_HIGH_ACCURACY An external crystal oscillator is the selected
 *                                       source of the high-frequency clock.
 */
NRF_STATIC_INLINE nrf_clock_hfclk_t nrf_clock_hf_src_get(NRF_CLOCK_Type const * p_reg);

/**
 * @brief Function for retrieving the state of the HFCLK clock.
 *
 * @note This function is deprecated. Use @ref nrf_clock_is_running instead.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] clk_src Clock source to be checked.
 *
 * @retval false The HFCLK clock is not running.
 * @retval true  The HFCLK clock is running.
 */
NRF_STATIC_INLINE bool nrf_clock_hf_is_running(NRF_CLOCK_Type const * p_reg,
                                               nrf_clock_hfclk_t      clk_src);

/**
 * @brief Function for retrieving the trigger status of the task HFCLKSTART.
 *
 * @note This function is deprecated. Use @ref nrf_clock_start_task_check instead.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval NRF_CLOCK_START_TASK_NOT_TRIGGERED The task HFCLKSTART has not been triggered.
 * @retval NRF_CLOCK_START_TASK_TRIGGERED     The task HFCLKSTART has been triggered.
 */
NRF_STATIC_INLINE
nrf_clock_start_task_status_t nrf_clock_hf_start_task_status_get(NRF_CLOCK_Type const * p_reg);

#if NRF_CLOCK_HAS_HFCLKAUDIO
/**
 * @brief Function for changing the Audio clock FREQ_VALUE.
 *
 * The frequency of HFCLKAUDIO ranges from 10.666 MHz to 13.333 MHz in 40.7 Hz steps.
 * To calculate @p freq_value corresponding to the chosen frequency, use the following equation:
 * FREQ_VALUE = 2^16 * ((12 * f_out / 32M) - 4)
 *
 * @warning Chosen frequency must fit in 11.176 MHz - 11.402 MHz or 12.165 MHz - 12.411 MHz frequency bands.
 *
 * @param[in] p_reg      Pointer to the structure of registers of the peripheral.
 * @param[in] freq_value New FREQ_VALUE
 */
NRF_STATIC_INLINE
void nrf_clock_hfclkaudio_config_set(NRF_CLOCK_Type * p_reg, uint16_t freq_value);

/**
 * @brief Function for retrieving the Audio clock FREQ_VALUE.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * The frequency of HFCLKAUDIO ranges from 10.666 MHz to 13.333 MHz in 40.7 Hz steps.
 * To calculate frequency corresponding to the returned FREQ_VALUE, use the following equation:
 * f_out = 32M * (4 + FREQ_VALUE * 2^(-16))/12

 * @return FREQ_VALUE of the Audio clock.
 */
NRF_STATIC_INLINE
uint16_t nrf_clock_hfclkaudio_config_get(NRF_CLOCK_Type const * p_reg);
#endif

#if NRF_CLOCK_HAS_CALIBRATION_TIMER
/**
 * @brief Function for changing the calibration timer interval.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] interval New calibration timer interval in 0.25 s resolution
 *                     (range: 0.25 seconds to 31.75 seconds).
 */
NRF_STATIC_INLINE void nrf_clock_cal_timer_timeout_set(NRF_CLOCK_Type * p_reg, uint32_t interval);
#endif

#if NRF_CLOCK_HAS_HFCLK_DIV
/**
 * @brief Function for changing the HFCLK frequency divider.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] divider New HFCLK divider.
 */
NRF_STATIC_INLINE void nrf_clock_hfclk_div_set(NRF_CLOCK_Type *      p_reg,
                                               nrf_clock_hfclk_div_t divider);

/**
 * @brief Function for retrieving the HFCLK frequency divider.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return HFCLK frequency divider.
 */
NRF_STATIC_INLINE nrf_clock_hfclk_div_t nrf_clock_hfclk_div_get(NRF_CLOCK_Type const * p_reg);
#endif

#if NRF_CLOCK_HAS_HFCLK192M
/**
 * @brief Function for changing the HFCLK192M frequency divider.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] divider New HFCLK192M divider.
 */
NRF_STATIC_INLINE void nrf_clock_hfclk192m_div_set(NRF_CLOCK_Type *      p_reg,
                                                   nrf_clock_hfclk_div_t divider);

/**
 * @brief Function for retrieving the HFCLK192M frequency divider.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return HFCLK192M frequency divider.
 */
NRF_STATIC_INLINE nrf_clock_hfclk_div_t nrf_clock_hfclk192m_div_get(NRF_CLOCK_Type const * p_reg);

/**
 * @brief Function for changing the HFCLK192M source.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] source New HFCLK192M source.
 */
NRF_STATIC_INLINE void nrf_clock_hfclk192m_src_set(NRF_CLOCK_Type *  p_reg,
                                                   nrf_clock_hfclk_t source);

/**
 * @brief Function for retrieving the selected source of the HFCLK192M.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval NRF_CLOCK_HFCLK_LOW_ACCURACY  The internal RC oscillator is the selected
 *                                       source of the HFCLK192M.
 * @retval NRF_CLOCK_HFCLK_HIGH_ACCURACY An external crystal oscillator is the selected
 *                                       source of the HFCLK192M.
 */
NRF_STATIC_INLINE nrf_clock_hfclk_t nrf_clock_hfclk192m_src_get(NRF_CLOCK_Type const * p_reg);
#endif // NRF_CLOCK_HAS_HFCLK192M

#if NRF_CLOCK_HAS_ALWAYSRUN
/**
 * @brief Function for setting the clock domain to always run.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] domain    Clock domain.
 * @param[in] alwaysrun Ensure the clock is always running.
 */
NRF_STATIC_INLINE void nrf_clock_alwaysrun_set(NRF_CLOCK_Type const * p_reg,
                                               nrf_clock_domain_t     domain,
                                               bool                   alwaysrun);
/**
 * @brief Function for checking if the clock domain is configured to always run.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] domain Clock domain.
 *
 * @retval true  The clock domain is configured to always run.
 * @retval false The clock domain is not configured to always run.
 */
NRF_STATIC_INLINE bool nrf_clock_alwaysrun_get(NRF_CLOCK_Type const * p_reg,
                                               nrf_clock_domain_t     domain);
/**
 * @brief Function for checking if the clock domain always run setting is active.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] domain Clock domain.
 *
 * @retval true  The clock domain always run setting is active.
 * @retval false The clock domain always run setting is not active.
 */
NRF_STATIC_INLINE bool nrf_clock_alwaysrun_active_get(NRF_CLOCK_Type const * p_reg,
                                                      nrf_clock_domain_t     domain);
#endif

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        CLOCK task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
NRF_STATIC_INLINE void nrf_clock_subscribe_set(NRF_CLOCK_Type * p_reg,
                                               nrf_clock_task_t task,
                                               uint8_t          channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        CLOCK task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_clock_subscribe_clear(NRF_CLOCK_Type * p_reg, nrf_clock_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        CLOCK event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
NRF_STATIC_INLINE void nrf_clock_publish_set(NRF_CLOCK_Type *  p_reg,
                                             nrf_clock_event_t event,
                                             uint8_t           channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        CLOCK event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_clock_publish_clear(NRF_CLOCK_Type * p_reg, nrf_clock_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_clock_int_enable(NRF_CLOCK_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE void nrf_clock_int_disable(NRF_CLOCK_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

NRF_STATIC_INLINE uint32_t nrf_clock_int_enable_check(NRF_CLOCK_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

NRF_STATIC_INLINE uint32_t nrf_clock_task_address_get(NRF_CLOCK_Type const * p_reg,
                                                      nrf_clock_task_t       task)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)task);
}

NRF_STATIC_INLINE void nrf_clock_task_trigger(NRF_CLOCK_Type * p_reg, nrf_clock_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

NRF_STATIC_INLINE uint32_t nrf_clock_event_address_get(NRF_CLOCK_Type const * p_reg,
                                                       nrf_clock_event_t      event)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_clock_event_clear(NRF_CLOCK_Type * p_reg, nrf_clock_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
    nrf_event_readback((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE bool nrf_clock_event_check(NRF_CLOCK_Type const * p_reg, nrf_clock_event_t event)
{
    return (bool)*((volatile uint32_t *)((uint8_t *)p_reg + event));
}

NRF_STATIC_INLINE bool nrf_clock_start_task_check(NRF_CLOCK_Type const * p_reg,
                                                  nrf_clock_domain_t     domain)
{
    switch (domain)
    {
        case NRF_CLOCK_DOMAIN_LFCLK:
            return ((p_reg->LFCLKRUN & CLOCK_LFCLKRUN_STATUS_Msk)
                    >> CLOCK_LFCLKRUN_STATUS_Pos);
        case NRF_CLOCK_DOMAIN_HFCLK:
            return ((p_reg->HFCLKRUN & CLOCK_HFCLKRUN_STATUS_Msk)
                    >> CLOCK_HFCLKRUN_STATUS_Pos);
#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            return ((p_reg->HFCLK192MRUN & CLOCK_HFCLK192MRUN_STATUS_Msk)
                    >> CLOCK_HFCLK192MRUN_STATUS_Pos);
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
        case NRF_CLOCK_DOMAIN_HFCLKAUDIO:
            return ((p_reg->HFCLKAUDIORUN & CLOCK_HFCLKAUDIORUN_STATUS_Msk)
                    >> CLOCK_HFCLKAUDIORUN_STATUS_Pos);
#endif
        default:
            NRFX_ASSERT(0);
            return false;
    }
}

NRF_STATIC_INLINE bool nrf_clock_is_running(NRF_CLOCK_Type const * p_reg,
                                            nrf_clock_domain_t     domain,
                                            void *                 p_clk_src)
{
    switch (domain)
    {
        case NRF_CLOCK_DOMAIN_LFCLK:
            if (p_clk_src != NULL)
            {
                (*(nrf_clock_lfclk_t *)p_clk_src) =
                    (nrf_clock_lfclk_t)((p_reg->LFCLKSTAT & CLOCK_LFCLKSTAT_SRC_Msk)
                                        >> CLOCK_LFCLKSTAT_SRC_Pos);
            }
            if ((p_reg->LFCLKSTAT & CLOCK_LFCLKSTAT_STATE_Msk)
                >> CLOCK_LFCLKSTAT_STATE_Pos)
            {
                return true;
            }
            break;
        case NRF_CLOCK_DOMAIN_HFCLK:
            if (p_clk_src != NULL)
            {
                (*(nrf_clock_hfclk_t *)p_clk_src) =
                    (nrf_clock_hfclk_t)((p_reg->HFCLKSTAT & CLOCK_HFCLKSTAT_SRC_Msk)
                                        >> CLOCK_HFCLKSTAT_SRC_Pos);
            }
            if ((p_reg->HFCLKSTAT & CLOCK_HFCLKSTAT_STATE_Msk)
                >> CLOCK_HFCLKSTAT_STATE_Pos)
            {
                return true;
            }
            break;
#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            if (p_clk_src != NULL)
            {
                (*(nrf_clock_hfclk_t *)p_clk_src) =
                    (nrf_clock_hfclk_t)((p_reg->HFCLK192MSTAT & CLOCK_HFCLK192MSTAT_SRC_Msk)
                                        >> CLOCK_HFCLK192MSTAT_SRC_Pos);
            }
            if ((p_reg->HFCLK192MSTAT & CLOCK_HFCLK192MSTAT_STATE_Msk)
                >> CLOCK_HFCLK192MSTAT_STATE_Pos)
            {
                return true;
            }
            break;
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
        case NRF_CLOCK_DOMAIN_HFCLKAUDIO:
            return (p_reg->HFCLKAUDIOSTAT & CLOCK_HFCLKAUDIOSTAT_STATE_Msk) ==
                   CLOCK_HFCLKAUDIOSTAT_STATE_Msk;
#endif
        default:
            NRFX_ASSERT(0);
            return false;
    }
    return false;
}

NRF_STATIC_INLINE void nrf_clock_lf_src_set(NRF_CLOCK_Type * p_reg, nrf_clock_lfclk_t source)
{
    p_reg->LFCLKSRC = (uint32_t)(source);
}

NRF_STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_src_get(NRF_CLOCK_Type const * p_reg)
{
    return (nrf_clock_lfclk_t)(p_reg->LFCLKSRC);
}

NRF_STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_actv_src_get(NRF_CLOCK_Type const * p_reg)
{
    nrf_clock_lfclk_t clk_src;
    (void)nrf_clock_is_running(p_reg, NRF_CLOCK_DOMAIN_LFCLK, &clk_src);
    return clk_src;
}

NRF_STATIC_INLINE nrf_clock_lfclk_t nrf_clock_lf_srccopy_get(NRF_CLOCK_Type const * p_reg)
{
    return (nrf_clock_lfclk_t)((p_reg->LFCLKSRCCOPY & CLOCK_LFCLKSRCCOPY_SRC_Msk)
                                >> CLOCK_LFCLKSRCCOPY_SRC_Pos);
}

NRF_STATIC_INLINE bool nrf_clock_lf_is_running(NRF_CLOCK_Type const * p_reg)
{
    return nrf_clock_is_running(p_reg, NRF_CLOCK_DOMAIN_LFCLK, NULL);
}

NRF_STATIC_INLINE
nrf_clock_start_task_status_t nrf_clock_lf_start_task_status_get(NRF_CLOCK_Type const * p_reg)
{
    return (nrf_clock_start_task_status_t)nrf_clock_start_task_check(p_reg,
                                                                     NRF_CLOCK_DOMAIN_LFCLK);
}

#if NRF_CLOCK_HAS_HFCLKSRC
NRF_STATIC_INLINE void nrf_clock_hf_src_set(NRF_CLOCK_Type * p_reg, nrf_clock_hfclk_t source)
{
    p_reg->HFCLKSRC = (uint32_t)(source);
}
#endif

NRF_STATIC_INLINE nrf_clock_hfclk_t nrf_clock_hf_src_get(NRF_CLOCK_Type const * p_reg)
{
#if NRF_CLOCK_HAS_HFCLKSRC
    return (nrf_clock_hfclk_t)(p_reg->HFCLKSRC);
#else
    return (nrf_clock_hfclk_t)((p_reg->HFCLKSTAT & CLOCK_HFCLKSTAT_SRC_Msk)
                                >> CLOCK_HFCLKSTAT_SRC_Pos);
#endif
}

NRF_STATIC_INLINE bool nrf_clock_hf_is_running(NRF_CLOCK_Type const * p_reg,
                                               nrf_clock_hfclk_t      clk_src)
{
    nrf_clock_hfclk_t active_clk_src;
    bool ret = nrf_clock_is_running(p_reg, NRF_CLOCK_DOMAIN_HFCLK, &active_clk_src);
    return (ret && (active_clk_src == clk_src));
}

NRF_STATIC_INLINE
nrf_clock_start_task_status_t nrf_clock_hf_start_task_status_get(NRF_CLOCK_Type const * p_reg)
{
    return (nrf_clock_start_task_status_t)nrf_clock_start_task_check(p_reg,
                                                                     NRF_CLOCK_DOMAIN_HFCLK);
}

#if NRF_CLOCK_HAS_HFCLKAUDIO
NRF_STATIC_INLINE
void nrf_clock_hfclkaudio_config_set(NRF_CLOCK_Type * p_reg, uint16_t freq_value)
{
    p_reg->HFCLKAUDIO.FREQUENCY = freq_value;
}

NRF_STATIC_INLINE
uint16_t nrf_clock_hfclkaudio_config_get(NRF_CLOCK_Type const * p_reg)
{
    return (uint16_t)(p_reg->HFCLKAUDIO.FREQUENCY);
}
#endif

#if NRF_CLOCK_HAS_HFCLK_DIV
NRF_STATIC_INLINE
void nrf_clock_hfclk_div_set(NRF_CLOCK_Type * p_reg, nrf_clock_hfclk_div_t divider)
{
    p_reg->HFCLKCTRL = (((uint8_t)(divider) << CLOCK_HFCLKCTRL_HCLK_Pos) &
                        CLOCK_HFCLKCTRL_HCLK_Msk);
}

NRF_STATIC_INLINE nrf_clock_hfclk_div_t nrf_clock_hfclk_div_get(NRF_CLOCK_Type const * p_reg)
{
    return (nrf_clock_hfclk_div_t)((p_reg->HFCLKCTRL & CLOCK_HFCLKCTRL_HCLK_Msk)
                                   >> CLOCK_HFCLKCTRL_HCLK_Pos);
}
#endif

#if NRF_CLOCK_HAS_HFCLK192M
NRF_STATIC_INLINE
void nrf_clock_hfclk192m_div_set(NRF_CLOCK_Type * p_reg, nrf_clock_hfclk_div_t divider)
{
    p_reg->HFCLK192MCTRL = (((uint8_t)(divider) << CLOCK_HFCLK192MCTRL_HCLK192M_Pos) &
                            CLOCK_HFCLK192MCTRL_HCLK192M_Msk);
}

NRF_STATIC_INLINE nrf_clock_hfclk_div_t nrf_clock_hfclk192m_div_get(NRF_CLOCK_Type const * p_reg)
{
    return (nrf_clock_hfclk_div_t)((p_reg->HFCLK192MCTRL & CLOCK_HFCLK192MCTRL_HCLK192M_Msk)
                                   >> CLOCK_HFCLK192MCTRL_HCLK192M_Pos);
}

NRF_STATIC_INLINE void nrf_clock_hfclk192m_src_set(NRF_CLOCK_Type * p_reg, nrf_clock_hfclk_t source)
{
    p_reg->HFCLK192MSRC = (uint32_t)(source);
}

NRF_STATIC_INLINE nrf_clock_hfclk_t nrf_clock_hfclk192m_src_get(NRF_CLOCK_Type const * p_reg)
{
    return (nrf_clock_hfclk_t)(p_reg->HFCLK192MSRC);
}
#endif

#if NRF_CLOCK_HAS_CALIBRATION_TIMER
NRF_STATIC_INLINE void nrf_clock_cal_timer_timeout_set(NRF_CLOCK_Type * p_reg, uint32_t interval)
{
    p_reg->CTIV = ((interval << CLOCK_CTIV_CTIV_Pos) & CLOCK_CTIV_CTIV_Msk);
}
#endif

#if NRF_CLOCK_HAS_ALWAYSRUN
NRF_STATIC_INLINE void nrf_clock_alwaysrun_set(NRF_CLOCK_Type const * p_reg,
                                               nrf_clock_domain_t     domain,
                                               bool                   alwaysrun)
{
    /* ALWAYSRUN registers should be R/W, but are marked as read-only.
     * Redefine them as R/W as a workaround. */
    switch (domain)
    {
        case NRF_CLOCK_DOMAIN_LFCLK:
            *(volatile uint32_t *)(&p_reg->LFCLKALWAYSRUN) =
                ((alwaysrun << CLOCK_LFCLKALWAYSRUN_ALWAYSRUN_Pos)
                 & CLOCK_LFCLKALWAYSRUN_ALWAYSRUN_Msk);
            break;
        case NRF_CLOCK_DOMAIN_HFCLK:
            *(volatile uint32_t *)(&p_reg->HFCLKALWAYSRUN) =
                ((alwaysrun << CLOCK_HFCLKALWAYSRUN_ALWAYSRUN_Pos)
                 & CLOCK_HFCLKALWAYSRUN_ALWAYSRUN_Msk);
            break;
#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            *(volatile uint32_t *)(&p_reg->HFCLK192MALWAYSRUN) =
                ((alwaysrun << CLOCK_HFCLK192MALWAYSRUN_ALWAYSRUN_Pos)
                 & CLOCK_HFCLK192MALWAYSRUN_ALWAYSRUN_Msk);
            break;
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
        case NRF_CLOCK_DOMAIN_HFCLKAUDIO:
            *(volatile uint32_t *)(&p_reg->HFCLKAUDIOALWAYSRUN) =
                ((alwaysrun << CLOCK_HFCLKAUDIOALWAYSRUN_ALWAYSRUN_Pos)
                 & CLOCK_HFCLKAUDIOALWAYSRUN_ALWAYSRUN_Msk);
            break;
#endif
        default:
            NRFX_ASSERT(0);
            break;
    }
}

NRF_STATIC_INLINE bool nrf_clock_alwaysrun_get(NRF_CLOCK_Type const * p_reg,
                                               nrf_clock_domain_t     domain)
{
    switch (domain)
    {
        case NRF_CLOCK_DOMAIN_LFCLK:
            return ((p_reg->LFCLKALWAYSRUN & CLOCK_LFCLKALWAYSRUN_ALWAYSRUN_Msk)
                    >> CLOCK_LFCLKALWAYSRUN_ALWAYSRUN_Pos);
        case NRF_CLOCK_DOMAIN_HFCLK:
            return ((p_reg->HFCLKALWAYSRUN & CLOCK_HFCLKALWAYSRUN_ALWAYSRUN_Msk)
                    >> CLOCK_HFCLKALWAYSRUN_ALWAYSRUN_Pos);
#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            return ((p_reg->HFCLK192MALWAYSRUN & CLOCK_HFCLK192MALWAYSRUN_ALWAYSRUN_Msk)
                    >> CLOCK_HFCLK192MALWAYSRUN_ALWAYSRUN_Pos);
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
        case NRF_CLOCK_DOMAIN_HFCLKAUDIO:
            return ((p_reg->HFCLKAUDIOALWAYSRUN & CLOCK_HFCLKAUDIOALWAYSRUN_ALWAYSRUN_Msk)
                    >> CLOCK_HFCLKAUDIOALWAYSRUN_ALWAYSRUN_Pos);
#endif
        default:
            NRFX_ASSERT(0);
            return false;
    }
}

NRF_STATIC_INLINE bool nrf_clock_alwaysrun_active_get(NRF_CLOCK_Type const * p_reg,
                                                      nrf_clock_domain_t     domain)
{
    switch (domain)
    {
        case NRF_CLOCK_DOMAIN_LFCLK:
            return ((p_reg->LFCLKSTAT & CLOCK_LFCLKSTAT_ALWAYSRUNNING_Msk)
                    >> CLOCK_LFCLKSTAT_ALWAYSRUNNING_Pos);
        case NRF_CLOCK_DOMAIN_HFCLK:
            return ((p_reg->HFCLKSTAT & CLOCK_HFCLKSTAT_ALWAYSRUNNING_Msk)
                    >> CLOCK_HFCLKSTAT_ALWAYSRUNNING_Pos);
#if NRF_CLOCK_HAS_HFCLK192M
        case NRF_CLOCK_DOMAIN_HFCLK192M:
            return ((p_reg->HFCLK192MSTAT & CLOCK_HFCLK192MSTAT_ALWAYSRUNNING_Msk)
                    >> CLOCK_HFCLK192MSTAT_ALWAYSRUNNING_Pos);
#endif
#if NRF_CLOCK_HAS_HFCLKAUDIO
        case NRF_CLOCK_DOMAIN_HFCLKAUDIO:
            return ((p_reg->HFCLKAUDIOSTAT & CLOCK_HFCLKAUDIOSTAT_ALWAYSRUNNING_Msk)
                    >> CLOCK_HFCLKAUDIOSTAT_ALWAYSRUNNING_Pos);
#endif
        default:
            NRFX_ASSERT(0);
            return false;
    }
}
#endif // NRF_CLOCK_HAS_ALWAYSRUN

#if defined(DPPI_PRESENT)
NRF_STATIC_INLINE void nrf_clock_subscribe_set(NRF_CLOCK_Type * p_reg,
                                               nrf_clock_task_t task,
                                               uint8_t          channel)
{
    *((volatile uint32_t *) ((uint8_t *)p_reg+ (uint32_t)task + 0x80uL)) =
            ((uint32_t)channel | CLOCK_SUBSCRIBE_HFCLKSTART_EN_Msk);
}

NRF_STATIC_INLINE void nrf_clock_subscribe_clear(NRF_CLOCK_Type * p_reg, nrf_clock_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *)p_reg + (uint32_t)task + 0x80uL)) = 0;
}

NRF_STATIC_INLINE void nrf_clock_publish_set(NRF_CLOCK_Type *  p_reg,
                                             nrf_clock_event_t event,
                                             uint8_t           channel)
{
    *((volatile uint32_t *) ((uint8_t *)p_reg + (uint32_t)event + 0x80uL)) =
            ((uint32_t)channel | CLOCK_PUBLISH_HFCLKSTARTED_EN_Msk);
}

NRF_STATIC_INLINE void nrf_clock_publish_clear(NRF_CLOCK_Type * p_reg, nrf_clock_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *)p_reg + (uint32_t)event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_CLOCK_H__
