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

#ifndef NRF_SAADC_H_
#define NRF_SAADC_H_

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_saadc_hal SAADC HAL
 * @{
 * @ingroup nrf_saadc
 * @brief   Hardware access layer for managing the SAADC peripheral.
 */

/** @brief Resolution of the analog-to-digital converter. */
typedef enum
{
    NRF_SAADC_RESOLUTION_8BIT  = SAADC_RESOLUTION_VAL_8bit,  ///< 8 bit resolution.
    NRF_SAADC_RESOLUTION_10BIT = SAADC_RESOLUTION_VAL_10bit, ///< 10 bit resolution.
    NRF_SAADC_RESOLUTION_12BIT = SAADC_RESOLUTION_VAL_12bit, ///< 12 bit resolution.
    NRF_SAADC_RESOLUTION_14BIT = SAADC_RESOLUTION_VAL_14bit  ///< 14 bit resolution.
} nrf_saadc_resolution_t;

/** @brief Input selection for the analog-to-digital converter. */
typedef enum
{
    NRF_SAADC_INPUT_DISABLED = SAADC_CH_PSELP_PSELP_NC,           ///< Not connected.
    NRF_SAADC_INPUT_AIN0     = SAADC_CH_PSELP_PSELP_AnalogInput0, ///< Analog input 0 (AIN0).
    NRF_SAADC_INPUT_AIN1     = SAADC_CH_PSELP_PSELP_AnalogInput1, ///< Analog input 1 (AIN1).
    NRF_SAADC_INPUT_AIN2     = SAADC_CH_PSELP_PSELP_AnalogInput2, ///< Analog input 2 (AIN2).
    NRF_SAADC_INPUT_AIN3     = SAADC_CH_PSELP_PSELP_AnalogInput3, ///< Analog input 3 (AIN3).
    NRF_SAADC_INPUT_AIN4     = SAADC_CH_PSELP_PSELP_AnalogInput4, ///< Analog input 4 (AIN4).
    NRF_SAADC_INPUT_AIN5     = SAADC_CH_PSELP_PSELP_AnalogInput5, ///< Analog input 5 (AIN5).
    NRF_SAADC_INPUT_AIN6     = SAADC_CH_PSELP_PSELP_AnalogInput6, ///< Analog input 6 (AIN6).
    NRF_SAADC_INPUT_AIN7     = SAADC_CH_PSELP_PSELP_AnalogInput7, ///< Analog input 7 (AIN7).
    NRF_SAADC_INPUT_VDD      = SAADC_CH_PSELP_PSELP_VDD,          ///< VDD as input.
#if defined(SAADC_CH_PSELP_PSELP_VDDHDIV5) || defined(__NRFX_DOXYGEN__)
    NRF_SAADC_INPUT_VDDHDIV5 = SAADC_CH_PSELP_PSELP_VDDHDIV5      ///< VDDH/5 as input.
#endif
} nrf_saadc_input_t;

/** @brief Analog-to-digital converter oversampling mode. */
typedef enum
{
    NRF_SAADC_OVERSAMPLE_DISABLED = SAADC_OVERSAMPLE_OVERSAMPLE_Bypass,   ///< No oversampling.
    NRF_SAADC_OVERSAMPLE_2X       = SAADC_OVERSAMPLE_OVERSAMPLE_Over2x,   ///< Oversample 2x.
    NRF_SAADC_OVERSAMPLE_4X       = SAADC_OVERSAMPLE_OVERSAMPLE_Over4x,   ///< Oversample 4x.
    NRF_SAADC_OVERSAMPLE_8X       = SAADC_OVERSAMPLE_OVERSAMPLE_Over8x,   ///< Oversample 8x.
    NRF_SAADC_OVERSAMPLE_16X      = SAADC_OVERSAMPLE_OVERSAMPLE_Over16x,  ///< Oversample 16x.
    NRF_SAADC_OVERSAMPLE_32X      = SAADC_OVERSAMPLE_OVERSAMPLE_Over32x,  ///< Oversample 32x.
    NRF_SAADC_OVERSAMPLE_64X      = SAADC_OVERSAMPLE_OVERSAMPLE_Over64x,  ///< Oversample 64x.
    NRF_SAADC_OVERSAMPLE_128X     = SAADC_OVERSAMPLE_OVERSAMPLE_Over128x, ///< Oversample 128x.
    NRF_SAADC_OVERSAMPLE_256X     = SAADC_OVERSAMPLE_OVERSAMPLE_Over256x  ///< Oversample 256x.
} nrf_saadc_oversample_t;

/** @brief Analog-to-digital converter channel resistor control. */
typedef enum
{
    NRF_SAADC_RESISTOR_DISABLED = SAADC_CH_CONFIG_RESP_Bypass,   ///< Bypass resistor ladder.
    NRF_SAADC_RESISTOR_PULLDOWN = SAADC_CH_CONFIG_RESP_Pulldown, ///< Pull-down to GND.
    NRF_SAADC_RESISTOR_PULLUP   = SAADC_CH_CONFIG_RESP_Pullup,   ///< Pull-up to VDD.
    NRF_SAADC_RESISTOR_VDD1_2   = SAADC_CH_CONFIG_RESP_VDD1_2    ///< Set input at VDD/2.
} nrf_saadc_resistor_t;

/** @brief Gain factor of the analog-to-digital converter input. */
typedef enum
{
    NRF_SAADC_GAIN1_6 = SAADC_CH_CONFIG_GAIN_Gain1_6, ///< Gain factor 1/6.
    NRF_SAADC_GAIN1_5 = SAADC_CH_CONFIG_GAIN_Gain1_5, ///< Gain factor 1/5.
    NRF_SAADC_GAIN1_4 = SAADC_CH_CONFIG_GAIN_Gain1_4, ///< Gain factor 1/4.
    NRF_SAADC_GAIN1_3 = SAADC_CH_CONFIG_GAIN_Gain1_3, ///< Gain factor 1/3.
    NRF_SAADC_GAIN1_2 = SAADC_CH_CONFIG_GAIN_Gain1_2, ///< Gain factor 1/2.
    NRF_SAADC_GAIN1   = SAADC_CH_CONFIG_GAIN_Gain1,   ///< Gain factor 1.
    NRF_SAADC_GAIN2   = SAADC_CH_CONFIG_GAIN_Gain2,   ///< Gain factor 2.
    NRF_SAADC_GAIN4   = SAADC_CH_CONFIG_GAIN_Gain4,   ///< Gain factor 4.
} nrf_saadc_gain_t;

/** @brief Reference selection for the analog-to-digital converter. */
typedef enum
{
    NRF_SAADC_REFERENCE_INTERNAL = SAADC_CH_CONFIG_REFSEL_Internal, ///< Internal reference (0.6 V).
    NRF_SAADC_REFERENCE_VDD4     = SAADC_CH_CONFIG_REFSEL_VDD1_4    ///< VDD/4 as reference.
} nrf_saadc_reference_t;

/** @brief Analog-to-digital converter acquisition time. */
typedef enum
{
    NRF_SAADC_ACQTIME_3US  = SAADC_CH_CONFIG_TACQ_3us,  ///< 3 us.
    NRF_SAADC_ACQTIME_5US  = SAADC_CH_CONFIG_TACQ_5us,  ///< 5 us.
    NRF_SAADC_ACQTIME_10US = SAADC_CH_CONFIG_TACQ_10us, ///< 10 us.
    NRF_SAADC_ACQTIME_15US = SAADC_CH_CONFIG_TACQ_15us, ///< 15 us.
    NRF_SAADC_ACQTIME_20US = SAADC_CH_CONFIG_TACQ_20us, ///< 20 us.
    NRF_SAADC_ACQTIME_40US = SAADC_CH_CONFIG_TACQ_40us  ///< 40 us.
} nrf_saadc_acqtime_t;

/** @brief Analog-to-digital converter channel mode. */
typedef enum
{
    NRF_SAADC_MODE_SINGLE_ENDED = SAADC_CH_CONFIG_MODE_SE,  ///< Single-ended mode. PSELN will be ignored, negative input to ADC shorted to GND.
    NRF_SAADC_MODE_DIFFERENTIAL = SAADC_CH_CONFIG_MODE_Diff ///< Differential mode.
} nrf_saadc_mode_t;

/** @brief Analog-to-digital converter channel burst mode. */
typedef enum
{
    NRF_SAADC_BURST_DISABLED = SAADC_CH_CONFIG_BURST_Disabled, ///< Burst mode is disabled (normal operation).
    NRF_SAADC_BURST_ENABLED  = SAADC_CH_CONFIG_BURST_Enabled   ///< Burst mode is enabled. SAADC takes 2^OVERSAMPLE number of samples as fast as it can, and sends the average to Data RAM.
} nrf_saadc_burst_t;

/** @brief Analog-to-digital converter tasks. */
typedef enum
{
    NRF_SAADC_TASK_START           = offsetof(NRF_SAADC_Type, TASKS_START),           ///< Start the ADC and prepare the result buffer in RAM.
    NRF_SAADC_TASK_SAMPLE          = offsetof(NRF_SAADC_Type, TASKS_SAMPLE),          ///< Take one ADC sample. If scan is enabled, all channels are sampled.
    NRF_SAADC_TASK_STOP            = offsetof(NRF_SAADC_Type, TASKS_STOP),            ///< Stop the ADC and terminate any ongoing conversion.
    NRF_SAADC_TASK_CALIBRATEOFFSET = offsetof(NRF_SAADC_Type, TASKS_CALIBRATEOFFSET), ///< Starts offset auto-calibration.
} nrf_saadc_task_t;

/** @brief Analog-to-digital converter events. */
typedef enum
{
    NRF_SAADC_EVENT_STARTED       = offsetof(NRF_SAADC_Type, EVENTS_STARTED),       ///< The ADC has started.
    NRF_SAADC_EVENT_END           = offsetof(NRF_SAADC_Type, EVENTS_END),           ///< The ADC has filled up the result buffer.
    NRF_SAADC_EVENT_DONE          = offsetof(NRF_SAADC_Type, EVENTS_DONE),          ///< A conversion task has been completed.
    NRF_SAADC_EVENT_RESULTDONE    = offsetof(NRF_SAADC_Type, EVENTS_RESULTDONE),    ///< A result is ready to get transferred to RAM.
    NRF_SAADC_EVENT_CALIBRATEDONE = offsetof(NRF_SAADC_Type, EVENTS_CALIBRATEDONE), ///< Calibration is complete.
    NRF_SAADC_EVENT_STOPPED       = offsetof(NRF_SAADC_Type, EVENTS_STOPPED),       ///< The ADC has stopped.
    NRF_SAADC_EVENT_CH0_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[0].LIMITH),  ///< Last result is equal or above CH[0].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH0_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[0].LIMITL),  ///< Last result is equal or below CH[0].LIMIT.LOW.
    NRF_SAADC_EVENT_CH1_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[1].LIMITH),  ///< Last result is equal or above CH[1].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH1_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[1].LIMITL),  ///< Last result is equal or below CH[1].LIMIT.LOW.
    NRF_SAADC_EVENT_CH2_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[2].LIMITH),  ///< Last result is equal or above CH[2].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH2_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[2].LIMITL),  ///< Last result is equal or below CH[2].LIMIT.LOW.
    NRF_SAADC_EVENT_CH3_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[3].LIMITH),  ///< Last result is equal or above CH[3].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH3_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[3].LIMITL),  ///< Last result is equal or below CH[3].LIMIT.LOW.
    NRF_SAADC_EVENT_CH4_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[4].LIMITH),  ///< Last result is equal or above CH[4].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH4_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[4].LIMITL),  ///< Last result is equal or below CH[4].LIMIT.LOW.
    NRF_SAADC_EVENT_CH5_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[5].LIMITH),  ///< Last result is equal or above CH[5].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH5_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[5].LIMITL),  ///< Last result is equal or below CH[5].LIMIT.LOW.
    NRF_SAADC_EVENT_CH6_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[6].LIMITH),  ///< Last result is equal or above CH[6].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH6_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[6].LIMITL),  ///< Last result is equal or below CH[6].LIMIT.LOW.
    NRF_SAADC_EVENT_CH7_LIMITH    = offsetof(NRF_SAADC_Type, EVENTS_CH[7].LIMITH),  ///< Last result is equal or above CH[7].LIMIT.HIGH.
    NRF_SAADC_EVENT_CH7_LIMITL    = offsetof(NRF_SAADC_Type, EVENTS_CH[7].LIMITL)   ///< Last result is equal or below CH[7].LIMIT.LOW.
} nrf_saadc_event_t;

