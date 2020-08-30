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

#ifndef NRF_IPC_H__
#define NRF_IPC_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_ipc_hal IPC HAL
 * @{
 * @ingroup nrf_ipc
 * @brief   Hardware access layer for managing the Interprocessor Communication (IPC) peripheral.
 */

/** @brief IPC tasks. */
typedef enum
{
    NRF_IPC_TASK_SEND_0  = offsetof(NRF_IPC_Type, TASKS_SEND[0]),  ///< Trigger events on channels enabled in SEND_CNF[0].
    NRF_IPC_TASK_SEND_1  = offsetof(NRF_IPC_Type, TASKS_SEND[1]),  ///< Trigger events on channels enabled in SEND_CNF[1].
    NRF_IPC_TASK_SEND_2  = offsetof(NRF_IPC_Type, TASKS_SEND[2]),  ///< Trigger events on channels enabled in SEND_CNF[2].
    NRF_IPC_TASK_SEND_3  = offsetof(NRF_IPC_Type, TASKS_SEND[3]),  ///< Trigger events on channels enabled in SEND_CNF[3].
    NRF_IPC_TASK_SEND_4  = offsetof(NRF_IPC_Type, TASKS_SEND[4]),  ///< Trigger events on channels enabled in SEND_CNF[4].
    NRF_IPC_TASK_SEND_5  = offsetof(NRF_IPC_Type, TASKS_SEND[5]),  ///< Trigger events on channels enabled in SEND_CNF[5].
    NRF_IPC_TASK_SEND_6  = offsetof(NRF_IPC_Type, TASKS_SEND[6]),  ///< Trigger events on channels enabled in SEND_CNF[6].
    NRF_IPC_TASK_SEND_7  = offsetof(NRF_IPC_Type, TASKS_SEND[7]),  ///< Trigger events on channels enabled in SEND_CNF[7].
#if (IPC_TASKS_NUM > 8) || defined(__NRFX_DOXYGEN__)
    NRF_IPC_TASK_SEND_8  = offsetof(NRF_IPC_Type, TASKS_SEND[8]),  ///< Trigger events on channels enabled in SEND_CNF[8].
    NRF_IPC_TASK_SEND_9  = offsetof(NRF_IPC_Type, TASKS_SEND[9]),  ///< Trigger events on channels enabled in SEND_CNF[9].
    NRF_IPC_TASK_SEND_10 = offsetof(NRF_IPC_Type, TASKS_SEND[10]), ///< Trigger events on channels enabled in SEND_CNF[10].
    NRF_IPC_TASK_SEND_11 = offsetof(NRF_IPC_Type, TASKS_SEND[11]), ///< Trigger events on channels enabled in SEND_CNF[11].
    NRF_IPC_TASK_SEND_12 = offsetof(NRF_IPC_Type, TASKS_SEND[12]), ///< Trigger events on channels enabled in SEND_CNF[12].
    NRF_IPC_TASK_SEND_13 = offsetof(NRF_IPC_Type, TASKS_SEND[13]), ///< Trigger events on channels enabled in SEND_CNF[13].
    NRF_IPC_TASK_SEND_14 = offsetof(NRF_IPC_Type, TASKS_SEND[14]), ///< Trigger events on channels enabled in SEND_CNF[14].
    NRF_IPC_TASK_SEND_15 = offsetof(NRF_IPC_Type, TASKS_SEND[15]), ///< Trigger events on channels enabled in SEND_CNF[15].
#endif // (IPC_TASKS_NUM > 8) || defined(__NRFX_DOXYGEN__)
} nrf_ipc_task_t;

/** @brief IPC events. */
typedef enum
{
    NRF_IPC_EVENT_RECEIVE_0  = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[0]),  ///< Event received on the IPC channels enabled in RECEIVE_CNF[0] register.
    NRF_IPC_EVENT_RECEIVE_1  = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[1]),  ///< Event received on the IPC channels enabled in RECEIVE_CNF[1] register.
    NRF_IPC_EVENT_RECEIVE_2  = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[2]),  ///< Event received on the IPC channels enabled in RECEIVE_CNF[2] register.
    NRF_IPC_EVENT_RECEIVE_3  = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[3]),  ///< Event received on the IPC channels enabled in RECEIVE_CNF[3] register.
    NRF_IPC_EVENT_RECEIVE_4  = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[4]),  ///< Event received on the IPC channels enabled in RECEIVE_CNF[4] register.
    NRF_IPC_EVENT_RECEIVE_5  = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[5]),  ///< Event received on the IPC channels enabled in RECEIVE_CNF[5] register.
    NRF_IPC_EVENT_RECEIVE_6  = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[6]),  ///< Event received on the IPC channels enabled in RECEIVE_CNF[6] register.
    NRF_IPC_EVENT_RECEIVE_7  = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[7]),  ///< Event received on the IPC channels enabled in RECEIVE_CNF[7] register.
