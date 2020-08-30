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

#if NRFX_CHECK(NRFX_I2S_ENABLED)

#include <nrfx_i2s.h>
#include <hal/nrf_gpio.h>

#define NRFX_LOG_MODULE I2S
#include <nrfx_log.h>

#define EVT_TO_STR(event)                                         \
    (event == NRF_I2S_EVENT_RXPTRUPD ? "NRF_I2S_EVENT_RXPTRUPD" : \
    (event == NRF_I2S_EVENT_TXPTRUPD ? "NRF_I2S_EVENT_TXPTRUPD" : \
    (event == NRF_I2S_EVENT_STOPPED  ? "NRF_I2S_EVENT_STOPPED"  : \
                                       "UNKNOWN EVENT")))

#if !defined(USE_WORKAROUND_FOR_I2S_STOP_ANOMALY) && \
    (defined(NRF52_SERIES) || defined(NRF9160_XXAA))
// Enable workaround for nRF52 Series anomaly 194 / nRF9160 anomaly 1
// (STOP task does not switch off all resources).
#define USE_WORKAROUND_FOR_I2S_STOP_ANOMALY 1
#endif

#if !defined(USE_WORKAROUND_FOR_ANOMALY_170) && defined(NRF52_SERIES)
// Enable workaround for nRF52 Series anomaly 170
// (when reading the value of PSEL registers, the CONNECT field might not
//  return the same value that has been written to it).
#define USE_WORKAROUND_FOR_ANOMALY_170 1
#endif

#if !defined(USE_WORKAROUND_FOR_ANOMALY_196) && defined(NRF52_SERIES)
// Enable workaround for nRF52 Series anomaly 196
// (PSEL acquires GPIO regardless of ENABLE).
#define USE_WORKAROUND_FOR_ANOMALY_196 1
#endif

// Control block - driver instance local data.
typedef struct
{
    nrfx_i2s_data_handler_t handler;
    nrfx_drv_state_t        state;

    bool use_rx         : 1;
    bool use_tx         : 1;
    bool rx_ready       : 1;
    bool tx_ready       : 1;
    bool buffers_needed : 1;
    bool buffers_reused : 1;

    uint16_t            buffer_size;
    nrfx_i2s_buffers_t  next_buffers;
    nrfx_i2s_buffers_t  current_buffers;
} i2s_control_block_t;
static i2s_control_block_t m_cb;


static void configure_pins(nrfx_i2s_config_t const * p_config)
{
    uint32_t mck_pin, sdout_pin, sdin_pin;

    // Configure pins used by the peripheral:

    // - SCK and LRCK (required) - depending on the mode of operation these
    //   pins are configured as outputs (in Master mode) or inputs (in Slave
    //   mode).
    if (p_config->mode == NRF_I2S_MODE_MASTER)
    {
        nrf_gpio_cfg_output(p_config->sck_pin);
        nrf_gpio_cfg_output(p_config->lrck_pin);
    }
    else
    {
        nrf_gpio_cfg_input(p_config->sck_pin,  NRF_GPIO_PIN_NOPULL);
        nrf_gpio_cfg_input(p_config->lrck_pin, NRF_GPIO_PIN_NOPULL);
    }

    // - MCK (optional) - always output,
    if (p_config->mck_pin != NRFX_I2S_PIN_NOT_USED)
    {
        mck_pin = p_config->mck_pin;
        nrf_gpio_cfg_output(mck_pin);
    }
    else
    {
        mck_pin = NRF_I2S_PIN_NOT_CONNECTED;
    }

    // - SDOUT (optional) - always output,
    if (p_config->sdout_pin != NRFX_I2S_PIN_NOT_USED)
    {
        sdout_pin = p_config->sdout_pin;
        nrf_gpio_cfg_output(sdout_pin);
    }
    else
    {
        sdout_pin = NRF_I2S_PIN_NOT_CONNECTED;
    }

    // - SDIN (optional) - always input.
    if (p_config->sdin_pin != NRFX_I2S_PIN_NOT_USED)
    {
        sdin_pin = p_config->sdin_pin;
        nrf_gpio_cfg_input(sdin_pin, NRF_GPIO_PIN_NOPULL);
    }
    else
    {
        sdin_pin = NRF_I2S_PIN_NOT_CONNECTED;
    }

    nrf_i2s_pins_set(NRF_I2S0,
                     p_config->sck_pin,
                     p_config->lrck_pin,
                     mck_pin,
                     sdout_pin,
                     sdin_pin);
}