/** @brief Analog-to-digital converter interrupt masks. */
typedef enum
{
    NRF_SAADC_INT_STARTED       = SAADC_INTENSET_STARTED_Msk,       ///< Interrupt on EVENTS_STARTED event.
    NRF_SAADC_INT_END           = SAADC_INTENSET_END_Msk,           ///< Interrupt on EVENTS_END event.
    NRF_SAADC_INT_DONE          = SAADC_INTENSET_DONE_Msk,          ///< Interrupt on EVENTS_DONE event.
    NRF_SAADC_INT_RESULTDONE    = SAADC_INTENSET_RESULTDONE_Msk,    ///< Interrupt on EVENTS_RESULTDONE event.
    NRF_SAADC_INT_CALIBRATEDONE = SAADC_INTENSET_CALIBRATEDONE_Msk, ///< Interrupt on EVENTS_CALIBRATEDONE event.
    NRF_SAADC_INT_STOPPED       = SAADC_INTENSET_STOPPED_Msk,       ///< Interrupt on EVENTS_STOPPED event.
    NRF_SAADC_INT_CH0LIMITH     = SAADC_INTENSET_CH0LIMITH_Msk,     ///< Interrupt on EVENTS_CH[0].LIMITH event.
    NRF_SAADC_INT_CH0LIMITL     = SAADC_INTENSET_CH0LIMITL_Msk,     ///< Interrupt on EVENTS_CH[0].LIMITL event.
    NRF_SAADC_INT_CH1LIMITH     = SAADC_INTENSET_CH1LIMITH_Msk,     ///< Interrupt on EVENTS_CH[1].LIMITH event.
    NRF_SAADC_INT_CH1LIMITL     = SAADC_INTENSET_CH1LIMITL_Msk,     ///< Interrupt on EVENTS_CH[1].LIMITL event.
    NRF_SAADC_INT_CH2LIMITH     = SAADC_INTENSET_CH2LIMITH_Msk,     ///< Interrupt on EVENTS_CH[2].LIMITH event.
    NRF_SAADC_INT_CH2LIMITL     = SAADC_INTENSET_CH2LIMITL_Msk,     ///< Interrupt on EVENTS_CH[2].LIMITL event.
    NRF_SAADC_INT_CH3LIMITH     = SAADC_INTENSET_CH3LIMITH_Msk,     ///< Interrupt on EVENTS_CH[3].LIMITH event.
    NRF_SAADC_INT_CH3LIMITL     = SAADC_INTENSET_CH3LIMITL_Msk,     ///< Interrupt on EVENTS_CH[3].LIMITL event.
    NRF_SAADC_INT_CH4LIMITH     = SAADC_INTENSET_CH4LIMITH_Msk,     ///< Interrupt on EVENTS_CH[4].LIMITH event.
    NRF_SAADC_INT_CH4LIMITL     = SAADC_INTENSET_CH4LIMITL_Msk,     ///< Interrupt on EVENTS_CH[4].LIMITL event.
    NRF_SAADC_INT_CH5LIMITH     = SAADC_INTENSET_CH5LIMITH_Msk,     ///< Interrupt on EVENTS_CH[5].LIMITH event.
    NRF_SAADC_INT_CH5LIMITL     = SAADC_INTENSET_CH5LIMITL_Msk,     ///< Interrupt on EVENTS_CH[5].LIMITL event.
    NRF_SAADC_INT_CH6LIMITH     = SAADC_INTENSET_CH6LIMITH_Msk,     ///< Interrupt on EVENTS_CH[6].LIMITH event.
    NRF_SAADC_INT_CH6LIMITL     = SAADC_INTENSET_CH6LIMITL_Msk,     ///< Interrupt on EVENTS_CH[6].LIMITL event.
    NRF_SAADC_INT_CH7LIMITH     = SAADC_INTENSET_CH7LIMITH_Msk,     ///< Interrupt on EVENTS_CH[7].LIMITH event.
    NRF_SAADC_INT_CH7LIMITL     = SAADC_INTENSET_CH7LIMITL_Msk,     ///< Interrupt on EVENTS_CH[7].LIMITL event.
    NRF_SAADC_INT_ALL           = 0x7FFFFFFFUL                      ///< Mask of all interrupts.
} nrf_saadc_int_mask_t;

