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

#if NRFX_CHECK(NRFX_IPC_ENABLED)

#include <nrfx_ipc.h>

// Control block - driver instance local data.
typedef struct
{
    nrfx_ipc_handler_t handler;
    nrfx_drv_state_t   state;
    void *             p_context;
} ipc_control_block_t;

static ipc_control_block_t m_ipc_cb;

nrfx_err_t nrfx_ipc_init(uint8_t irq_priority, nrfx_ipc_handler_t handler, void * p_context)
{
    NRFX_ASSERT(handler);
    if (m_ipc_cb.state != NRFX_DRV_STATE_UNINITIALIZED)
    {
        return NRFX_ERROR_ALREADY_INITIALIZED;
    }

    NRFX_IRQ_PRIORITY_SET(IPC_IRQn, irq_priority);
    NRFX_IRQ_ENABLE(IPC_IRQn);

    m_ipc_cb.state = NRFX_DRV_STATE_INITIALIZED;
    m_ipc_cb.handler = handler;
    m_ipc_cb.p_context = p_context;

    return NRFX_SUCCESS;
}

void nrfx_ipc_config_load(const nrfx_ipc_config_t * p_config)
{
    NRFX_ASSERT(p_config);
    NRFX_ASSERT(m_ipc_cb.state == NRFX_DRV_STATE_INITIALIZED);

    uint32_t i;
    for (i = 0; i < IPC_CONF_NUM; ++i)
    {
        nrf_ipc_send_config_set(NRF_IPC, i, p_config->send_task_config[i]);
    }

    for (i = 0; i < IPC_CONF_NUM; ++i)
    {
        nrf_ipc_receive_config_set(NRF_IPC, i, p_config->receive_event_config[i]);
    }

    nrf_ipc_int_enable(NRF_IPC, p_config->receive_events_enabled);
}

void nrfx_ipc_uninit(void)
{
    NRFX_ASSERT(m_ipc_cb.state == NRFX_DRV_STATE_INITIALIZED);

    uint32_t i;
    for (i = 0; i < IPC_CONF_NUM; ++i)
    {
        nrf_ipc_send_config_set(NRF_IPC, i, 0);
    }

    for (i = 0; i < IPC_CONF_NUM; ++i)
    {
        nrf_ipc_receive_config_set(NRF_IPC, i, 0);
    }

    nrf_ipc_int_disable(NRF_IPC, 0xFFFFFFFF);
    m_ipc_cb.state = NRFX_DRV_STATE_UNINITIALIZED;
}

void nrfx_ipc_receive_event_enable(uint8_t event_index)
{
    NRFX_ASSERT(m_ipc_cb.state == NRFX_DRV_STATE_INITIALIZED);
    nrf_ipc_int_enable(NRF_IPC, (1UL << event_index));
}

void nrfx_ipc_receive_event_disable(uint8_t event_index)
{
    NRFX_ASSERT(m_ipc_cb.state == NRFX_DRV_STATE_INITIALIZED);
    nrf_ipc_int_disable(NRF_IPC, (1UL << event_index));
}

void nrfx_ipc_receive_event_group_enable(uint32_t event_bitmask)
{
    NRFX_ASSERT(m_ipc_cb.state == NRFX_DRV_STATE_INITIALIZED);
    nrf_ipc_int_enable(NRF_IPC, event_bitmask);
}

void nrfx_ipc_receive_event_group_disable(uint32_t event_bitmask)
{
    NRFX_ASSERT(m_ipc_cb.state == NRFX_DRV_STATE_INITIALIZED);
    nrf_ipc_int_disable(NRF_IPC, event_bitmask);
}

void nrfx_ipc_receive_event_channel_assign(uint8_t event_index, uint8_t channel_index)
{
    NRFX_ASSERT(channel_index < IPC_CH_NUM);
    uint32_t channel_bitmask = (1UL << channel_index);
    channel_bitmask |= nrf_ipc_receive_config_get(NRF_IPC, event_index);
    nrf_ipc_receive_config_set(NRF_IPC, event_index, channel_bitmask);
}

void nrfx_ipc_send_task_channel_assign(uint8_t send_index, uint8_t channel_index)
{
    NRFX_ASSERT(channel_index < IPC_CH_NUM);
    uint32_t channel_bitmask = (1UL << channel_index);
    channel_bitmask |= nrf_ipc_send_config_get(NRF_IPC, send_index);
    nrf_ipc_send_config_set(NRF_IPC, send_index, channel_bitmask);
}

void nrfx_ipc_irq_handler(void)
{
    // Get the information about events that fire this interrupt
    uint32_t events_map = nrf_ipc_int_pending_get(NRF_IPC);

    // Clear these events
    uint32_t bitmask = events_map;
    while (bitmask)
    {
        uint8_t event_idx = __CLZ(__RBIT(bitmask));
        bitmask &= ~(1UL << event_idx);
        nrf_ipc_event_clear(NRF_IPC, nrf_ipc_receive_event_get(event_idx));
    }

    // Execute interrupt handler to provide information about events to app
    m_ipc_cb.handler(events_map, m_ipc_cb.p_context);
}

#endif // NRFX_CHECK(NRFX_IPC_ENABLED)
