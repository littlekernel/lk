/*
 * Copyright (c) 2014 - 2020, Nordic Semiconductor ASA
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

#ifndef NRF_RTC_H
#define NRF_RTC_H

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_rtc_hal RTC HAL
 * @{
 * @ingroup nrf_rtc
 * @brief   Hardware access layer for managing the Real Time Counter (RTC) peripheral.
 */

/** @brief Macro for getting the number of compare channels available in a given RTC instance. */
#define NRF_RTC_CC_CHANNEL_COUNT(id)  NRFX_CONCAT_3(RTC, id, _CC_NUM)

/** @brief Input frequency of the RTC instance. */
#define RTC_INPUT_FREQ 32768

/** @brief Macro for converting expected frequency to prescaler setting. */
#define RTC_FREQ_TO_PRESCALER(FREQ) (uint16_t)(((RTC_INPUT_FREQ) / (FREQ)) - 1)

/** @brief Macro for trimming values to the RTC bit width. */
#define RTC_WRAP(val) ((val) & RTC_COUNTER_COUNTER_Msk)

/** @brief Macro for creating the interrupt bitmask for the specified compare channel. */
#define RTC_CHANNEL_INT_MASK(ch)    ((uint32_t)(NRF_RTC_INT_COMPARE0_MASK) << (ch))

/** @brief Macro for obtaining the compare event for the specified channel. */
#define RTC_CHANNEL_EVENT_ADDR(ch)  (nrf_rtc_event_t)((NRF_RTC_EVENT_COMPARE_0) + (ch) * sizeof(uint32_t))


/** @brief RTC tasks. */
typedef enum
{
    NRF_RTC_TASK_START            = offsetof(NRF_RTC_Type,TASKS_START),     /**< Start. */
    NRF_RTC_TASK_STOP             = offsetof(NRF_RTC_Type,TASKS_STOP),      /**< Stop. */
    NRF_RTC_TASK_CLEAR            = offsetof(NRF_RTC_Type,TASKS_CLEAR),     /**< Clear. */
    NRF_RTC_TASK_TRIGGER_OVERFLOW = offsetof(NRF_RTC_Type,TASKS_TRIGOVRFLW),/**< Trigger overflow. */
} nrf_rtc_task_t;

/** @brief RTC events. */
typedef enum
{
    NRF_RTC_EVENT_TICK        = offsetof(NRF_RTC_Type,EVENTS_TICK),       /**< Tick event. */
    NRF_RTC_EVENT_OVERFLOW    = offsetof(NRF_RTC_Type,EVENTS_OVRFLW),     /**< Overflow event. */
    NRF_RTC_EVENT_COMPARE_0   = offsetof(NRF_RTC_Type,EVENTS_COMPARE[0]), /**< Compare 0 event. */
    NRF_RTC_EVENT_COMPARE_1   = offsetof(NRF_RTC_Type,EVENTS_COMPARE[1]), /**< Compare 1 event. */
    NRF_RTC_EVENT_COMPARE_2   = offsetof(NRF_RTC_Type,EVENTS_COMPARE[2]), /**< Compare 2 event. */
    NRF_RTC_EVENT_COMPARE_3   = offsetof(NRF_RTC_Type,EVENTS_COMPARE[3])  /**< Compare 3 event. */
} nrf_rtc_event_t;

/** @brief RTC interrupts. */
typedef enum
{
    NRF_RTC_INT_TICK_MASK     = RTC_INTENSET_TICK_Msk,     /**< RTC interrupt from tick event. */
    NRF_RTC_INT_OVERFLOW_MASK = RTC_INTENSET_OVRFLW_Msk,   /**< RTC interrupt from overflow event. */
    NRF_RTC_INT_COMPARE0_MASK = RTC_INTENSET_COMPARE0_Msk, /**< RTC interrupt from compare event on channel 0. */
    NRF_RTC_INT_COMPARE1_MASK = RTC_INTENSET_COMPARE1_Msk, /**< RTC interrupt from compare event on channel 1. */
    NRF_RTC_INT_COMPARE2_MASK = RTC_INTENSET_COMPARE2_Msk, /**< RTC interrupt from compare event on channel 2. */
    NRF_RTC_INT_COMPARE3_MASK = RTC_INTENSET_COMPARE3_Msk  /**< RTC interrupt from compare event on channel 3. */
} nrf_rtc_int_t;


