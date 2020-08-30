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
#include <nrfx.h>

#if NRFX_CHECK(NRFX_SAADC_ENABLED)
#include <nrfx_saadc.h>

#define NRFX_LOG_MODULE SAADC
#include <nrfx_log.h>

#if defined(NRF52_SERIES) && !defined(USE_WORKAROUND_FOR_ANOMALY_212)
    // ANOMALY 212 - SAADC events are missing when switching from single channel
    //               to multi channel configuration with burst enabled.
    #define USE_WORKAROUND_FOR_ANOMALY_212 1
#endif

#if defined(NRF53_SERIES) || defined(NRF91_SERIES)
    // Make sure that SAADC is stopped before channel configuration.
    #define STOP_SAADC_ON_CHANNEL_CONFIG 1

    // Make sure that SAADC calibration samples do not affect next conversions.
    #define INTERCEPT_SAADC_CALIBRATION_SAMPLES 1
#endif

/** @brief SAADC driver states.*/
typedef enum
{
    NRF_SAADC_STATE_UNINITIALIZED = 0,
    NRF_SAADC_STATE_IDLE,
    NRF_SAADC_STATE_SIMPLE_MODE,
    NRF_SAADC_STATE_SIMPLE_MODE_SAMPLE,
    NRF_SAADC_STATE_ADV_MODE,
    NRF_SAADC_STATE_ADV_MODE_SAMPLE,
    NRF_SAADC_STATE_ADV_MODE_SAMPLE_STARTED,
    NRF_SAADC_STATE_CALIBRATION
} nrf_saadc_state_t;

/** @brief SAADC control block.*/
typedef struct
{
    nrfx_saadc_event_handler_t event_handler;                ///< Event handler function pointer.
    nrf_saadc_value_t *        p_buffer_primary;             ///< Pointer to the primary result buffer.
    nrf_saadc_value_t *        p_buffer_secondary;           ///< Pointer to the secondary result buffer.
#if NRFX_CHECK(INTERCEPT_SAADC_CALIBRATION_SAMPLES)
    nrf_saadc_value_t          calib_samples[6];             ///< Scratch buffer for calibration samples.
#endif
    uint16_t                   size_primary;                 ///< Size of the primary result buffer.
    uint16_t                   size_secondary;               ///< Size of the secondary result buffer.
    uint16_t                   samples_converted;            ///< Number of samples present in result buffer when in the blocking mode.
    nrf_saadc_input_t          channels_pselp[SAADC_CH_NUM]; ///< Array holding each channel positive input.
    nrf_saadc_input_t          channels_pseln[SAADC_CH_NUM]; ///< Array holding each channel negative input.
    nrf_saadc_state_t          saadc_state;                  ///< State of the SAADC driver.
    uint8_t                    channels_configured;          ///< Bitmask of the configured channels.
    uint8_t                    channels_activated;           ///< Bitmask of the activated channels.
    uint8_t                    channels_activated_count;     ///< Number of the activated channels.
    uint8_t                    limits_low_activated;         ///< Bitmask of the activated low limits.
    uint8_t                    limits_high_activated;        ///< Bitmask of the activated high limits.
    bool                       start_on_end;                 ///< Flag indicating if the START task is to be triggered on the END event.
    bool                       oversampling_without_burst;   ///< Flag indicating whether oversampling without burst is configured.
} nrfx_saadc_cb_t;

static nrfx_saadc_cb_t m_cb;

#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_212)
static void saadc_anomaly_212_workaround_apply(void)
{
    uint32_t c[SAADC_CH_NUM];
    uint32_t l[SAADC_CH_NUM];

    for (uint32_t i = 0; i < SAADC_CH_NUM; i++)
    {
        c[i] = NRF_SAADC->CH[i].CONFIG;
        l[i] = NRF_SAADC->CH[i].LIMIT;
    }
    nrf_saadc_resolution_t resolution = nrf_saadc_resolution_get(NRF_SAADC);
    uint32_t u640 = *(volatile uint32_t *)0x40007640;
    uint32_t u644 = *(volatile uint32_t *)0x40007644;
    uint32_t u648 = *(volatile uint32_t *)0x40007648;

    *(volatile uint32_t *)0x40007FFC = 0;
    *(volatile uint32_t *)0x40007FFC = 1;

    for (uint32_t i = 0; i < SAADC_CH_NUM; i++)
    {
        NRF_SAADC->CH[i].CONFIG = c[i];
        NRF_SAADC->CH[i].LIMIT = l[i];
    }
    *(volatile uint32_t *)0x40007640 = u640;
    *(volatile uint32_t *)0x40007644 = u644;
    *(volatile uint32_t *)0x40007648 = u648;
    nrf_saadc_resolution_set(NRF_SAADC, resolution);
}
#endif // NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_212)

