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

#include <nrfx.h>

#if NRFX_CHECK(NRFX_USBREG_ENABLED)

#include <nrfx_usbreg.h>

static nrfx_usbreg_event_handler_t m_usbevt_handler;

nrfx_usbreg_event_handler_t nrfx_usbreg_handler_get(void)
{
    return m_usbevt_handler;
}

void nrfx_usbreg_init(nrfx_usbreg_config_t const * p_config)
{
    NRFX_ASSERT(p_config != NULL);
    NRFX_ASSERT(p_config->handler != NULL);

    nrfx_usbreg_uninit();
    m_usbevt_handler = p_config->handler;

    NRFX_IRQ_PRIORITY_SET(nrfx_get_irq_number(NRF_USBREGULATOR), p_config->irq_priority);
    NRFX_IRQ_ENABLE(nrfx_get_irq_number(NRF_USBREGULATOR));
}

void nrfx_usbreg_enable(void)
{
    nrf_usbreg_int_enable(NRF_USBREGULATOR, NRF_USBREG_INT_USBDETECTED |
                                            NRF_USBREG_INT_USBREMOVED  |
                                            NRF_USBREG_INT_USBPWRRDY);
}

void nrfx_usbreg_disable(void)
{
    nrf_usbreg_int_disable(NRF_USBREGULATOR, NRF_USBREG_INT_USBDETECTED |
                                             NRF_USBREG_INT_USBREMOVED  |
                                             NRF_USBREG_INT_USBPWRRDY);
}

void nrfx_usbreg_uninit(void)
{
    nrfx_usbreg_disable();
    NRFX_IRQ_DISABLE(nrfx_get_irq_number(NRF_USBREGULATOR));
    m_usbevt_handler = NULL;
}

void nrfx_usbreg_irq_handler(void)
{
    if (nrf_usbreg_event_check(NRF_USBREGULATOR, NRF_USBREG_EVENT_USBDETECTED))
    {
        nrf_usbreg_event_clear(NRF_USBREGULATOR, NRF_USBREG_EVENT_USBDETECTED);
        m_usbevt_handler(NRFX_USBREG_EVT_DETECTED);
    }
    if (nrf_usbreg_event_check(NRF_USBREGULATOR, NRF_USBREG_EVENT_USBREMOVED))
    {
        nrf_usbreg_event_clear(NRF_USBREGULATOR, NRF_USBREG_EVENT_USBREMOVED);
        m_usbevt_handler(NRFX_USBREG_EVT_REMOVED);
    }
    if (nrf_usbreg_event_check(NRF_USBREGULATOR, NRF_USBREG_EVENT_USBPWRRDY))
    {
        nrf_usbreg_event_clear(NRF_USBREGULATOR, NRF_USBREG_EVENT_USBPWRRDY);
        m_usbevt_handler(NRFX_USBREG_EVT_READY);
    }
}

#endif // NRFX_CHECK(NRFX_USBREG_ENABLED)