/** @brief Analog-to-digital converter value limit type. */
typedef enum
{
    NRF_SAADC_LIMIT_LOW  = 0, ///< Low limit type.
    NRF_SAADC_LIMIT_HIGH = 1  ///< High limit type.
} nrf_saadc_limit_t;

/** @brief Type of a single ADC conversion result. */
typedef int16_t nrf_saadc_value_t;

/** @brief Analog-to-digital converter configuration structure. */
typedef struct
{
    nrf_saadc_resolution_t resolution;  ///< Resolution of samples.
    nrf_saadc_oversample_t oversample;  ///< Oversampling configuration.
    nrf_saadc_value_t *    buffer;      ///< Pointer to sample buffer.
    uint32_t               buffer_size; ///< Size of the sample buffer.
} nrf_saadc_config_t;

/** @brief Analog-to-digital converter channel configuration structure. */
typedef struct
{
    nrf_saadc_resistor_t  resistor_p; ///< Resistor value on positive input.
    nrf_saadc_resistor_t  resistor_n; ///< Resistor value on negative input.
    nrf_saadc_gain_t      gain;       ///< Gain control value.
    nrf_saadc_reference_t reference;  ///< Reference control value.
    nrf_saadc_acqtime_t   acq_time;   ///< Acquisition time.
    nrf_saadc_mode_t      mode;       ///< SAADC mode. Single-ended or differential.
    nrf_saadc_burst_t     burst;      ///< Burst mode configuration.
} nrf_saadc_channel_config_t;