/**
 * @brief Function for setting a compare value for a channel.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] ch     Channel.
 * @param[in] cc_val Compare value to be set.
 */
NRF_STATIC_INLINE  void nrf_rtc_cc_set(NRF_RTC_Type * p_reg, uint32_t ch, uint32_t cc_val);

/**
 * @brief Function for returning the compare value for a channel.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] ch    Channel.
 *
 * @return COMPARE[ch] value.
 */
NRF_STATIC_INLINE  uint32_t nrf_rtc_cc_get(NRF_RTC_Type const * p_reg, uint32_t ch);

/**
 * @brief Function for enabling interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Interrupt mask to be enabled.
 */
NRF_STATIC_INLINE void nrf_rtc_int_enable(NRF_RTC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Interrupt mask to be disabled.
 */
NRF_STATIC_INLINE void nrf_rtc_int_disable(NRF_RTC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_rtc_int_enable_check(NRF_RTC_Type const * p_reg, uint32_t mask);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        RTC task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
NRF_STATIC_INLINE void nrf_rtc_subscribe_set(NRF_RTC_Type * p_reg,
                                             nrf_rtc_task_t task,
                                             uint8_t        channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        RTC task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_rtc_subscribe_clear(NRF_RTC_Type * p_reg,
                                               nrf_rtc_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        RTC event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
NRF_STATIC_INLINE void nrf_rtc_publish_set(NRF_RTC_Type *  p_reg,
                                           nrf_rtc_event_t event,
                                           uint8_t         channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        RTC event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_rtc_publish_clear(NRF_RTC_Type *  p_reg,
                                             nrf_rtc_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for retrieving the state of the RTC event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_rtc_event_check(NRF_RTC_Type const * p_reg, nrf_rtc_event_t event);

/**
 * @brief Function for clearing an event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be cleared.
 */
NRF_STATIC_INLINE void nrf_rtc_event_clear(NRF_RTC_Type * p_reg, nrf_rtc_event_t event);

/**
 * @brief Function for returning a counter value.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Counter value.
 */
NRF_STATIC_INLINE uint32_t nrf_rtc_counter_get(NRF_RTC_Type const * p_reg);

/**
 * @brief Function for setting a prescaler value.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] val   Value to set the prescaler to.
 */
NRF_STATIC_INLINE void nrf_rtc_prescaler_set(NRF_RTC_Type * p_reg, uint32_t val);

/**
 * @brief Function for getting a prescaler value.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Prescaler value.
 */
NRF_STATIC_INLINE uint32_t nrf_rtc_prescaler_get(NRF_RTC_Type const * p_reg);

/**
 * @brief Function for returning the address of an event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Requested event.
 *
 * @return Address of the requested event register.
 */
NRF_STATIC_INLINE uint32_t nrf_rtc_event_address_get(NRF_RTC_Type const * p_reg,
                                                     nrf_rtc_event_t      event);

/**
 * @brief Function for returning the address of a task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Requested task.
 *
 * @return Address of the requested task register.
 */
NRF_STATIC_INLINE uint32_t nrf_rtc_task_address_get(NRF_RTC_Type const * p_reg,
                                                    nrf_rtc_task_t       task);

/**
 * @brief Function for starting a task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Requested task.
 */
NRF_STATIC_INLINE void nrf_rtc_task_trigger(NRF_RTC_Type * p_reg, nrf_rtc_task_t task);

/**
 * @brief Function for enabling events.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of event flags to be enabled.
 */
NRF_STATIC_INLINE void nrf_rtc_event_enable(NRF_RTC_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling an event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Requested event.
 */
NRF_STATIC_INLINE void nrf_rtc_event_disable(NRF_RTC_Type * p_reg, uint32_t event);

/**
 * @brief Function for getting the COMPARE event associated with the specified compare channel.
 *
 * @param[in] index Compare channel index.
 *
 * @return Requested COMPARE event.
 */
NRF_STATIC_INLINE nrf_rtc_event_t nrf_rtc_compare_event_get(uint8_t index);

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE  void nrf_rtc_cc_set(NRF_RTC_Type * p_reg, uint32_t ch, uint32_t cc_val)
{
    p_reg->CC[ch] = cc_val;
}

NRF_STATIC_INLINE  uint32_t nrf_rtc_cc_get(NRF_RTC_Type const * p_reg, uint32_t ch)
{
    return p_reg->CC[ch];
}

NRF_STATIC_INLINE void nrf_rtc_int_enable(NRF_RTC_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE void nrf_rtc_int_disable(NRF_RTC_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

NRF_STATIC_INLINE uint32_t nrf_rtc_int_enable_check(NRF_RTC_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

#if defined(DPPI_PRESENT)
NRF_STATIC_INLINE void nrf_rtc_subscribe_set(NRF_RTC_Type * p_reg,
                                             nrf_rtc_task_t task,
                                             uint8_t        channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | RTC_SUBSCRIBE_START_EN_Msk);
}

NRF_STATIC_INLINE void nrf_rtc_subscribe_clear(NRF_RTC_Type * p_reg,
                                               nrf_rtc_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

NRF_STATIC_INLINE void nrf_rtc_publish_set(NRF_RTC_Type *  p_reg,
                                           nrf_rtc_event_t event,
                                           uint8_t         channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | RTC_PUBLISH_TICK_EN_Msk);
}

NRF_STATIC_INLINE void nrf_rtc_publish_clear(NRF_RTC_Type *  p_reg,
                                             nrf_rtc_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

NRF_STATIC_INLINE bool nrf_rtc_event_check(NRF_RTC_Type const * p_reg, nrf_rtc_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_rtc_event_clear(NRF_RTC_Type * p_reg, nrf_rtc_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0;
    nrf_event_readback((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE uint32_t nrf_rtc_counter_get(NRF_RTC_Type const * p_reg)
{
     return p_reg->COUNTER;
}

NRF_STATIC_INLINE void nrf_rtc_prescaler_set(NRF_RTC_Type * p_reg, uint32_t val)
{
    NRFX_ASSERT(val <= (RTC_PRESCALER_PRESCALER_Msk >> RTC_PRESCALER_PRESCALER_Pos));
    p_reg->PRESCALER = val;
}

NRF_STATIC_INLINE uint32_t nrf_rtc_prescaler_get(NRF_RTC_Type const * p_reg)
{
    return p_reg->PRESCALER;
}

NRF_STATIC_INLINE uint32_t nrf_rtc_event_address_get(NRF_RTC_Type const * p_reg,
                                                     nrf_rtc_event_t      event)
{
    return (uint32_t)p_reg + event;
}

NRF_STATIC_INLINE uint32_t nrf_rtc_task_address_get(NRF_RTC_Type const * p_reg,
                                                    nrf_rtc_task_t       task)
{
    return (uint32_t)p_reg + task;
}

NRF_STATIC_INLINE void nrf_rtc_task_trigger(NRF_RTC_Type * p_reg, nrf_rtc_task_t task)
{
    *(__IO uint32_t *)((uint32_t)p_reg + task) = 1;
}

NRF_STATIC_INLINE void nrf_rtc_event_enable(NRF_RTC_Type * p_reg, uint32_t mask)
{
    p_reg->EVTENSET = mask;
}

NRF_STATIC_INLINE void nrf_rtc_event_disable(NRF_RTC_Type * p_reg, uint32_t mask)
{
    p_reg->EVTENCLR = mask;
}

NRF_STATIC_INLINE nrf_rtc_event_t nrf_rtc_compare_event_get(uint8_t index)
{
    return (nrf_rtc_event_t)NRFX_OFFSETOF(NRF_RTC_Type, EVENTS_COMPARE[index]);
}

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* NRF_RTC_H */