#if (IPC_EVENTS_NUM > 8) || defined(__NRFX_DOXYGEN__)
    NRF_IPC_EVENT_RECEIVE_8  = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[8]),  ///< Event received on the IPC channels enabled in RECEIVE_CNF[8] register.
    NRF_IPC_EVENT_RECEIVE_9  = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[9]),  ///< Event received on the IPC channels enabled in RECEIVE_CNF[9] register.
    NRF_IPC_EVENT_RECEIVE_10 = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[10]), ///< Event received on the IPC channels enabled in RECEIVE_CNF[10] register.
    NRF_IPC_EVENT_RECEIVE_11 = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[11]), ///< Event received on the IPC channels enabled in RECEIVE_CNF[11] register.
    NRF_IPC_EVENT_RECEIVE_12 = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[12]), ///< Event received on the IPC channels enabled in RECEIVE_CNF[12] register.
    NRF_IPC_EVENT_RECEIVE_13 = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[13]), ///< Event received on the IPC channels enabled in RECEIVE_CNF[13] register.
    NRF_IPC_EVENT_RECEIVE_14 = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[14]), ///< Event received on the IPC channels enabled in RECEIVE_CNF[14] register.
    NRF_IPC_EVENT_RECEIVE_15 = offsetof(NRF_IPC_Type, EVENTS_RECEIVE[15]), ///< Event received on the IPC channels enabled in RECEIVE_CNF[15] register.
#endif // (IPC_EVENTS_NUM > 8) || defined(__NRFX_DOXYGEN__)
} nrf_ipc_event_t;

/** @brief IPC channel positions. */
typedef enum
{
    NRF_IPC_CHANNEL_0 = IPC_RECEIVE_CNF_CHEN0_Msk,   ///< Bitmask position for IPC channel 0.
    NRF_IPC_CHANNEL_1 = IPC_RECEIVE_CNF_CHEN1_Msk,   ///< Bitmask position for IPC channel 1.
    NRF_IPC_CHANNEL_2 = IPC_RECEIVE_CNF_CHEN2_Msk,   ///< Bitmask position for IPC channel 2.
    NRF_IPC_CHANNEL_3 = IPC_RECEIVE_CNF_CHEN3_Msk,   ///< Bitmask position for IPC channel 3.
    NRF_IPC_CHANNEL_4 = IPC_RECEIVE_CNF_CHEN4_Msk,   ///< Bitmask position for IPC channel 4.
    NRF_IPC_CHANNEL_5 = IPC_RECEIVE_CNF_CHEN5_Msk,   ///< Bitmask position for IPC channel 5.
    NRF_IPC_CHANNEL_6 = IPC_RECEIVE_CNF_CHEN6_Msk,   ///< Bitmask position for IPC channel 6.
    NRF_IPC_CHANNEL_7 = IPC_RECEIVE_CNF_CHEN7_Msk,   ///< Bitmask position for IPC channel 7.
#if (IPC_CH_NUM > 8) || defined(__NRFX_DOXYGEN__)
    NRF_IPC_CHANNEL_8  = IPC_RECEIVE_CNF_CHEN8_Msk,  ///< Bitmask position for IPC channel 8.
    NRF_IPC_CHANNEL_9  = IPC_RECEIVE_CNF_CHEN9_Msk,  ///< Bitmask position for IPC channel 9.
    NRF_IPC_CHANNEL_10 = IPC_RECEIVE_CNF_CHEN10_Msk, ///< Bitmask position for IPC channel 10.
    NRF_IPC_CHANNEL_11 = IPC_RECEIVE_CNF_CHEN11_Msk, ///< Bitmask position for IPC channel 11.
    NRF_IPC_CHANNEL_12 = IPC_RECEIVE_CNF_CHEN12_Msk, ///< Bitmask position for IPC channel 12.
    NRF_IPC_CHANNEL_13 = IPC_RECEIVE_CNF_CHEN13_Msk, ///< Bitmask position for IPC channel 13.
    NRF_IPC_CHANNEL_14 = IPC_RECEIVE_CNF_CHEN14_Msk, ///< Bitmask position for IPC channel 14.
    NRF_IPC_CHANNEL_15 = IPC_RECEIVE_CNF_CHEN15_Msk, ///< Bitmask position for IPC channel 15.
#endif // (IPC_CH_NUM > 8) || defined(__NRFX_DOXYGEN__)
} nrf_ipc_channel_t;

