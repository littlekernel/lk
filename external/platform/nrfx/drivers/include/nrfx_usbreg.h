/*
 * Copyright (c) 2019 - 2020, Nordic Semiconductor ASA
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

#ifndef NRFX_USBREG_H__
#define NRFX_USBREG_H__

#include <nrfx.h>
#include <hal/nrf_usbreg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_usbreg USBREG driver
 * @{
 * @ingroup nrf_usbd
 * @ingroup nrf_power
 * @brief   USB regulators (USBREG) peripheral driver.
 */

/** @brief Events from USB power system */
typedef enum {
    NRFX_USBREG_EVT_DETECTED, /**< USB power detected on the connector (plugged in). */
    NRFX_USBREG_EVT_REMOVED,  /**< USB power removed from the connector. */
    NRFX_USBREG_EVT_READY     /**< USB power regulator ready. */
} nrfx_usbreg_evt_t;

/**
 * @brief USB power state
 *
 * The single enumerator that holds all data about current state of USB
 * related POWER.
 *
 * Organized this way that higher power state has higher numeric value
 */
typedef enum {
    NRFX_USBREG_STATE_DISCONNECTED, /**< No power on USB lines detected. */
    NRFX_USBREG_STATE_CONNECTED,    /**< The USB power is detected, but USB power regulator is not ready. */
    NRFX_USBREG_STATE_READY         /**< From the power viewpoint, USB is ready for working. */
} nrfx_usbreg_state_t;

/**
 * @brief Event handler for the USB-related power events.
 *
 * @param event Event type
 */
typedef void (*nrfx_usbreg_event_handler_t)(nrfx_usbreg_evt_t event);

/**
 * @brief The configuration of the USB-related power events.
 *
 * Configuration used to enable and configure USB power event handling.
 */
typedef struct
{
    nrfx_usbreg_event_handler_t handler;      //!< Event processing.
    uint8_t                     irq_priority; //!< Priority of the USBREG interrupt.
} nrfx_usbreg_config_t;

/**
 * @brief Function for getting the USBREG handler.
 *
 * @return Handler of the USB power.
 */
nrfx_usbreg_event_handler_t nrfx_usbreg_handler_get(void);

/**
 * @brief Function for initializing the processing of USBREG events.
 *
 * Configures and sets up the USB power event processing.
 *
 * @param[in] p_config Configuration structure. Must not be NULL.
 *
 * @sa nrfx_usbreg_uninit
 */
void nrfx_usbreg_init(nrfx_usbreg_config_t const *p_config);

/** @brief Function for enabling the processing of USBREG events. */
void nrfx_usbreg_enable(void);

/** @brief Function for disabling the processing of USBREG events. */
void nrfx_usbreg_disable(void);

/**
 * @brief Function for uninitalizing the processing of USBREG events.
 *
 * @sa nrfx_usbreg_init
 */
void nrfx_usbreg_uninit(void);

/**
 * @brief Function for getting the status of USBREG.
 *
 * @return Current USB power status.
 */
NRFX_STATIC_INLINE nrfx_usbreg_state_t nrfx_usbreg_usbstatus_get(void);

#ifndef NRFX_DECLARE_ONLY
NRFX_STATIC_INLINE nrfx_usbreg_state_t nrfx_usbreg_usbstatus_get(void)
{
    uint32_t status = nrf_usbreg_status_get(NRF_USBREGULATOR);
    if (0 == (status & NRF_USBREG_STATUS_VBUSDETECT_MASK))
    {
        return NRFX_USBREG_STATE_DISCONNECTED;
    }
    if (0 == (status & NRF_USBREG_STATUS_OUTPUTRDY_MASK))
    {
        return NRFX_USBREG_STATE_CONNECTED;
    }
    return NRFX_USBREG_STATE_READY;
}
#endif // NRFX_DECLARE_ONLY

/** @} */

void nrfx_usbreg_irq_handler(void);

#ifdef __cplusplus
}
#endif

#endif // NRFX_USBREG_H__