static nrfx_err_t saadc_channel_count_get(uint32_t  ch_to_activate_mask,
                                          uint8_t * p_active_ch_count)
{
    NRFX_ASSERT(ch_to_activate_mask);
    NRFX_ASSERT(ch_to_activate_mask < (1 << SAADC_CH_NUM));

    uint8_t active_ch_count = 0;
    for (uint32_t ch_mask = 1; ch_mask < (1 << SAADC_CH_NUM); ch_mask <<= 1)
    {
        if (ch_to_activate_mask & ch_mask)
        {
            // Check if requested channels are configured.
            if (!(m_cb.channels_configured & ch_mask))
            {
                return NRFX_ERROR_INVALID_PARAM;
            }
            active_ch_count++;
        }
    }

    *p_active_ch_count = active_ch_count;
    return NRFX_SUCCESS;
}

static bool saadc_busy_check(void)
{
    if ((m_cb.saadc_state == NRF_SAADC_STATE_IDLE)     ||
        (m_cb.saadc_state == NRF_SAADC_STATE_ADV_MODE) ||
        (m_cb.saadc_state == NRF_SAADC_STATE_SIMPLE_MODE))
    {
        return false;
    }
    else
    {
        return true;
    }
}

static void saadc_generic_mode_set(uint32_t                   ch_to_activate_mask,
                                   nrf_saadc_resolution_t     resolution,
                                   nrf_saadc_oversample_t     oversampling,
                                   nrf_saadc_burst_t          burst,
                                   nrfx_saadc_event_handler_t event_handler)
{
#if NRFX_CHECK(USE_WORKAROUND_FOR_ANOMALY_212)
    saadc_anomaly_212_workaround_apply();
#endif

#if NRFX_CHECK(STOP_SAADC_ON_CHANNEL_CONFIG)
    nrf_saadc_int_disable(NRF_SAADC, NRF_SAADC_INT_STOPPED);
    nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_STOP);
    while (!nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_STOPPED))
    {}
    nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_STOPPED);
#endif

    m_cb.limits_low_activated = 0;
    m_cb.limits_high_activated = 0;

    m_cb.p_buffer_primary = NULL;
    m_cb.p_buffer_secondary = NULL;
    m_cb.event_handler = event_handler;
    m_cb.channels_activated = ch_to_activate_mask;
    m_cb.samples_converted = 0;

    nrf_saadc_resolution_set(NRF_SAADC, resolution);
    nrf_saadc_oversample_set(NRF_SAADC, oversampling);
    if (event_handler)
    {
        nrf_saadc_int_set(NRF_SAADC,
                          NRF_SAADC_INT_STARTED |
                          NRF_SAADC_INT_STOPPED |
                          NRF_SAADC_INT_END);
    }
    else
    {
        nrf_saadc_int_set(NRF_SAADC, 0);
    }

    for (uint32_t ch_pos = 0; ch_pos < SAADC_CH_NUM; ch_pos++)
    {
        nrf_saadc_burst_t burst_to_set;
        nrf_saadc_input_t pselp;
        nrf_saadc_input_t pseln;
        if (ch_to_activate_mask & (1 << ch_pos))
        {
            pselp = m_cb.channels_pselp[ch_pos];
            pseln = m_cb.channels_pseln[ch_pos];
            burst_to_set = burst;
        }
        else
        {
            pselp = NRF_SAADC_INPUT_DISABLED;
            pseln = NRF_SAADC_INPUT_DISABLED;
            burst_to_set = NRF_SAADC_BURST_DISABLED;
        }
        nrf_saadc_burst_set(NRF_SAADC, ch_pos, burst_to_set);
        nrf_saadc_channel_input_set(NRF_SAADC, ch_pos, pselp, pseln);
    }
}