/** @brief IPC interrupts. */
typedef enum
{
    NRF_IPC_INT_RECEIVE_0  = IPC_INTEN_RECEIVE0_Msk,  ///< Interrupt on receive event 0.
    NRF_IPC_INT_RECEIVE_1  = IPC_INTEN_RECEIVE1_Msk,  ///< Interrupt on receive event 1.
    NRF_IPC_INT_RECEIVE_2  = IPC_INTEN_RECEIVE2_Msk,  ///< Interrupt on receive event 2.
    NRF_IPC_INT_RECEIVE_3  = IPC_INTEN_RECEIVE3_Msk,  ///< Interrupt on receive event 3.
    NRF_IPC_INT_RECEIVE_4  = IPC_INTEN_RECEIVE4_Msk,  ///< Interrupt on receive event 4.
    NRF_IPC_INT_RECEIVE_5  = IPC_INTEN_RECEIVE5_Msk,  ///< Interrupt on receive event 5.
    NRF_IPC_INT_RECEIVE_6  = IPC_INTEN_RECEIVE6_Msk,  ///< Interrupt on receive event 6.
    NRF_IPC_INT_RECEIVE_7  = IPC_INTEN_RECEIVE7_Msk,  ///< Interrupt on receive event 7.
#if (IPC_EVENTS_NUM > 8) || defined(__NRFX_DOXYGEN__)
    NRF_IPC_INT_RECEIVE_8  = IPC_INTEN_RECEIVE8_Msk,  ///< Interrupt on receive event 8.
    NRF_IPC_INT_RECEIVE_9  = IPC_INTEN_RECEIVE9_Msk,  ///< Interrupt on receive event 9.
    NRF_IPC_INT_RECEIVE_10 = IPC_INTEN_RECEIVE10_Msk, ///< Interrupt on receive event 10.
    NRF_IPC_INT_RECEIVE_11 = IPC_INTEN_RECEIVE11_Msk, ///< Interrupt on receive event 11.
    NRF_IPC_INT_RECEIVE_12 = IPC_INTEN_RECEIVE12_Msk, ///< Interrupt on receive event 12.
    NRF_IPC_INT_RECEIVE_13 = IPC_INTEN_RECEIVE13_Msk, ///< Interrupt on receive event 13.
    NRF_IPC_INT_RECEIVE_14 = IPC_INTEN_RECEIVE14_Msk, ///< Interrupt on receive event 14.
    NRF_IPC_INT_RECEIVE_15 = IPC_INTEN_RECEIVE15_Msk, ///< Interrupt on receive event 15.
#endif // (IPC_EVENTS_NUM > 8) || defined(__NRFX_DOXYGEN__)
} nrf_ipc_int_mask_t;


/**
 * @brief Function for triggering the specified IPC task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task to be triggered.
 */
NRF_STATIC_INLINE void nrf_ipc_task_trigger(NRF_IPC_Type * p_reg, nrf_ipc_task_t task);

/**
 * @brief Function for getting the address of the specified IPC task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Specified task.
 *
 * @return Address of the specified task register.
 */
NRF_STATIC_INLINE uint32_t nrf_ipc_task_address_get(NRF_IPC_Type const * p_reg,
                                                    nrf_ipc_task_t       task);

/**
 * @brief Function for clearing the specified IPC event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to clear.
 */
NRF_STATIC_INLINE void nrf_ipc_event_clear(NRF_IPC_Type * p_reg, nrf_ipc_event_t event);

/**
 * @brief Function for retrieving the state of the IPC event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_ipc_event_check(NRF_IPC_Type const * p_reg, nrf_ipc_event_t event);

/**
 * @brief Function for getting the address of the specified IPC event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Specified event.
 *
 * @return Address of the specified event register.
 */