/**
 * @brief Function for triggering the specified SAADC task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  SAADC task.
 */
NRF_STATIC_INLINE void nrf_saadc_task_trigger(NRF_SAADC_Type * p_reg,
                                              nrf_saadc_task_t task);

/**
 * @brief Function for getting the address of the specified SAADC task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  SAADC task.
 *
 * @return Address of the specified SAADC task.
 */
NRF_STATIC_INLINE uint32_t nrf_saadc_task_address_get(NRF_SAADC_Type const * p_reg,
                                                      nrf_saadc_task_t       task);

/**
 * @brief Function for retrieving the state of the SAADC event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_saadc_event_check(NRF_SAADC_Type const * p_reg,
                                             nrf_saadc_event_t      event);

/**
 * @brief Function for clearing the specific SAADC event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event SAADC event.
 */
NRF_STATIC_INLINE void nrf_saadc_event_clear(NRF_SAADC_Type *  p_reg,
                                             nrf_saadc_event_t event);

/**
 * @brief Function for getting the address of the specified SAADC event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event SAADC event.
 *
 * @return Address of the specified SAADC event.
 */
NRF_STATIC_INLINE uint32_t nrf_saadc_event_address_get(NRF_SAADC_Type const * p_reg,
                                                       nrf_saadc_event_t      event);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        SAADC task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