nrfx_err_t nrfx_saadc_init(uint8_t interrupt_priority)
{
    nrfx_err_t err_code;
    if (m_cb.saadc_state != NRF_SAADC_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }
    m_cb.saadc_state = NRF_SAADC_STATE_IDLE;

    nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_STARTED);
    nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_STOPPED);
    nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_END);
    nrf_saadc_int_set(NRF_SAADC, 0);
    NRFX_IRQ_ENABLE(SAADC_IRQn);
    NRFX_IRQ_PRIORITY_SET(SAADC_IRQn, interrupt_priority);

    err_code = NRFX_SUCCESS;
    NRFX_LOG_INFO("Function: %s, error code: %s.", __func__, NRFX_LOG_ERROR_STRING_GET(err_code));

    return err_code;
}

void nrfx_saadc_uninit(void)
{
    nrfx_saadc_abort();
    NRFX_IRQ_DISABLE(SAADC_IRQn);
    nrf_saadc_disable(NRF_SAADC);
    m_cb.saadc_state = NRF_SAADC_STATE_UNINITIALIZED;
}

nrfx_err_t nrfx_saadc_channels_config(nrfx_saadc_channel_t const * p_channels,
                                      uint32_t                     channel_count)
{
    NRFX_ASSERT(m_cb.saadc_state != NRF_SAADC_STATE_UNINITIALIZED);
    NRFX_ASSERT(channel_count <= SAADC_CH_NUM);

    if (saadc_busy_check())
    {
        return NRFX_ERROR_BUSY;
    }

    m_cb.channels_configured = 0;
    uint8_t i = 0;

    for (; i < SAADC_CH_NUM; i++)
    {
        m_cb.channels_pselp[i] = NRF_SAADC_INPUT_DISABLED;
        m_cb.channels_pseln[i] = NRF_SAADC_INPUT_DISABLED;
    }

    for (i = 0; i < channel_count; i++)
    {
        if (m_cb.channels_configured & (1 << p_channels[i].channel_index))
        {
            // This channel is already configured!
            return NRFX_ERROR_INVALID_PARAM;
        }
        nrf_saadc_channel_init(NRF_SAADC,
                               p_channels[i].channel_index,
                               &p_channels[i].channel_config);

        NRFX_ASSERT(p_channels[i].pin_p != NRF_SAADC_INPUT_DISABLED);
        m_cb.channels_pselp[p_channels[i].channel_index] = p_channels[i].pin_p;
        m_cb.channels_pseln[p_channels[i].channel_index] = p_channels[i].pin_n;
        m_cb.channels_configured |= 1U << p_channels[i].channel_index;
    }

    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_saadc_simple_mode_set(uint32_t                   channel_mask,
                                      nrf_saadc_resolution_t     resolution,
                                      nrf_saadc_oversample_t     oversampling,
                                      nrfx_saadc_event_handler_t event_handler)
{
    NRFX_ASSERT(m_cb.saadc_state != NRF_SAADC_STATE_UNINITIALIZED);

    if (saadc_busy_check())
    {
        return NRFX_ERROR_BUSY;
    }

    uint8_t active_ch_count;
    nrfx_err_t err = saadc_channel_count_get(channel_mask, &active_ch_count);
    if (err != NRFX_SUCCESS)
    {
        return err;
    }

    nrf_saadc_burst_t burst;
    if (oversampling == NRF_SAADC_OVERSAMPLE_DISABLED)
    {
        burst = NRF_SAADC_BURST_DISABLED;
    }
    else
    {
        // Burst is implicitly enabled if oversampling is enabled.
        burst = NRF_SAADC_BURST_ENABLED;
    }

    saadc_generic_mode_set(channel_mask,
                           resolution,
                           oversampling,
                           burst,
                           event_handler);

    m_cb.channels_activated_count = active_ch_count;
    m_cb.saadc_state = NRF_SAADC_STATE_SIMPLE_MODE;

    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_saadc_advanced_mode_set(uint32_t                        channel_mask,
                                        nrf_saadc_resolution_t          resolution,
                                        nrfx_saadc_adv_config_t const * p_config,
                                        nrfx_saadc_event_handler_t      event_handler)
{
    NRFX_ASSERT(m_cb.saadc_state != NRF_SAADC_STATE_UNINITIALIZED);
    NRFX_ASSERT(p_config);

    if (saadc_busy_check())
    {
        return NRFX_ERROR_BUSY;
    }

    uint8_t active_ch_count;
    nrfx_err_t err = saadc_channel_count_get(channel_mask, &active_ch_count);
    if (err != NRFX_SUCCESS)
    {
        return err;
    }

    if ((p_config->internal_timer_cc) && ((active_ch_count > 1) || (!event_handler)))
    {
        return NRFX_ERROR_NOT_SUPPORTED;
    }

    bool oversampling_without_burst = false;
    if ((p_config->oversampling != NRF_SAADC_OVERSAMPLE_DISABLED) &&
        (p_config->burst == NRF_SAADC_BURST_DISABLED))
    {
        if (active_ch_count > 1)
        {
            // Oversampling without burst is possible only on single channel.
            return NRFX_ERROR_NOT_SUPPORTED;
        }
        else
        {
            oversampling_without_burst = true;
        }
    }

    saadc_generic_mode_set(channel_mask,
                           resolution,
                           p_config->oversampling,
                           p_config->burst,
                           event_handler);

    if (p_config->internal_timer_cc)
    {
        nrf_saadc_continuous_mode_enable(NRF_SAADC, p_config->internal_timer_cc);
    }
    else
    {
        nrf_saadc_continuous_mode_disable(NRF_SAADC);
    }

    m_cb.channels_activated_count = active_ch_count;
    m_cb.start_on_end = p_config->start_on_end;
    m_cb.oversampling_without_burst = oversampling_without_burst;

    m_cb.saadc_state = NRF_SAADC_STATE_ADV_MODE;

    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_saadc_buffer_set(nrf_saadc_value_t * p_buffer, uint16_t size)
{
    NRFX_ASSERT(m_cb.saadc_state != NRF_SAADC_STATE_UNINITIALIZED);

    if (m_cb.p_buffer_secondary)
    {
        return NRFX_ERROR_ALREADY_INITIALIZED;
    }

    if (!nrfx_is_in_ram(p_buffer))
    {
        return NRFX_ERROR_INVALID_ADDR;
    }

    if ((size % m_cb.channels_activated_count != 0) ||
        (size >= (1 << SAADC_EASYDMA_MAXCNT_SIZE))  ||
        (!size))
    {
        return NRFX_ERROR_INVALID_LENGTH;
    }

    switch (m_cb.saadc_state)
    {
        case NRF_SAADC_STATE_SIMPLE_MODE:
            if (m_cb.channels_activated_count != size)
            {
                return NRFX_ERROR_INVALID_LENGTH;
            }
            m_cb.size_primary     = size;
            m_cb.p_buffer_primary = p_buffer;
            break;

        case NRF_SAADC_STATE_ADV_MODE_SAMPLE_STARTED:
            nrf_saadc_buffer_init(NRF_SAADC, p_buffer, size);
            /* fall-through */

        case NRF_SAADC_STATE_ADV_MODE:
            /* fall-through */

        case NRF_SAADC_STATE_ADV_MODE_SAMPLE:
            if (m_cb.p_buffer_primary)
            {
                m_cb.size_secondary     = size;
                m_cb.p_buffer_secondary = p_buffer;
            }
            else
            {
                m_cb.size_primary     = size;
                m_cb.p_buffer_primary = p_buffer;
            }
            break;

        default:
            return NRFX_ERROR_INVALID_STATE;
    }

    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_saadc_mode_trigger(void)
{
    NRFX_ASSERT(m_cb.saadc_state != NRF_SAADC_STATE_UNINITIALIZED);
    NRFX_ASSERT(m_cb.saadc_state != NRF_SAADC_STATE_IDLE);

    if (!m_cb.p_buffer_primary)
    {
        return NRFX_ERROR_NO_MEM;
    }

    nrfx_err_t result = NRFX_SUCCESS;
    switch (m_cb.saadc_state)
    {
        case NRF_SAADC_STATE_SIMPLE_MODE:
            nrf_saadc_enable(NRF_SAADC);
            // When in simple blocking or non-blocking mode, buffer size is equal to activated channel count.
            // Single SAMPLE task is enough to obtain one sample on each activated channel.
            // This will result in buffer being filled with samples and therefore END event will appear.
            nrf_saadc_buffer_init(NRF_SAADC, m_cb.p_buffer_primary, m_cb.size_primary);
            if (m_cb.event_handler)
            {
                m_cb.saadc_state = NRF_SAADC_STATE_SIMPLE_MODE_SAMPLE;
                nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_START);
            }
            else
            {
                nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_START);
                while (!nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_STARTED))
                {}
                nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_STARTED);

                nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_SAMPLE);
                while (!nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_END))
                {}
                nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_END);
                nrf_saadc_disable(NRF_SAADC);
            }
            break;

        case NRF_SAADC_STATE_ADV_MODE:
            nrf_saadc_enable(NRF_SAADC);
            if (m_cb.event_handler)
            {
                // When in advanced non-blocking mode, latch whole buffer in EasyDMA.
                // END event will arrive when whole buffer is filled with samples.
                m_cb.saadc_state = NRF_SAADC_STATE_ADV_MODE_SAMPLE;
                nrf_saadc_buffer_init(NRF_SAADC, m_cb.p_buffer_primary, m_cb.size_primary);
                nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_START);
                break;
            }

            // When in advanced blocking mode, latch single chunk of buffer in EasyDMA.
            // Each chunk consists of single sample from each activated channels.
            // END event will arrive when single chunk is filled with samples.
            nrf_saadc_buffer_init(NRF_SAADC,
                                  &m_cb.p_buffer_primary[m_cb.samples_converted],
                                  m_cb.channels_activated_count);

            nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_START);
            while (!nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_STARTED))
            {}
            nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_STARTED);

            if (m_cb.oversampling_without_burst)
            {
                // Oversampling without burst is possible only on single channel.
                // In this configuration more than one SAMPLE task is needed to obtain single sample.
                uint32_t samples_to_take =
                    nrf_saadc_oversample_sample_count_get(nrf_saadc_oversample_get(NRF_SAADC));

                for (uint32_t sample_idx = 0; sample_idx < samples_to_take; sample_idx++)
                {
                    nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_DONE);
                    nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_SAMPLE);
                    while (!nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_DONE))
                    {}
                }
            }
            else
            {
                // Single SAMPLE task is enough to obtain one sample on each activated channel.
                // This will result in chunk being filled with samples and therefore END event will appear.
                nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_SAMPLE);
            }
            while (!nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_END))
            {}
            nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_END);

            m_cb.samples_converted += m_cb.channels_activated_count;
            if (m_cb.samples_converted < m_cb.size_primary)
            {
                result = NRFX_ERROR_BUSY;
            }
            else
            {
                m_cb.samples_converted  = 0;
                m_cb.p_buffer_primary   = m_cb.p_buffer_secondary;
                m_cb.size_primary       = m_cb.size_secondary;
                m_cb.p_buffer_secondary = NULL;
            }
            nrf_saadc_disable(NRF_SAADC);
            break;

        default:
            result = NRFX_ERROR_INVALID_STATE;
            break;
    }

    return result;
}