static void deconfigure_pins(void)
{
    uint32_t sck_pin   = nrf_i2s_sck_pin_get(NRF_I2S0);
    uint32_t lrck_pin  = nrf_i2s_lrck_pin_get(NRF_I2S0);
    uint32_t mck_pin   = nrf_i2s_mck_pin_get(NRF_I2S0);
    uint32_t sdout_pin = nrf_i2s_sdout_pin_get(NRF_I2S0);
    uint32_t sdin_pin  = nrf_i2s_sdin_pin_get(NRF_I2S0);

#if USE_WORKAROUND_FOR_ANOMALY_170
    // Create bitmask for extracting pin number from PSEL register.
    uint32_t pin_mask = I2S_PSEL_SCK_PIN_Msk;
#if defined(I2S_PSEL_SCK_PORT_Msk)
    // If device supports more than one GPIO port, take port number into account as well.
    pin_mask |= I2S_PSEL_SCK_PORT_Msk;
#endif
#else
    uint32_t pin_mask = 0xFFFFFFFF;
#endif // USE_WORKAROUND_FOR_ANOMALY_170

    nrf_gpio_cfg_default(sck_pin & pin_mask);
    nrf_gpio_cfg_default(lrck_pin & pin_mask);

    if (mck_pin != NRF_I2S_PIN_NOT_CONNECTED)
    {
        nrf_gpio_cfg_default(mck_pin & pin_mask);
    }

    if (sdout_pin != NRF_I2S_PIN_NOT_CONNECTED)
    {
        nrf_gpio_cfg_default(sdout_pin & pin_mask);
    }

    if (sdin_pin != NRF_I2S_PIN_NOT_CONNECTED)
    {
        nrf_gpio_cfg_default(sdin_pin & pin_mask);
    }
}

nrfx_err_t nrfx_i2s_init(nrfx_i2s_config_t const * p_config,
                         nrfx_i2s_data_handler_t   handler)
{
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(handler);

    nrfx_err_t err_code;

    if (m_cb.state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (!nrf_i2s_configure(NRF_I2S0,
                           p_config->mode,
                           p_config->format,
                           p_config->alignment,
                           p_config->sample_width,
                           p_config->channels,
                           p_config->mck_setup,
                           p_config->ratio))
    {
        err_code = NRFX_ERROR_INVALID_PARAM;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

#if NRF_I2S_HAS_CLKCONFIG
    nrf_i2s_clk_configure(NRF_I2S0, p_config->clksrc, p_config->enable_bypass);
#endif
    configure_pins(p_config);

    m_cb.handler = handler;

    NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(NRF_I2S0), p_config->irq_priority);
    NRFX_IRQ_ENABLE(nrfx_get_irq_number(NRF_I2S0));

    m_cb.state = NRFX_DRV_STATE_INITIALIZED;

    NRFX_LOG_INFO("Initialized.");
    return NRFX_SUCCESS;
}


void nrfx_i2s_uninit(void)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    nrfx_i2s_stop();

    NRFX_IRQ_DISABLE(nrfx_get_irq_number(NRF_I2S0));

    nrf_i2s_disable(NRF_I2S0);

    deconfigure_pins();

#if USE_WORKAROUND_FOR_ANOMALY_196
    // Disabling I2S is insufficient to release pins acquired by the peripheral.
    // Explicit disconnect is needed.
    nrf_i2s_pins_set(NRF_I2S0,
                     NRF_I2S_PIN_NOT_CONNECTED,
                     NRF_I2S_PIN_NOT_CONNECTED,
                     NRF_I2S_PIN_NOT_CONNECTED,
                     NRF_I2S_PIN_NOT_CONNECTED,
                     NRF_I2S_PIN_NOT_CONNECTED);
#endif

    m_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
    NRFX_LOG_INFO("Uninitialized.");
}


