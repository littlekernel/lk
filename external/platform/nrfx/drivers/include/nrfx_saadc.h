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

#ifndef NRFX_SAADC_H__
#define NRFX_SAADC_H__

#include <nrfx.h>
#include <hal/nrf_saadc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_saadc SAADC driver
 * @{
 * @ingroup nrf_saadc
 * @brief   Successive Approximation Analog-to-Digital Converter (SAADC) peripheral driver.
 */


/**
 * @brief SAADC channel default configuration for the single-ended mode.
 *
 * This configuration sets up single-ended SAADC channel with the following options:
 * - resistor ladder disabled
 * - gain: 1/6
 * - reference voltage: internal 0.6 V
 * - sample acquisition time: 10 us
 * - burst disabled
 *
 * @param[in] _pin_p Positive input analog pin.
 * @param[in] _index Channel index.
 *
 * @sa nrfx_saadc_channel_t
 */
#define NRFX_SAADC_DEFAULT_CHANNEL_SE(_pin_p, _index)       \
{                                                           \
    .channel_config =                                       \
    {                                                       \
        .resistor_p = NRF_SAADC_RESISTOR_DISABLED,          \
        .resistor_n = NRF_SAADC_RESISTOR_DISABLED,          \
        .gain       = NRF_SAADC_GAIN1_6,                    \
        .reference  = NRF_SAADC_REFERENCE_INTERNAL,         \
        .acq_time   = NRF_SAADC_ACQTIME_10US,               \
        .mode       = NRF_SAADC_MODE_SINGLE_ENDED,          \
        .burst      = NRF_SAADC_BURST_DISABLED,             \
    },                                                      \
    .pin_p          = (nrf_saadc_input_t)_pin_p,            \
    .pin_n          = NRF_SAADC_INPUT_DISABLED,             \
    .channel_index  = _index,                               \
}

/**
 * @brief SAADC channel default configuration for the differential mode.
 *
 * This configuration sets up differential SAADC channel with the following options:
 * - resistor ladder disabled
 * - gain: 1/6
 * - reference voltage: internal 0.6 V
 * - sample acquisition time: 10 us
 * - burst disabled
 *
 * @param[in] _pin_p Positive input analog pin.
 * @param[in] _pin_n Negative input analog pin.
 * @param[in] _index Channel index.
 *
 * @sa nrfx_saadc_channel_t
 */
#define NRFX_SAADC_DEFAULT_CHANNEL_DIFFERENTIAL(_pin_p, _pin_n, _index) \
{                                                                       \
    .channel_config =                                                   \
    {                                                                   \
        .resistor_p = NRF_SAADC_RESISTOR_DISABLED,                      \
        .resistor_n = NRF_SAADC_RESISTOR_DISABLED,                      \
        .gain       = NRF_SAADC_GAIN1_6,                                \
        .reference  = NRF_SAADC_REFERENCE_INTERNAL,                     \
        .acq_time   = NRF_SAADC_ACQTIME_10US,                           \
        .mode       = NRF_SAADC_MODE_DIFFERENTIAL,                      \
        .burst      = NRF_SAADC_BURST_DISABLED,                         \
    },                                                                  \
    .pin_p          = (nrf_saadc_input_t)_pin_p,                        \
    .pin_n          = (nrf_saadc_input_t)_pin_n,                        \
    .channel_index  = _index,                                           \
}

/**
 * @brief SAADC driver advanced mode default configuration.
 *
 * This configuration sets up advanced mode of the SAADC driver with the following options:
 * - oversampling disabled
 * - burst disabled
 * - internal sampling timer disabled
 * - triggering of the START task on the END event disabled
 *
 * @param[in] _pin_p Positive input analog pin.
 * @param[in] _pin_n Negative input analog pin.
 * @param[in] _index Channel index.
 *
 * @sa nrfx_saadc_adv_config_t
 */
#define NRFX_SAADC_DEFAULT_ADV_CONFIG                                           \
{                                                                               \
    .oversampling      = NRF_SAADC_OVERSAMPLE_DISABLED,                         \
    .burst             = NRF_SAADC_BURST_DISABLED,                              \
    .internal_timer_cc = 0,                                                     \
    .start_on_end      = false,                                                 \
}

/** @brief SAADC channel configuration structure. */
typedef struct
{
    nrf_saadc_channel_config_t channel_config; ///< Channel hardware configuration.
    nrf_saadc_input_t          pin_p;          ///< Input positive pin selection.
    nrf_saadc_input_t          pin_n;          ///< Input negative pin selection.
    uint8_t                    channel_index;  ///< Channel index.
} nrfx_saadc_channel_t;

/** @brief SAADC driver advanced mode configuration structure. */
typedef struct
{
    nrf_saadc_oversample_t oversampling;      ///< Oversampling configuration.
    nrf_saadc_burst_t      burst;             ///< Burst configuration.
    uint16_t               internal_timer_cc; ///< Internal timer capture and compare value.
    bool                   start_on_end;      ///< Flag indicating if the START task is to be triggered on the END event.
} nrfx_saadc_adv_config_t;

/** @brief SAADC driver event types. */
typedef enum
{
    NRFX_SAADC_EVT_DONE,          ///< Event generated when the buffer is filled with samples.
    NRFX_SAADC_EVT_LIMIT,         ///< Event generated when one of the limits is reached.
    NRFX_SAADC_EVT_CALIBRATEDONE, ///< Event generated when the calibration is complete.
    NRFX_SAADC_EVT_BUF_REQ,       ///< Event generated when the next buffer for continuous conversion is requested.
    NRFX_SAADC_EVT_READY,         ///< Event generated when the first buffer is acquired by the peripheral and sampling can be started.
    NRFX_SAADC_EVT_FINISHED,      ///< Event generated when all supplied buffers are filled with results.
} nrfx_saadc_evt_type_t;

/** @brief SAADC driver done event data. */
typedef struct
{
    nrf_saadc_value_t * p_buffer; ///< Pointer to the buffer with converted samples.
    uint16_t            size;     ///< Number of samples in the buffer.
} nrfx_saadc_done_evt_t;

/** @brief SAADC driver limit event data. */
typedef struct
{
    uint8_t           channel;    ///< Channel on which the limit was detected.
    nrf_saadc_limit_t limit_type; ///< Type of limit detected.
} nrfx_saadc_limit_evt_t;

/** @brief SAADC driver event structure. */
typedef struct
{
    nrfx_saadc_evt_type_t type; ///< Event type.
    union
    {
        nrfx_saadc_done_evt_t  done;  ///< Data for @ref NRFX_SAADC_EVT_DONE event.
        nrfx_saadc_limit_evt_t limit; ///< Data for @ref NRFX_SAADC_EVT_LIMIT event.
    } data;                           ///< Union to store event data.
} nrfx_saadc_evt_t;

/**
 * @brief SAADC driver event handler.
 *
 * When operating in the advanced mode:
 * - when the sampling is performed by the external timer, the external timer can be safely started
 *   on @ref NRFX_SAADC_EVT_READY and stopped on @ref NRFX_SAADC_EVT_FINISHED.
 * - call the @ref nrfx_saadc_buffer_set() on @ref NRFX_SAADC_EVT_BUF_REQ to achieve the continuous conversion.
 *
 * @param[in] p_event Pointer to an SAADC driver event. The event structure is allocated on
 *                    the stack, so it is valid only within the context of the event handler.
 */
typedef void (* nrfx_saadc_event_handler_t)(nrfx_saadc_evt_t const * p_event);

/**
 * @brief Function for initializing the SAADC driver.
 *
 * @param[in] interrupt_priority Interrupt priority.
 *
 * @retval NRFX_SUCCESS             Initialization was successful.
 * @retval NRFX_ERROR_INVALID_STATE The driver is already initialized.
 */
nrfx_err_t nrfx_saadc_init(uint8_t interrupt_priority);

/**
 * @brief Function for uninitializing the SAADC driver.
 *
 * This function stops all ongoing conversions and disables all channels.
 */
void nrfx_saadc_uninit(void);

/**
 * @brief Function for configuring the SAADC channels.
 *
 * @note The values of the @ref nrf_saadc_channel_config_t.burst fields in channel configurations
 *       are ignored. They will be overridden with the value suitable for the selected driver
 *       operation mode.
 * @note The desired mode (simple or advanced) must be set after the channels are configured.
 *
 * @param[in] p_channels    Pointer to the array of channel configuration structures.
 * @param[in] channel_count Number of channels to be configured.
 *
 * @retval NRFX_SUCCESS             Configuration was successful.
 * @retval NRFX_ERROR_BUSY          There is a conversion or calibration ongoing.
 * @retval NRFX_ERROR_INVALID_PARAM Attempt to configure the same channel more than once.
 */
nrfx_err_t nrfx_saadc_channels_config(nrfx_saadc_channel_t const * p_channels,
                                      uint32_t                     channel_count);

/**
 * @brief Function for setting the SAADC driver in the simple mode.
 *
 * The simple mode allows obtaining a single sample from each requested channel.
 * The conversion can be done in a blocking or non-blocking manner.
 * Sampling is initiated by calling @ref nrfx_saadc_mode_trigger() once.
 *
 * @param[in] channel_mask  Bitmask of channels to be used in the simple mode.
 * @param[in] resolution    Resolution configuration.
 * @param[in] oversampling  Oversampling configuration.
 * @param[in] event_handler Event handler provided by the user. In case of providing NULL,
 *                          the conversion will be performed in the blocking manner.
 *
 * @retval NRFX_SUCCESS             Initialization was successful.
 * @retval NRFX_ERROR_BUSY          There is a conversion or calibration ongoing.
 * @retval NRFX_ERROR_INVALID_PARAM Attempt to activate channel that is not configured.
 */
nrfx_err_t nrfx_saadc_simple_mode_set(uint32_t                   channel_mask,
                                      nrf_saadc_resolution_t     resolution,
                                      nrf_saadc_oversample_t     oversampling,
                                      nrfx_saadc_event_handler_t event_handler);

/**
 * @brief Function for setting the SAADC driver in the advanced mode.
 *
 * The advanced mode allows performing double-buffered conversions of arbitrary length.
 * The conversions can be done in a blocking or non-blocking manner. When performing conversions
 * in the non-blocking manner and @ref nrfx_saadc_adv_config_t.internal_timer_cc is set to 0,
 * sampling needs to be done by triggering @ref NRF_SAADC_TASK_SAMPLE externally
 * (for example by using the TIMER and/or the PPI/DPPI).
 * When performing conversions in the non-blocking manner and @ref nrfx_saadc_adv_config_t.start_on_end
 * is false, the @ref NRF_SAADC_TASK_START needs to be triggered on @ref NRF_SAADC_EVENT_END
 * externally (for example by using the PPI/DPPI).
 * Sampling is initiated by calling @ref nrfx_saadc_mode_trigger(). In case of performing
 * conversions in the blocking manner, @ref nrfx_saadc_mode_trigger() may need to be called several
 * times as each call sample each requested channel once.
 *
 * @note The internal timer can only be used when a single input channel is enabled.
 * @note The internal timer can only be used in the non-blocking mode.
 *
 * @param[in] channel_mask  Bitmask of channels to be used in the advanced mode.
 * @param[in] resolution    Resolution configuration.
 * @param[in] p_config      Pointer to the structure with the advanced mode configuration.
 * @param[in] event_handler Event handler provided by the user. In case of providing NULL,
 *                          the conversion will be performed in the blocking manner.
 *
 * @retval NRFX_SUCCESS             Initialization was successful.
 * @retval NRFX_ERROR_BUSY          There is a conversion or calibration ongoing.
 * @retval NRFX_ERROR_INVALID_PARAM Attempt to activate channel that is not configured.
 * @retval NRFX_ERROR_NOT_SUPPORTED Attempt to activate internal timer or oversampling without burst
 *                                  with multiple channels enabled.
 */
nrfx_err_t nrfx_saadc_advanced_mode_set(uint32_t                        channel_mask,
                                        nrf_saadc_resolution_t          resolution,
                                        nrfx_saadc_adv_config_t const * p_config,
                                        nrfx_saadc_event_handler_t      event_handler);

/**
 * @brief Function for supplying the buffer to be used in the next part of
 *        the conversion.
 *
 * @param[in] p_buffer Pointer to the buffer to be filled with conversion results.
 * @param[in] size     Number of @ref nrf_saadc_value_t samples in buffer.
 *
 * @retval NRFX_SUCCESS                   Buffer was supplied successfully.
 * @retval NRFX_ERROR_INVALID_ADDR        The provided buffer is not in the Data RAM region.
 * @retval NRFX_ERROR_INVALID_LENGTH      The provided buffer is not aligned to the number of activated channels
 *                                        or is too long for the EasyDMA to handle.
 * @retval NRFX_ERROR_INVALID_STATE       The driver is in the idle mode.
 * @retval NRFX_ERROR_ALREADY_INITIALIZED Both buffers for double-buffered conversions are already set.
 */
nrfx_err_t nrfx_saadc_buffer_set(nrf_saadc_value_t * p_buffer, uint16_t size);

/**
 * @brief Function for triggering the conversion in the configured mode.
 *
 * @retval NRFX_SUCCESS             Operation finished successfully in the blocking manner or started
 *                                  successfully in the non-blocking manner.
 * @retval NRFX_ERROR_BUSY          The driver is performing the conversion in the advanced blocking mode.
 *                                  Call the function again to continue the conversion.
 * @retval NRFX_ERROR_NO_MEM        There is no buffer provided.
 *                                  Supply the buffer using @ref nrfx_saadc_buffer_set() and try again.
 * @retval NRFX_ERROR_INVALID_STATE There is an ongoing conversion being performed in the non-blocking manner
 *                                  or the driver is in the idle mode.
 */
nrfx_err_t nrfx_saadc_mode_trigger(void);

/**
 * @brief Function for aborting the ongoing and buffered conversions.
 *
 * @note @ref NRFX_SAADC_EVT_DONE event will be generated if there is a conversion in progress.
 *       Event will contain number of words in the sample buffer.
 */
void nrfx_saadc_abort(void);

/**
 * @brief Function for setting the SAADC channel limits.
 *
 * When limits are enabled and the conversion result exceeds the defined bounds,
 * the handler function is called with the corresponding event as parameter.
 *
 * @note Before the limits are set, the driver operation mode (simple or advanced) has
 *       to be configured. Only non-blocking conversions can be monitored.
 *
 * @note Changing of the driver operation mode disables all configured limits.
 *
 * @param[in] channel    Channel index.
 * @param[in] limit_low  Limit low value to generate interrupt. Use @c INT16_MIN
 *                       to disable interrupt generation.
 * @param[in] limit_high Limit high value to generate interrupt. Use @c INT16_MAX
 *                       to disable interrupt generation.
 *
 * @retval NRFX_SUCCESS             Requested channel limits were set.
 * @retval NRFX_ERROR_INVALID_PARAM Attempt to activate the limits on disabled channel.
 * @retval NRFX_ERROR_FORBIDDEN     Attempt to activate the limits for blocking conversions.
 * @retval NRFX_ERROR_INVALID_STATE Attempt to activate the limits without configured mode.
 */
nrfx_err_t nrfx_saadc_limits_set(uint8_t channel, int16_t limit_low, int16_t limit_high);

/**
 * @brief Function for starting the SAADC offset calibration.
 *
 * @note This function cancels the currently selected driver operation mode, if any.
 *       The desired mode (simple or advanced) must be set after the calibration process completes.
 *
 * @param[in] event_handler Event handler provided by the user. In case of providing NULL,
 *                          the calibration will be performed in the blocking manner.
 *
 * @retval NRFX_SUCCESS    Calibration finished successfully in the blocking manner
 *                         or started successfully in the non-blocking manner.
 * @retval NRFX_ERROR_BUSY There is a conversion or calibration ongoing.
 */
nrfx_err_t nrfx_saadc_offset_calibrate(nrfx_saadc_event_handler_t event_handler);

/** @} */

void nrfx_saadc_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif // NRFX_SAADC_H__