void nrfx_saadc_abort(void)
{
    NRFX_ASSERT(m_cb.saadc_state != NRF_SAADC_STATE_UNINITIALIZED);

    if (!m_cb.event_handler)
    {
        m_cb.p_buffer_primary = NULL;
        m_cb.p_buffer_secondary = NULL;
        m_cb.samples_converted = 0;
    }
    else
    {
        nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_STOP);
        if (m_cb.saadc_state == NRF_SAADC_STATE_CALIBRATION)
        {
            // STOPPED event does not appear when the calibration is ongoing
            m_cb.saadc_state = NRF_SAADC_STATE_IDLE;
        }
    }
}

nrfx_err_t nrfx_saadc_limits_set(uint8_t channel, int16_t limit_low, int16_t limit_high)
{
    NRFX_ASSERT(m_cb.saadc_state != NRF_SAADC_STATE_UNINITIALIZED);
    NRFX_ASSERT(limit_high >= limit_low);

    if (!m_cb.event_handler)
    {
        return NRFX_ERROR_FORBIDDEN;
    }

    if ((m_cb.saadc_state == NRF_SAADC_STATE_IDLE) ||
        (m_cb.saadc_state == NRF_SAADC_STATE_CALIBRATION))
    {
        return NRFX_ERROR_INVALID_STATE;
    }

    if (!(m_cb.channels_activated & (1 << channel)))
    {
        return NRFX_ERROR_INVALID_PARAM;
    }

    nrf_saadc_channel_limits_set(NRF_SAADC, channel, limit_low, limit_high);

    uint32_t int_mask = nrf_saadc_limit_int_get(channel, NRF_SAADC_LIMIT_LOW);
    if (limit_low == INT16_MIN)
    {
        m_cb.limits_low_activated &= ~(1 << channel);
        nrf_saadc_int_disable(NRF_SAADC, int_mask);
    }
    else
    {
        m_cb.limits_low_activated |= (1 << channel);
        nrf_saadc_int_enable(NRF_SAADC, int_mask);
    }

    int_mask = nrf_saadc_limit_int_get(channel, NRF_SAADC_LIMIT_HIGH);
    if (limit_high == INT16_MAX)
    {
        m_cb.limits_high_activated &= ~(1 << channel);
        nrf_saadc_int_disable(NRF_SAADC, int_mask);
    }
    else
    {
        m_cb.limits_high_activated |= (1 << channel);
        nrf_saadc_int_enable(NRF_SAADC, int_mask);
    }

    return NRFX_SUCCESS;
}