NRF_STATIC_INLINE uint32_t nrf_ipc_event_address_get(NRF_IPC_Type const * p_reg,
                                                     nrf_ipc_event_t      event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
NRF_STATIC_INLINE void nrf_ipc_int_enable(NRF_IPC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
NRF_STATIC_INLINE void nrf_ipc_int_disable(NRF_IPC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_ipc_int_enable_check(NRF_IPC_Type const * p_reg, uint32_t mask);

/**
 * @brief Function for retrieving the state of pending interrupts of the receive event.
 *
 * States of pending interrupt are saved as a bitmask. First position corresponds with
 * EVENTS_RECEIVE[0] event, second one with EVENTS_RECEIVE[1] etc.
 * One set at particular position means that interrupt for event is pending.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Bitmask with information about pending interrupts of EVENTS_RECEIVE[n] events.
 */
NRF_STATIC_INLINE uint32_t nrf_ipc_int_pending_get(NRF_IPC_Type const * p_reg);

/**
 * @brief Function for setting the DPPI subscribe configuration for a given
 *        IPC task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel DPPI channel through which to subscribe events.
 */
NRF_STATIC_INLINE void nrf_ipc_subscribe_set(NRF_IPC_Type * p_reg,
                                             nrf_ipc_task_t task,
                                             uint8_t        channel);

/**
 * @brief Function for clearing the DPPI subscribe configuration for a given
 *        IPC task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_ipc_subscribe_clear(NRF_IPC_Type * p_reg, nrf_ipc_task_t task);

/**
 * @brief Function for setting the DPPI publish configuration for a given
 *        IPC event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel DPPI channel through which to publish the event.
 */
NRF_STATIC_INLINE void nrf_ipc_publish_set(NRF_IPC_Type *  p_reg,
                                           nrf_ipc_event_t event,
                                           uint8_t         channel);

/**
 * @brief Function for clearing the DPPI publish configuration for a given
 *        IPC event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_ipc_publish_clear(NRF_IPC_Type * p_reg, nrf_ipc_event_t event);

/**
 * @brief Function for setting the configuration of the specified send task.
 *
 * @p channels_mask bitmask must be created with @ref nrf_ipc_channel_t values:
 * NRF_IPC_CHANNEL_0 | NRF_IPC_CHANNEL_1 | ... | NRF_IPC_CHANNEL_n
 *
 * @note This function overrides current configuration.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] index         Index of the send task.
 * @param[in] channels_mask Bitmask specifying channels that are to be enabled for this task.
 */
NRF_STATIC_INLINE void nrf_ipc_send_config_set(NRF_IPC_Type * p_reg,
                                               uint8_t        index,
                                               uint32_t       channels_mask);

/**
 * @brief Function for getting the configuration of the specified send task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] index Index of the send event configuration.
 *
 * @retval Bitmask of channels enabled for this task.
 */
NRF_STATIC_INLINE uint32_t nrf_ipc_send_config_get(NRF_IPC_Type * const p_reg, uint8_t index);

/**
 * @brief Function for assigning receive event to the IPC channels.
 *
 * @p channels_mask bitmask must be created with @ref nrf_ipc_channel_t values:
 * NRF_IPC_CHANNEL_0 | NRF_IPC_CHANNEL_1 | ... | NRF_IPC_CHANNEL_n
 *
 * @note This function overrides current configuration.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] index         Index of the receive event configuration.
 * @param[in] channels_mask Bitmask with IPC channels from which the receive event
 *                          will generate interrupts.
 */
NRF_STATIC_INLINE void nrf_ipc_receive_config_set(NRF_IPC_Type * p_reg,
                                                  uint8_t        index,
                                                  uint32_t       channels_mask);

/**
 * @brief Function for getting receive event configuration.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] index         Index of the receive event configuration.
 *
 * @return Mask of channels connected with receive event.
 */
NRF_STATIC_INLINE uint32_t nrf_ipc_receive_config_get(NRF_IPC_Type * const p_reg, uint8_t index);

/**
 * @brief Function for storing data in general purpose memory cell.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] index Index of the general purpose memory cell.
 * @param[in] data  Data to be stored.
 */
NRF_STATIC_INLINE void nrf_ipc_gpmem_set(NRF_IPC_Type * p_reg,
                                         uint8_t        index,
                                         uint32_t       data);

/**
 * @brief Function for getting value of the general purpose memory cell.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] index Index of the general purpose memory cell.
 *
 * @return Stored data.
 */
NRF_STATIC_INLINE uint32_t nrf_ipc_gpmem_get(NRF_IPC_Type const * p_reg, uint8_t index);

/**
 * @brief Function for getting SEND task by its index.
 *
 * @param[in] index Index of the SEND task.
 *
 * @return SEND task.
 */
NRF_STATIC_INLINE nrf_ipc_task_t nrf_ipc_send_task_get(uint8_t index);

/**
 * @brief Function for getting RECEIVE event by its index.
 *
 * @param[in] index Index of the RECEIVE event.
 *
 * @return RECEIVE event.
 */
NRF_STATIC_INLINE nrf_ipc_event_t nrf_ipc_receive_event_get(uint8_t index);

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_ipc_task_trigger(NRF_IPC_Type * p_reg, nrf_ipc_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

NRF_STATIC_INLINE uint32_t nrf_ipc_task_address_get(NRF_IPC_Type const * p_reg,
                                                    nrf_ipc_task_t       task)
{
    return ((uint32_t)p_reg + (uint32_t)task);
}

NRF_STATIC_INLINE void nrf_ipc_event_clear(NRF_IPC_Type * p_reg, nrf_ipc_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
}

NRF_STATIC_INLINE bool nrf_ipc_event_check(NRF_IPC_Type const * p_reg, nrf_ipc_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE uint32_t nrf_ipc_event_address_get(NRF_IPC_Type const * p_reg,
                                                     nrf_ipc_event_t      event)
{
    return ((uint32_t)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_ipc_int_enable(NRF_IPC_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE void nrf_ipc_int_disable(NRF_IPC_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

NRF_STATIC_INLINE uint32_t nrf_ipc_int_enable_check(NRF_IPC_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

NRF_STATIC_INLINE uint32_t nrf_ipc_int_pending_get(NRF_IPC_Type const * p_reg)
{
    return p_reg->INTPEND;
}

NRF_STATIC_INLINE void nrf_ipc_subscribe_set(NRF_IPC_Type * p_reg,
                                             nrf_ipc_task_t task,
                                             uint8_t        channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | IPC_SUBSCRIBE_SEND_EN_Msk);
}

NRF_STATIC_INLINE void nrf_ipc_subscribe_clear(NRF_IPC_Type * p_reg, nrf_ipc_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

NRF_STATIC_INLINE void nrf_ipc_publish_set(NRF_IPC_Type *  p_reg,
                                           nrf_ipc_event_t event,
                                           uint8_t         channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | IPC_PUBLISH_RECEIVE_EN_Msk);
}

NRF_STATIC_INLINE void nrf_ipc_publish_clear(NRF_IPC_Type *  p_reg, nrf_ipc_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}

NRF_STATIC_INLINE void nrf_ipc_send_config_set(NRF_IPC_Type * p_reg,
                                               uint8_t        index,
                                               uint32_t       channels_mask)
{
    p_reg->SEND_CNF[index] = channels_mask;
}

NRF_STATIC_INLINE uint32_t nrf_ipc_send_config_get(NRF_IPC_Type * const p_reg, uint8_t index)
{
    return p_reg->SEND_CNF[index];
}

NRF_STATIC_INLINE void nrf_ipc_receive_config_set(NRF_IPC_Type * p_reg,
                                                  uint8_t        index,
                                                  uint32_t       channels_mask)
{
    p_reg->RECEIVE_CNF[index] = channels_mask;
}


NRF_STATIC_INLINE uint32_t nrf_ipc_receive_config_get(NRF_IPC_Type * const p_reg, uint8_t index)
{
    return p_reg->RECEIVE_CNF[index];
}

NRF_STATIC_INLINE void nrf_ipc_gpmem_set(NRF_IPC_Type * p_reg,
                                         uint8_t        index,
                                         uint32_t       data)
{
    NRFX_ASSERT(index < IPC_GPMEM_NUM);
    p_reg->GPMEM[index] = data;
}

NRF_STATIC_INLINE uint32_t nrf_ipc_gpmem_get(NRF_IPC_Type const * p_reg,  uint8_t index)
{
    NRFX_ASSERT(index < IPC_GPMEM_NUM);
    return p_reg->GPMEM[index];
}

NRF_STATIC_INLINE nrf_ipc_task_t nrf_ipc_send_task_get(uint8_t index)
{
    NRFX_ASSERT(index < IPC_CH_NUM);
    return (nrf_ipc_task_t)(NRFX_OFFSETOF(NRF_IPC_Type, TASKS_SEND[index]));
}

NRF_STATIC_INLINE nrf_ipc_event_t nrf_ipc_receive_event_get(uint8_t index)
{
    NRFX_ASSERT(index < IPC_CH_NUM);
    return (nrf_ipc_event_t)(NRFX_OFFSETOF(NRF_IPC_Type, EVENTS_RECEIVE[index]));
}

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_IPC_H__