nrfx_err_t nrfx_i2s_start(nrfx_i2s_buffers_t const * p_initial_buffers,
                          uint16_t                   buffer_size,
                          uint8_t                    flags)
{
    NRFX_ASSERT(p_initial_buffers != NULL);
    NRFX_ASSERT(p_initial_buffers->p_rx_buffer != NULL ||
                p_initial_buffers->p_tx_buffer != NULL);
    NRFX_ASSERT((p_initial_buffers->p_rx_buffer == NULL) ||
                (nrfx_is_in_ram(p_initial_buffers->p_rx_buffer) &&
                 nrfx_is_word_aligned(p_initial_buffers->p_rx_buffer)));
    NRFX_ASSERT((p_initial_buffers->p_tx_buffer == NULL) ||
                (nrfx_is_in_ram(p_initial_buffers->p_tx_buffer) &&
                 nrfx_is_word_aligned(p_initial_buffers->p_tx_buffer)));
    NRFX_ASSERT(buffer_size != 0);
    (void)(flags);

    nrfx_err_t err_code;

    if (m_cb.state != NRFX_DRV_STATE_INITIALIZED)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (((p_initial_buffers->p_rx_buffer != NULL)
         && !nrfx_is_in_ram(p_initial_buffers->p_rx_buffer))
        ||
        ((p_initial_buffers->p_tx_buffer != NULL)
         && !nrfx_is_in_ram(p_initial_buffers->p_tx_buffer)))
    {
        err_code = NRFX_ERROR_INVALID_ADDR;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    m_cb.use_rx         = (p_initial_buffers->p_rx_buffer != NULL);
    m_cb.use_tx         = (p_initial_buffers->p_tx_buffer != NULL);
    m_cb.rx_ready       = false;
    m_cb.tx_ready       = false;
    m_cb.buffers_needed = false;
    m_cb.buffer_size    = buffer_size;

    // Set the provided initial buffers as next, they will become the current
    // ones after the IRQ handler is called for the first time, what will occur
    // right after the START task is triggered.
    m_cb.next_buffers = *p_initial_buffers;
    m_cb.current_buffers.p_rx_buffer = NULL;
    m_cb.current_buffers.p_tx_buffer = NULL;

    nrf_i2s_transfer_set(NRF_I2S0,
                         m_cb.buffer_size,
                         m_cb.next_buffers.p_rx_buffer,
                         m_cb.next_buffers.p_tx_buffer);

    nrf_i2s_enable(NRF_I2S0);

    m_cb.state = NRFX_DRV_STATE_POWERED_ON;

    nrf_i2s_event_clear(NRF_I2S0, NRF_I2S_EVENT_RXPTRUPD);
    nrf_i2s_event_clear(NRF_I2S0, NRF_I2S_EVENT_TXPTRUPD);
    nrf_i2s_event_clear(NRF_I2S0, NRF_I2S_EVENT_STOPPED);
    nrf_i2s_int_enable(NRF_I2S0, (m_cb.use_rx ? NRF_I2S_INT_RXPTRUPD_MASK : 0) |
                                 (m_cb.use_tx ? NRF_I2S_INT_TXPTRUPD_MASK : 0) |
                                 NRF_I2S_INT_STOPPED_MASK);
    nrf_i2s_task_trigger(NRF_I2S0, NRF_I2S_TASK_START);

    NRFX_LOG_INFO("Started.");
    return NRFX_SUCCESS;
}


nrfx_err_t nrfx_i2s_next_buffers_set(nrfx_i2s_buffers_t const * p_buffers)
{
    NRFX_ASSERT(m_cb.state == NRFX_DRV_STATE_POWERED_ON);
    NRFX_ASSERT(p_buffers);
    NRFX_ASSERT((p_buffers->p_rx_buffer == NULL) ||
                (nrfx_is_in_ram(p_buffers->p_rx_buffer) &&
                 nrfx_is_word_aligned(p_buffers->p_rx_buffer)));
    NRFX_ASSERT((p_buffers->p_tx_buffer == NULL) ||
                (nrfx_is_in_ram(p_buffers->p_tx_buffer) &&
                 nrfx_is_word_aligned(p_buffers->p_tx_buffer)));

    nrfx_err_t err_code;

    if (!m_cb.buffers_needed)
    {
        err_code = NRFX_ERROR_INVALID_STATE;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (((p_buffers->p_rx_buffer != NULL)
         && !nrfx_is_in_ram(p_buffers->p_rx_buffer))
        ||
        ((p_buffers->p_tx_buffer != NULL)
         && !nrfx_is_in_ram(p_buffers->p_tx_buffer)))
    {
        err_code = NRFX_ERROR_INVALID_ADDR;
        NRFX_LOG_WARNING("Function: %s, error code: %s.",
                         __func__,
                         NRFX_LOG_ERROR_STRING_GET(err_code));
        return err_code;
    }

    if (m_cb.use_tx)
    {
        NRFX_ASSERT(p_buffers->p_tx_buffer != NULL);
        nrf_i2s_tx_buffer_set(NRF_I2S0, p_buffers->p_tx_buffer);
    }
    if (m_cb.use_rx)
    {
        NRFX_ASSERT(p_buffers->p_rx_buffer != NULL);
        nrf_i2s_rx_buffer_set(NRF_I2S0, p_buffers->p_rx_buffer);
    }

    m_cb.next_buffers   = *p_buffers;
    m_cb.buffers_needed = false;

    return NRFX_SUCCESS;
}


void nrfx_i2s_stop(void)
{
    NRFX_ASSERT(m_cb.state != NRFX_DRV_STATE_UNINITIALIZED);

    m_cb.buffers_needed = false;

    // First disable interrupts, then trigger the STOP task, so no spurious
    // RXPTRUPD and TXPTRUPD events (see nRF52 anomaly 55) are processed.
    nrf_i2s_int_disable(NRF_I2S0, NRF_I2S_INT_RXPTRUPD_MASK |
                                  NRF_I2S_INT_TXPTRUPD_MASK);
    nrf_i2s_task_trigger(NRF_I2S0, NRF_I2S_TASK_STOP);

#if NRFX_CHECK(USE_WORKAROUND_FOR_I2S_STOP_ANOMALY)
    *((volatile uint32_t *)(((uint32_t)NRF_I2S0) + 0x38)) = 1;
    *((volatile uint32_t *)(((uint32_t)NRF_I2S0) + 0x3C)) = 1;
#endif
}


void nrfx_i2s_irq_handler(void)
{
    if (nrf_i2s_event_check(NRF_I2S0, NRF_I2S_EVENT_TXPTRUPD))
    {
        nrf_i2s_event_clear(NRF_I2S0, NRF_I2S_EVENT_TXPTRUPD);
        m_cb.tx_ready = true;
        if (m_cb.use_tx && m_cb.buffers_needed)
        {
            m_cb.buffers_reused = true;
        }
    }
    if (nrf_i2s_event_check(NRF_I2S0, NRF_I2S_EVENT_RXPTRUPD))
    {
        nrf_i2s_event_clear(NRF_I2S0, NRF_I2S_EVENT_RXPTRUPD);
        m_cb.rx_ready = true;
        if (m_cb.use_rx && m_cb.buffers_needed)
        {
            m_cb.buffers_reused = true;
        }
    }

    if (nrf_i2s_event_check(NRF_I2S0, NRF_I2S_EVENT_STOPPED))
    {
        nrf_i2s_event_clear(NRF_I2S0, NRF_I2S_EVENT_STOPPED);
        nrf_i2s_int_disable(NRF_I2S0, NRF_I2S_INT_STOPPED_MASK);
        nrf_i2s_disable(NRF_I2S0);

        // When stopped, release all buffers, including these scheduled for
        // the next part of the transfer, and signal that the transfer has
        // finished.

        m_cb.handler(&m_cb.current_buffers, 0);

        // Change the state of the driver before calling the handler with
        // the flag signaling that the transfer has finished, so that it is
        // possible to start a new transfer directly from the handler function.
        m_cb.state = NRFX_DRV_STATE_INITIALIZED;
        NRFX_LOG_INFO("Stopped.");

        m_cb.handler(&m_cb.next_buffers, NRFX_I2S_STATUS_TRANSFER_STOPPED);
    }
    else
    {
        // Check if the requested transfer has been completed:
        // - full-duplex mode
        if ((m_cb.use_tx && m_cb.use_rx && m_cb.tx_ready && m_cb.rx_ready) ||
            // - TX only mode
            (!m_cb.use_rx && m_cb.tx_ready) ||
            // - RX only mode
            (!m_cb.use_tx && m_cb.rx_ready))
        {
            m_cb.tx_ready = false;
            m_cb.rx_ready = false;

            // If the application did not supply the buffers for the next
            // part of the transfer until this moment, the current buffers
            // cannot be released, since the I2S peripheral already started
            // using them. Signal this situation to the application by
            // passing NULL instead of the structure with released buffers.
            if (m_cb.buffers_reused)
            {
                m_cb.buffers_reused = false;
                // This will most likely be set at this point. However, there is
                // a small time window between TXPTRUPD and RXPTRUPD events,
                // and it is theoretically possible that next buffers will be
                // set in this window, so to be sure this flag is set to true,
                // set it explicitly.
                m_cb.buffers_needed = true;
                m_cb.handler(NULL,
                             NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
            }
            else
            {
                // Buffers that have been used by the I2S peripheral (current)
                // are now released and will be returned to the application,
                // and the ones scheduled to be used as next become the current
                // ones.
                nrfx_i2s_buffers_t released_buffers = m_cb.current_buffers;
                m_cb.current_buffers = m_cb.next_buffers;
                m_cb.next_buffers.p_rx_buffer = NULL;
                m_cb.next_buffers.p_tx_buffer = NULL;
                m_cb.buffers_needed = true;
                m_cb.handler(&released_buffers,
                             NRFX_I2S_STATUS_NEXT_BUFFERS_NEEDED);
            }

        }
    }
}

#endif // NRFX_CHECK(NRFX_I2S_ENABLED)