nrfx_err_t nrfx_saadc_offset_calibrate(nrfx_saadc_event_handler_t event_handler)
{
    NRFX_ASSERT(m_cb.saadc_state != NRF_SAADC_STATE_UNINITIALIZED);

    if (saadc_busy_check())
    {
        return NRFX_ERROR_BUSY;
    }

    m_cb.saadc_state = NRF_SAADC_STATE_CALIBRATION;
    m_cb.event_handler = event_handler;

    nrf_saadc_enable(NRF_SAADC);
#if NRFX_CHECK(INTERCEPT_SAADC_CALIBRATION_SAMPLES)
    nrf_saadc_buffer_init(NRF_SAADC, m_cb.calib_samples, NRFX_ARRAY_SIZE(m_cb.calib_samples));
    if (event_handler)
    {
        nrf_saadc_int_set(NRF_SAADC, NRF_SAADC_INT_STARTED | NRF_SAADC_INT_CALIBRATEDONE);
        nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_START);
    }
    else
    {
        nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_START);
        while (!nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_STARTED))
        {}
        nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_STARTED);

        nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_CALIBRATEOFFSET);
        while (!nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_CALIBRATEDONE))
        {}
        nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_CALIBRATEDONE);
        nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_END);

        nrf_saadc_disable(NRF_SAADC);
        m_cb.saadc_state = NRF_SAADC_STATE_IDLE;
    }