NRF_STATIC_INLINE void nrf_saadc_subscribe_set(NRF_SAADC_Type * p_reg,
                                               nrf_saadc_task_t task,
                                               uint8_t          channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        SAADC task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_saadc_subscribe_clear(NRF_SAADC_Type * p_reg,
                                                 nrf_saadc_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        SAADC event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
NRF_STATIC_INLINE void nrf_saadc_publish_set(NRF_SAADC_Type *  p_reg,
                                             nrf_saadc_event_t event,
                                             uint8_t           channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        SAADC event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_saadc_publish_clear(NRF_SAADC_Type *  p_reg,
                                               nrf_saadc_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for getting the SAADC channel monitoring limit events.
 *
 * @param[in] channel    Channel number.
 * @param[in] limit_type Low limit or high limit.
 *
 * @return The SAADC channel monitoring limit event.
 */
NRF_STATIC_INLINE nrf_saadc_event_t nrf_saadc_limit_event_get(uint8_t           channel,
                                                              nrf_saadc_limit_t limit_type);

/**
 * @brief Function for configuring the input pins for the specified SAADC channel.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] channel Channel number.
 * @param[in] pselp   Positive input.
 * @param[in] pseln   Negative input. Set to NRF_SAADC_INPUT_DISABLED in single ended mode.
 */
NRF_STATIC_INLINE void nrf_saadc_channel_input_set(NRF_SAADC_Type *  p_reg,
                                                   uint8_t           channel,
                                                   nrf_saadc_input_t pselp,
                                                   nrf_saadc_input_t pseln);

/**
 * @brief Function for configuring the positive input pin for the specified SAADC channel.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] channel Channel number.
 * @param[in] pselp   Positive input.
 */
NRF_STATIC_INLINE void nrf_saadc_channel_pos_input_set(NRF_SAADC_Type *  p_reg,
                                                       uint8_t           channel,
                                                       nrf_saadc_input_t pselp);

/**
 * @brief Function for setting the SAADC channel monitoring limits.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] channel Channel number.
 * @param[in] low     Low limit.
 * @param[in] high    High limit.
 */
NRF_STATIC_INLINE void nrf_saadc_channel_limits_set(NRF_SAADC_Type * p_reg,
                                                    uint8_t          channel,
                                                    int16_t          low,
                                                    int16_t          high);

/**
 * @brief Function for setting the configuration of SAADC interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Interrupts configuration to be set.
 */
NRF_STATIC_INLINE void nrf_saadc_int_set(NRF_SAADC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for enabling specified SAADC interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
NRF_STATIC_INLINE void nrf_saadc_int_enable(NRF_SAADC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_saadc_int_enable_check(NRF_SAADC_Type const * p_reg, uint32_t mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
NRF_STATIC_INLINE void nrf_saadc_int_disable(NRF_SAADC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for generating masks for SAADC channel limit interrupts.
 *
 * @param[in] channel    SAADC channel number.
 * @param[in] limit_type Limit type.
 *
 * @return Interrupt mask.
 */
NRF_STATIC_INLINE uint32_t nrf_saadc_limit_int_get(uint8_t           channel,
                                                   nrf_saadc_limit_t limit_type);

/**
 * @brief Function for checking whether the SAADC is busy.
 *
 * This function checks whether the analog-to-digital converter is busy with a conversion.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  The SAADC is busy.
 * @retval false The SAADC is not busy.
 */
NRF_STATIC_INLINE bool nrf_saadc_busy_check(NRF_SAADC_Type const * p_reg);

/**
 * @brief Function for enabling the SAADC.
 *
 * The analog-to-digital converter must be enabled before use.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_saadc_enable(NRF_SAADC_Type * p_reg);

/**
 * @brief Function for disabling the SAADC.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_saadc_disable(NRF_SAADC_Type * p_reg);

/**
 * @brief Function for checking if the SAADC is enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  The SAADC is enabled.
 * @retval false The SAADC is not enabled.
 */
NRF_STATIC_INLINE bool nrf_saadc_enable_check(NRF_SAADC_Type const * p_reg);

/**
 * @brief Function for initializing the SAADC result buffer.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_buffer Pointer to the result buffer.
 * @param[in] size     Size of the buffer (in 16-bit samples).
 */
NRF_STATIC_INLINE void nrf_saadc_buffer_init(NRF_SAADC_Type *    p_reg,
                                             nrf_saadc_value_t * p_buffer,
                                             uint32_t            size);

/**
 * @brief Function for setting the SAADC result buffer pointer.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] p_buffer Pointer to the result buffer.
 */
NRF_STATIC_INLINE void nrf_saadc_buffer_pointer_set(NRF_SAADC_Type *    p_reg,
                                                    nrf_saadc_value_t * p_buffer);

/**
 * @brief Function for getting the SAADC result buffer pointer.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Pointer to the result buffer.
 */
NRF_STATIC_INLINE nrf_saadc_value_t * nrf_saadc_buffer_pointer_get(NRF_SAADC_Type const * p_reg);

/**
 * @brief Function for getting the number of samples written to the result
 *        buffer since the previous START task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Number of 16-bit samples written to the buffer.
 */
NRF_STATIC_INLINE uint16_t nrf_saadc_amount_get(NRF_SAADC_Type const * p_reg);

/**
 * @brief Function for setting the SAADC sample resolution.
 *
 * @param[in] p_reg      Pointer to the structure of registers of the peripheral.
 * @param[in] resolution Bit resolution.
 */
NRF_STATIC_INLINE void nrf_saadc_resolution_set(NRF_SAADC_Type *       p_reg,
                                                nrf_saadc_resolution_t resolution);

/**
 * @brief Function for getting the SAADC sample resolution.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Sample resolution.
 */
NRF_STATIC_INLINE nrf_saadc_resolution_t nrf_saadc_resolution_get(NRF_SAADC_Type const * p_reg);

/**
 * @brief Function for configuring the oversampling feature.
 *
 * @param[in] p_reg      Pointer to the structure of registers of the peripheral.
 * @param[in] oversample Oversampling mode.
 */
NRF_STATIC_INLINE void nrf_saadc_oversample_set(NRF_SAADC_Type *       p_reg,
                                                nrf_saadc_oversample_t oversample);

/**
 * @brief Function for getting the oversampling feature configuration.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Oversampling configuration.
 */
NRF_STATIC_INLINE nrf_saadc_oversample_t nrf_saadc_oversample_get(NRF_SAADC_Type const * p_reg);

/**
 * @brief Function for getting the sample count needed for one averaged result for a given
 *        oversampling configuration.
 *
 * @param[in] oversample Oversampling configuration.
 *
 * @return Sample count.
 */
NRF_STATIC_INLINE uint32_t nrf_saadc_oversample_sample_count_get(nrf_saadc_oversample_t oversample);

/**
 * @brief Function for enabling the continuous sampling.
 *
 * This function configures the SAADC internal timer to automatically take new samples at a fixed
 * sample rate. Trigger the START task to begin continuous sampling. To stop the sampling, trigger
 * the STOP task.
 *
 * @note The internal timer can only be used when a single input channel is enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] cc    Capture and compare value. Sample rate is 16 MHz/cc.
 *                  Valid @c CC range is from 80 to 2047.
 */
NRF_STATIC_INLINE void nrf_saadc_continuous_mode_enable(NRF_SAADC_Type * p_reg,
                                                        uint16_t         cc);

/**
 * @brief Function for checking if the continuous sampling is enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  The continuous sampling is enabled.
 * @retval false The continuous sampling is disabled.
 */
NRF_STATIC_INLINE bool nrf_saadc_continuous_mode_enable_check(NRF_SAADC_Type const * p_reg);

/**
 * @brief Function for disabling the continuous sampling.
 *
 * New samples can still be acquired by manually triggering the SAMPLE task or by PPI.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_saadc_continuous_mode_disable(NRF_SAADC_Type * p_reg);

/**
 * @brief Function for initializing the SAADC channel.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] channel Channel number.
 * @param[in] config  Pointer to the channel configuration structure.
 */
NRF_STATIC_INLINE void nrf_saadc_channel_init(NRF_SAADC_Type *                   p_reg,
                                              uint8_t                            channel,
                                              nrf_saadc_channel_config_t const * config);

/**
 * @brief Function for configuring the burst mode for the specified channel.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] channel Channel number.
 * @param[in] burst   Burst mode setting.
 */
NRF_STATIC_INLINE void nrf_saadc_burst_set(NRF_SAADC_Type *  p_reg,
                                           uint8_t           channel,
                                           nrf_saadc_burst_t burst);

/**
 * @brief Function for getting the minimum value of the conversion result.
 *
 * The minimum value of the conversion result depends on the configured resolution.
 *
 * @param[in] resolution Bit resolution.
 *
 * @return Minimum value of the conversion result.
 */
NRF_STATIC_INLINE nrf_saadc_value_t nrf_saadc_value_min_get(nrf_saadc_resolution_t resolution);

/**
 * @brief Function for getting the maximum value of the conversion result.
 *
 * The maximum value of the conversion result depends on the configured resolution.
 *
 * @param[in] resolution Bit resolution.
 *
 * @return Maximum value of the conversion result.
 */
NRF_STATIC_INLINE nrf_saadc_value_t nrf_saadc_value_max_get(nrf_saadc_resolution_t resolution);

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_saadc_task_trigger(NRF_SAADC_Type * p_reg, nrf_saadc_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

NRF_STATIC_INLINE uint32_t nrf_saadc_task_address_get(NRF_SAADC_Type const * p_reg,
                                                      nrf_saadc_task_t       task)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)task);
}

NRF_STATIC_INLINE bool nrf_saadc_event_check(NRF_SAADC_Type const * p_reg, nrf_saadc_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_saadc_event_clear(NRF_SAADC_Type * p_reg, nrf_saadc_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
    nrf_event_readback((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE uint32_t  nrf_saadc_event_address_get(NRF_SAADC_Type const * p_reg,
                                                        nrf_saadc_event_t      event)
{
    return (uint32_t )((uint8_t *)p_reg + (uint32_t)event);
}

#if defined(DPPI_PRESENT)
NRF_STATIC_INLINE void nrf_saadc_subscribe_set(NRF_SAADC_Type * p_reg,
                                               nrf_saadc_task_t task,
                                               uint8_t          channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | SAADC_SUBSCRIBE_START_EN_Msk);
}

NRF_STATIC_INLINE void nrf_saadc_subscribe_clear(NRF_SAADC_Type * p_reg, nrf_saadc_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

NRF_STATIC_INLINE void nrf_saadc_publish_set(NRF_SAADC_Type *  p_reg,
                                             nrf_saadc_event_t event,
                                             uint8_t           channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | SAADC_PUBLISH_STARTED_EN_Msk);
}

NRF_STATIC_INLINE void nrf_saadc_publish_clear(NRF_SAADC_Type * p_reg, nrf_saadc_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

NRF_STATIC_INLINE nrf_saadc_event_t nrf_saadc_limit_event_get(uint8_t           channel,
                                                              nrf_saadc_limit_t limit_type)
{
    if (limit_type == NRF_SAADC_LIMIT_HIGH)
    {
        return (nrf_saadc_event_t)NRFX_OFFSETOF(NRF_SAADC_Type, EVENTS_CH[channel].LIMITH);
    }
    else
    {
        return (nrf_saadc_event_t)NRFX_OFFSETOF(NRF_SAADC_Type, EVENTS_CH[channel].LIMITL);
    }
}

NRF_STATIC_INLINE void nrf_saadc_channel_input_set(NRF_SAADC_Type *  p_reg,
                                                   uint8_t           channel,
                                                   nrf_saadc_input_t pselp,
                                                   nrf_saadc_input_t pseln)
{
    p_reg->CH[channel].PSELN = pseln;
    p_reg->CH[channel].PSELP = pselp;
}

NRF_STATIC_INLINE void nrf_saadc_channel_pos_input_set(NRF_SAADC_Type *  p_reg,
                                                       uint8_t           channel,
                                                       nrf_saadc_input_t pselp)
{
    p_reg->CH[channel].PSELP = pselp;
}

NRF_STATIC_INLINE void nrf_saadc_channel_limits_set(NRF_SAADC_Type * p_reg,
                                                    uint8_t          channel,
                                                    int16_t          low,
                                                    int16_t          high)
{
    p_reg->CH[channel].LIMIT = (
            (((uint32_t) low << SAADC_CH_LIMIT_LOW_Pos) & SAADC_CH_LIMIT_LOW_Msk)
          | (((uint32_t) high << SAADC_CH_LIMIT_HIGH_Pos) & SAADC_CH_LIMIT_HIGH_Msk));
}

NRF_STATIC_INLINE void nrf_saadc_int_set(NRF_SAADC_Type * p_reg, uint32_t mask)
{
    p_reg->INTEN = mask;
}

NRF_STATIC_INLINE void nrf_saadc_int_enable(NRF_SAADC_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE uint32_t nrf_saadc_int_enable_check(NRF_SAADC_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

NRF_STATIC_INLINE void nrf_saadc_int_disable(NRF_SAADC_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

NRF_STATIC_INLINE uint32_t nrf_saadc_limit_int_get(uint8_t           channel,
                                                   nrf_saadc_limit_t limit_type)
{
    NRFX_ASSERT(channel < SAADC_CH_NUM);
    uint32_t mask = (limit_type == NRF_SAADC_LIMIT_LOW) ?
                     NRF_SAADC_INT_CH0LIMITL : NRF_SAADC_INT_CH0LIMITH;
    return mask << (channel * 2);
}

NRF_STATIC_INLINE bool nrf_saadc_busy_check(NRF_SAADC_Type const * p_reg)
{
    return (p_reg->STATUS == (SAADC_STATUS_STATUS_Busy << SAADC_STATUS_STATUS_Pos));
}

NRF_STATIC_INLINE void nrf_saadc_enable(NRF_SAADC_Type * p_reg)
{
    p_reg->ENABLE = (SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos);
}

NRF_STATIC_INLINE void nrf_saadc_disable(NRF_SAADC_Type * p_reg)
{
    p_reg->ENABLE = (SAADC_ENABLE_ENABLE_Disabled << SAADC_ENABLE_ENABLE_Pos);
}

NRF_STATIC_INLINE bool nrf_saadc_enable_check(NRF_SAADC_Type const * p_reg)
{
    return (p_reg->ENABLE == (SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos));
}

NRF_STATIC_INLINE void nrf_saadc_buffer_init(NRF_SAADC_Type *    p_reg,
                                             nrf_saadc_value_t * p_buffer,
                                             uint32_t            size)
{
    p_reg->RESULT.PTR = (uint32_t)p_buffer;
    p_reg->RESULT.MAXCNT = size;
}

NRF_STATIC_INLINE void nrf_saadc_buffer_pointer_set(NRF_SAADC_Type *    p_reg,
                                                    nrf_saadc_value_t * p_buffer)
{
    p_reg->RESULT.PTR = (uint32_t)p_buffer;
}

NRF_STATIC_INLINE nrf_saadc_value_t * nrf_saadc_buffer_pointer_get(NRF_SAADC_Type const * p_reg)
{
    return (nrf_saadc_value_t *)p_reg->RESULT.PTR;
}

NRF_STATIC_INLINE uint16_t nrf_saadc_amount_get(NRF_SAADC_Type const * p_reg)
{
    return p_reg->RESULT.AMOUNT;
}

NRF_STATIC_INLINE void nrf_saadc_resolution_set(NRF_SAADC_Type *       p_reg,
                                                nrf_saadc_resolution_t resolution)
{
    p_reg->RESOLUTION = resolution;
}

NRF_STATIC_INLINE nrf_saadc_resolution_t nrf_saadc_resolution_get(NRF_SAADC_Type const * p_reg)
{
    return (nrf_saadc_resolution_t)p_reg->RESOLUTION;
}

NRF_STATIC_INLINE void nrf_saadc_oversample_set(NRF_SAADC_Type *       p_reg,
                                                nrf_saadc_oversample_t oversample)
{
    p_reg->OVERSAMPLE = oversample;
}

NRF_STATIC_INLINE nrf_saadc_oversample_t nrf_saadc_oversample_get(NRF_SAADC_Type const * p_reg)
{
    return (nrf_saadc_oversample_t)p_reg->OVERSAMPLE;
}

NRF_STATIC_INLINE uint32_t nrf_saadc_oversample_sample_count_get(nrf_saadc_oversample_t oversample)
{
    return (1 << (uint32_t)oversample);
}

NRF_STATIC_INLINE void nrf_saadc_continuous_mode_enable(NRF_SAADC_Type * p_reg, uint16_t cc)
{
    NRFX_ASSERT((cc >= 80) && (cc <= 2047));
    p_reg->SAMPLERATE = (SAADC_SAMPLERATE_MODE_Timers << SAADC_SAMPLERATE_MODE_Pos)
                        | ((uint32_t)cc << SAADC_SAMPLERATE_CC_Pos);
}

NRF_STATIC_INLINE bool nrf_saadc_continuous_mode_enable_check(NRF_SAADC_Type const * p_reg)
{
    return (bool)((p_reg->SAMPLERATE & SAADC_SAMPLERATE_MODE_Msk)
                   == (SAADC_SAMPLERATE_MODE_Timers << SAADC_SAMPLERATE_MODE_Pos));
}

NRF_STATIC_INLINE void nrf_saadc_continuous_mode_disable(NRF_SAADC_Type * p_reg)
{
    p_reg->SAMPLERATE = SAADC_SAMPLERATE_MODE_Task << SAADC_SAMPLERATE_MODE_Pos;
}

NRF_STATIC_INLINE void nrf_saadc_channel_init(NRF_SAADC_Type *                   p_reg,
                                              uint8_t                            channel,
                                              nrf_saadc_channel_config_t const * config)
{
    p_reg->CH[channel].CONFIG =
            ((config->resistor_p   << SAADC_CH_CONFIG_RESP_Pos)   & SAADC_CH_CONFIG_RESP_Msk)
            | ((config->resistor_n << SAADC_CH_CONFIG_RESN_Pos)   & SAADC_CH_CONFIG_RESN_Msk)
            | ((config->gain       << SAADC_CH_CONFIG_GAIN_Pos)   & SAADC_CH_CONFIG_GAIN_Msk)
            | ((config->reference  << SAADC_CH_CONFIG_REFSEL_Pos) & SAADC_CH_CONFIG_REFSEL_Msk)
            | ((config->acq_time   << SAADC_CH_CONFIG_TACQ_Pos)   & SAADC_CH_CONFIG_TACQ_Msk)
            | ((config->mode       << SAADC_CH_CONFIG_MODE_Pos)   & SAADC_CH_CONFIG_MODE_Msk)
            | ((config->burst      << SAADC_CH_CONFIG_BURST_Pos)  & SAADC_CH_CONFIG_BURST_Msk);
}

NRF_STATIC_INLINE void nrf_saadc_burst_set(NRF_SAADC_Type *  p_reg,
                                           uint8_t           channel,
                                           nrf_saadc_burst_t burst)
{
    p_reg->CH[channel].CONFIG = (p_reg->CH[channel].CONFIG & ~SAADC_CH_CONFIG_BURST_Msk) |
                                (burst << SAADC_CH_CONFIG_BURST_Pos);
}

NRF_STATIC_INLINE nrf_saadc_value_t nrf_saadc_value_min_get(nrf_saadc_resolution_t resolution)
{
    uint8_t res_bits = 0;
    switch (resolution)
    {
        case NRF_SAADC_RESOLUTION_8BIT:
            res_bits = 8;
            break;
        case NRF_SAADC_RESOLUTION_10BIT:
            res_bits = 10;
            break;
        case NRF_SAADC_RESOLUTION_12BIT:
            res_bits = 12;
            break;
        case NRF_SAADC_RESOLUTION_14BIT:
            res_bits = 14;
            break;
        default:
            NRFX_ASSERT(false);
    }
    return (nrf_saadc_value_t)(-(1 << res_bits));
}

NRF_STATIC_INLINE nrf_saadc_value_t nrf_saadc_value_max_get(nrf_saadc_resolution_t resolution)
{
    uint8_t res_bits = 0;
    switch (resolution)
    {
        case NRF_SAADC_RESOLUTION_8BIT:
            res_bits = 8;
            break;
        case NRF_SAADC_RESOLUTION_10BIT:
            res_bits = 10;
            break;
        case NRF_SAADC_RESOLUTION_12BIT:
            res_bits = 12;
            break;
        case NRF_SAADC_RESOLUTION_14BIT:
            res_bits = 14;
            break;
        default:
            NRFX_ASSERT(false);
    }
    return (nrf_saadc_value_t)((1 << res_bits) - 1);
}

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_SAADC_H_
