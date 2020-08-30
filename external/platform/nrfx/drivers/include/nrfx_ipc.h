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
#ifndef NRFX_IPC_H__
#define NRFX_IPC_H__

#include <nrfx.h>
#include <hal/nrf_ipc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_ipc IPC driver
 * @{
 * @ingroup nrf_ipc
 * @brief   Interprocessor Communication (IPC) peripheral driver.
 */

/**
 * @brief IPC driver handler type.
 *
 * @param[in] event_mask Bitmask with events that triggered the interrupt.
 * @param[in] p_context  Context passed to the interrupt handler, set on initialization.
 */
typedef void (*nrfx_ipc_handler_t)(uint32_t event_mask, void * p_context);

/** @brief IPC configuration structure. */
typedef struct
{
    uint32_t send_task_config[IPC_CONF_NUM];     ///< Configuration of the connection between signals and IPC channels.
    uint32_t receive_event_config[IPC_CONF_NUM]; ///< Configuration of the connection between events and IPC channels.
    uint32_t receive_events_enabled;             ///< Bitmask with events to be enabled to generate interrupt.
} nrfx_ipc_config_t;

/**
 * @brief Function for initializing the IPC driver.
 *
 * @param irq_priority Interrupt priority.
 * @param handler      Event handler provided by the user. Cannot be NULL.
 * @param p_context    Context passed to event handler.
 *
 * @retval NRFX_SUCCESS             Initialization was successful.
 * @retval NRFX_ERROR_INVALID_STATE Driver is already initialized.
 */
nrfx_err_t nrfx_ipc_init(uint8_t irq_priority, nrfx_ipc_handler_t handler, void * p_context);

/**
 * @brief Function for loading configuration directly into IPC peripheral.
 *
 * @param p_config Pointer to the structure with the initial configuration.
 */
void nrfx_ipc_config_load(nrfx_ipc_config_t const * p_config);

/**
 * @brief Function for convey signal on configured channels.
 *
 * Events connected to the IPC channels configured within this signal will
 * be set and can generate interrupts when configured.
 *
 * @param send_index Index of the SEND task to trigger.
 */
NRFX_STATIC_INLINE void nrfx_ipc_signal(uint8_t send_index);

/**
 * @brief Function for storing data in GPMEM register in the IPC peripheral.
 *
 * @param mem_index Index of the memory cell.
 * @param data      Data to be saved.
 */
NRFX_STATIC_INLINE void nrfx_ipc_gpmem_set(uint8_t mem_index, uint32_t data);

/**
 * @brief Function for getting data from the GPMEM register in the IPC peripheral.
 *
 * @param mem_index Index of the memory cell.
 *
 * @return Saved data.
 */
NRFX_STATIC_INLINE uint32_t nrfx_ipc_mem_get(uint8_t mem_index);

/** @brief Function for uninitializing the IPC module. */
void nrfx_ipc_uninit(void);

/**
 * @brief Function for enabling events to generate interrupt.
 *
 * @param event_index Index of event to be enabled.
 */
void nrfx_ipc_receive_event_enable(uint8_t event_index);

/**
 * @brief Function for disabling events from generate interrupt.
 *
 * @param event_index Index of event to be disabled.
 */
void nrfx_ipc_receive_event_disable(uint8_t event_index);

/**
 * @brief Function for enabling set of events to generate interrupt.
 *
 * @param event_bitmask Bitmask with events to be enabled.
 */
void nrfx_ipc_receive_event_group_enable(uint32_t event_bitmask);

/**
 * @brief Function for disabling set of events from generate interrupt.
 *
 * @param event_bitmask Bitmask with events to be disabled.
 */
void nrfx_ipc_receive_event_group_disable(uint32_t event_bitmask);

/**
 * @brief Function for assigning event to the IPC channel.
 *
 * @param event_index   Index of the event to be configured.
 * @param channel_index Index of the channel to which event will be connected.
 */
void nrfx_ipc_receive_event_channel_assign(uint8_t event_index, uint8_t channel_index);

/**
 * @brief Function for assigning signal to the IPC channel.
 *
 * @param send_index    Index of the signal to be configured.
 * @param channel_index Index of the instance of channel.
 */
void nrfx_ipc_send_task_channel_assign(uint8_t send_index, uint8_t channel_index);

/**
 * @brief Function for assigning event to the IPC channels.
 *
 * @param event_index     Index of the event to be configured.
 * @param channel_bitmask Bitmask with channels to which event will be connected.
 */
NRFX_STATIC_INLINE void nrfx_ipc_receive_config_set(uint8_t event_index, uint32_t channel_bitmask);

/**
 * @brief Function for assigning signal to the IPC channels.
 *
 * @param send_index      Index of the signal to be configured.
 * @param channel_bitmask Bitmask with channels to which signal will be connected.
 */
NRFX_STATIC_INLINE void nrfx_ipc_send_config_set(uint8_t send_index, uint32_t channel_bitmask);

/** @} */


#ifndef NRFX_DECLARE_ONLY

NRFX_STATIC_INLINE void nrfx_ipc_gpmem_set(uint8_t mem_index, uint32_t data)
{
    NRFX_ASSERT(mem_index < NRFX_ARRAY_SIZE(NRF_IPC->GPMEM));
    nrf_ipc_gpmem_set(NRF_IPC, mem_index, data);
}

NRFX_STATIC_INLINE uint32_t nrfx_ipc_mem_get(uint8_t mem_index)
{
    NRFX_ASSERT(mem_index < NRFX_ARRAY_SIZE(NRF_IPC->GPMEM));
    return nrf_ipc_gpmem_get(NRF_IPC, mem_index);
}

NRFX_STATIC_INLINE void nrfx_ipc_signal(uint8_t send_index)
{
    NRFX_ASSERT(send_index < IPC_CONF_NUM);
    nrf_ipc_task_trigger(NRF_IPC, nrf_ipc_send_task_get(send_index));
}

NRFX_STATIC_INLINE void nrfx_ipc_receive_config_set(uint8_t event_index, uint32_t channel_bitmask)
{
    NRFX_ASSERT(event_index < IPC_CONF_NUM);
    nrf_ipc_receive_config_set(NRF_IPC, event_index, channel_bitmask);
}

NRFX_STATIC_INLINE void nrfx_ipc_send_config_set(uint8_t send_index, uint32_t channel_bitmask)
{
    NRFX_ASSERT(send_index < IPC_CONF_NUM);
    nrf_ipc_send_config_set(NRF_IPC, send_index, channel_bitmask);
}

#endif // NRFX_DECLARE_ONLY


void nrfx_ipc_irq_handler(void);


#ifdef __cplusplus
}
#endif

#endif // NRFX_IPC_H__