#else
    nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_CALIBRATEOFFSET);
    if (event_handler)
    {
        nrf_saadc_int_enable(NRF_SAADC, NRF_SAADC_INT_CALIBRATEDONE);
    }
    else
    {
        while (!nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_CALIBRATEDONE))
        {}
        nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_CALIBRATEDONE);
        nrf_saadc_disable(NRF_SAADC);
        m_cb.saadc_state = NRF_SAADC_STATE_IDLE;
    }
#endif // NRFX_CHECK(INTERCEPT_SAADC_CALIBRATION_SAMPLES)

    return NRFX_SUCCESS;
}

static void saadc_event_started_handle(void)
{
    nrfx_saadc_evt_t evt_data;

    switch (m_cb.saadc_state)
    {
        case NRF_SAADC_STATE_ADV_MODE_SAMPLE:
            evt_data.type = NRFX_SAADC_EVT_READY;
            m_cb.event_handler(&evt_data);

            if (nrf_saadc_continuous_mode_enable_check(NRF_SAADC))
            {
                // Trigger internal timer
                nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_SAMPLE);
            }

            m_cb.saadc_state = NRF_SAADC_STATE_ADV_MODE_SAMPLE_STARTED;
            if (m_cb.p_buffer_secondary)
            {
                nrf_saadc_buffer_init(NRF_SAADC,
                                      m_cb.p_buffer_secondary,
                                      m_cb.size_secondary);
            }
            /* fall-through */

        case NRF_SAADC_STATE_ADV_MODE_SAMPLE_STARTED:
            if (!m_cb.p_buffer_secondary)
            {
                // Send next buffer request only if it was not provided earlier,
                // before conversion start or outside of user's callback context.
                evt_data.type = NRFX_SAADC_EVT_BUF_REQ;
                m_cb.event_handler(&evt_data);
            }
            break;

        case NRF_SAADC_STATE_SIMPLE_MODE_SAMPLE:
            nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_SAMPLE);
            break;

#if NRFX_CHECK(INTERCEPT_SAADC_CALIBRATION_SAMPLES)
        case NRF_SAADC_STATE_CALIBRATION:
            nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_CALIBRATEOFFSET);
            break;
#endif

        default:
            break;
    }
}

static void saadc_event_end_handle(void)
{
    nrfx_saadc_evt_t evt_data;
    evt_data.type = NRFX_SAADC_EVT_DONE;
    evt_data.data.done.p_buffer = m_cb.p_buffer_primary;
    evt_data.data.done.size = m_cb.size_primary;
    m_cb.event_handler(&evt_data);

    switch (m_cb.saadc_state)
    {
        case NRF_SAADC_STATE_SIMPLE_MODE_SAMPLE:
            nrf_saadc_disable(NRF_SAADC);
            m_cb.saadc_state = NRF_SAADC_STATE_SIMPLE_MODE;
            break;

        case NRF_SAADC_STATE_ADV_MODE_SAMPLE_STARTED:
            m_cb.p_buffer_primary = m_cb.p_buffer_secondary;
            m_cb.size_primary     = m_cb.size_secondary;
            m_cb.p_buffer_secondary = NULL;
            if (m_cb.p_buffer_primary)
            {
                if (m_cb.start_on_end)
                {
                    nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_START);
                }
            }
            else
            {
                nrf_saadc_disable(NRF_SAADC);
                m_cb.saadc_state = NRF_SAADC_STATE_ADV_MODE;
                evt_data.type = NRFX_SAADC_EVT_FINISHED;
                m_cb.event_handler(&evt_data);
            }
            break;

        default:
            break;
    }
}

static void saadc_event_limits_handle(uint8_t limits_activated, nrf_saadc_limit_t limit_type)
{
    while (limits_activated)
    {
        uint8_t channel = __CLZ(__RBIT((uint32_t)limits_activated));
        limits_activated &= ~(1 << channel);

        nrf_saadc_event_t event = nrf_saadc_limit_event_get(channel, limit_type);
        if (nrf_saadc_event_check(NRF_SAADC, event))
        {
            nrf_saadc_event_clear(NRF_SAADC, event);

            nrfx_saadc_evt_t evt_data;
            evt_data.type = NRFX_SAADC_EVT_LIMIT;
            evt_data.data.limit.channel = channel;
            evt_data.data.limit.limit_type = limit_type;
            m_cb.event_handler(&evt_data);
        }
    }
}

void nrfx_saadc_irq_handler(void)
{
    if (nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_STARTED))
    {
        nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_STARTED);
        saadc_event_started_handle();
    }

    if (nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_STOPPED))
    {
        nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_STOPPED);

        // If there was ongoing conversion the STOP task also triggers the END event
        m_cb.size_primary = nrf_saadc_amount_get(NRF_SAADC);
        m_cb.p_buffer_secondary = NULL;
        /* fall-through to the END event handler */
    }

    if (nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_END))
    {
        nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_END);

#if NRFX_CHECK(INTERCEPT_SAADC_CALIBRATION_SAMPLES)
        // When samples are intercepted into scratch buffer during calibration,
        // END event appears when the calibration finishes. This event should be ignored.
        if (m_cb.saadc_state != NRF_SAADC_STATE_CALIBRATION)
#endif
        {
            saadc_event_end_handle();
        }
    }

    saadc_event_limits_handle(m_cb.limits_low_activated,  NRF_SAADC_LIMIT_LOW);
    saadc_event_limits_handle(m_cb.limits_high_activated, NRF_SAADC_LIMIT_HIGH);

    if (nrf_saadc_event_check(NRF_SAADC, NRF_SAADC_EVENT_CALIBRATEDONE))
    {
        nrf_saadc_event_clear(NRF_SAADC, NRF_SAADC_EVENT_CALIBRATEDONE);
        nrf_saadc_disable(NRF_SAADC);

        m_cb.saadc_state = NRF_SAADC_STATE_IDLE;

        nrfx_saadc_evt_t evt_data;
        evt_data.type = NRFX_SAADC_EVT_CALIBRATEDONE;
        m_cb.event_handler(&evt_data);

    }
}
#endif // NRFX_CHECK(NRFX_SAADC_ENABLED)
