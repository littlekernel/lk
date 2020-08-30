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

#ifndef NRF_TWIS_H__
#define NRF_TWIS_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_twis_hal TWIS HAL
 * @{
 * @ingroup nrf_twis
 * @brief   Hardware access layer for managing the Two Wire Interface Slave with EasyDMA
 *          (TWIS) peripheral.
 */

/** @brief TWIS tasks. */
typedef enum
{
    NRF_TWIS_TASK_STOP      = offsetof(NRF_TWIS_Type, TASKS_STOP),      /**< Stop TWIS transaction. */
    NRF_TWIS_TASK_SUSPEND   = offsetof(NRF_TWIS_Type, TASKS_SUSPEND),   /**< Suspend TWIS transaction. */
    NRF_TWIS_TASK_RESUME    = offsetof(NRF_TWIS_Type, TASKS_RESUME),    /**< Resume TWIS transaction. */
    NRF_TWIS_TASK_PREPARERX = offsetof(NRF_TWIS_Type, TASKS_PREPARERX), /**< Prepare the TWIS slave to respond to a write command. */
    NRF_TWIS_TASK_PREPARETX = offsetof(NRF_TWIS_Type, TASKS_PREPARETX)  /**< Prepare the TWIS slave to respond to a read command. */
} nrf_twis_task_t;

/** @brief TWIS events. */
typedef enum
{
    NRF_TWIS_EVENT_STOPPED   = offsetof(NRF_TWIS_Type, EVENTS_STOPPED),   /**< TWIS stopped. */
    NRF_TWIS_EVENT_ERROR     = offsetof(NRF_TWIS_Type, EVENTS_ERROR),     /**< TWIS error. */
    NRF_TWIS_EVENT_RXSTARTED = offsetof(NRF_TWIS_Type, EVENTS_RXSTARTED), /**< Receive sequence started. */
    NRF_TWIS_EVENT_TXSTARTED = offsetof(NRF_TWIS_Type, EVENTS_TXSTARTED), /**< Transmit sequence started. */
    NRF_TWIS_EVENT_WRITE     = offsetof(NRF_TWIS_Type, EVENTS_WRITE),     /**< Write command received. */
    NRF_TWIS_EVENT_READ      = offsetof(NRF_TWIS_Type, EVENTS_READ)       /**< Read command received. */
} nrf_twis_event_t;

/** @brief TWIS shortcuts. */
typedef enum
{
    NRF_TWIS_SHORT_WRITE_SUSPEND_MASK   = TWIS_SHORTS_WRITE_SUSPEND_Msk,   /**< Shortcut between WRITE event and SUSPEND task. */
    NRF_TWIS_SHORT_READ_SUSPEND_MASK    = TWIS_SHORTS_READ_SUSPEND_Msk,    /**< Shortcut between READ event and SUSPEND task. */
} nrf_twis_short_mask_t;

/** @brief TWIS interrupts. */
typedef enum
{
    NRF_TWIS_INT_STOPPED_MASK   = TWIS_INTEN_STOPPED_Msk,   /**< Interrupt on STOPPED event. */
    NRF_TWIS_INT_ERROR_MASK     = TWIS_INTEN_ERROR_Msk,     /**< Interrupt on ERROR event. */
    NRF_TWIS_INT_RXSTARTED_MASK = TWIS_INTEN_RXSTARTED_Msk, /**< Interrupt on RXSTARTED event. */
    NRF_TWIS_INT_TXSTARTED_MASK = TWIS_INTEN_TXSTARTED_Msk, /**< Interrupt on TXSTARTED event. */
    NRF_TWIS_INT_WRITE_MASK     = TWIS_INTEN_WRITE_Msk,     /**< Interrupt on WRITE event. */
    NRF_TWIS_INT_READ_MASK      = TWIS_INTEN_READ_Msk,      /**< Interrupt on READ event. */
} nrf_twis_int_mask_t;

/** @brief TWIS error source. */
typedef enum
{
    NRF_TWIS_ERROR_OVERFLOW  = TWIS_ERRORSRC_OVERFLOW_Msk, /**< RX buffer overflow detected, and prevented. */
    NRF_TWIS_ERROR_DATA_NACK = TWIS_ERRORSRC_DNACK_Msk,    /**< NACK sent after receiving a data byte. */
    NRF_TWIS_ERROR_OVERREAD  = TWIS_ERRORSRC_OVERREAD_Msk  /**< TX buffer over-read detected, and prevented. */
} nrf_twis_error_t;

/** @brief TWIS address matching configuration. */
typedef enum
{
    NRF_TWIS_CONFIG_ADDRESS0_MASK  = TWIS_CONFIG_ADDRESS0_Msk, /**< Enable or disable address matching on ADDRESS[0]. */
    NRF_TWIS_CONFIG_ADDRESS1_MASK  = TWIS_CONFIG_ADDRESS1_Msk, /**< Enable or disable address matching on ADDRESS[1]. */
    NRF_TWIS_CONFIG_ADDRESS01_MASK = TWIS_CONFIG_ADDRESS0_Msk | TWIS_CONFIG_ADDRESS1_Msk /**< Enable both address matching. */
} nrf_twis_config_addr_mask_t;

/**
 * @brief Smallest variable type to hold the TWI address.
 *
 * Variable of the minimum size that can hold a single TWI address.
 *
 * @note Defined to make it simple to change if the new TWI supports for example
 *       10 bit addressing mode.
 */
typedef uint8_t nrf_twis_address_t;

/**
 * @brief Function for activating the specified TWIS task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task to be activated.
 */
NRF_STATIC_INLINE void nrf_twis_task_trigger(NRF_TWIS_Type * p_reg, nrf_twis_task_t task);

/**
 * @brief Function for returning the address of the specified TWIS task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  The specified task.
 *
 * @return Task address.
 */
NRF_STATIC_INLINE uint32_t nrf_twis_task_address_get(NRF_TWIS_Type const * p_reg,
                                                     nrf_twis_task_t       task);

/**
 * @brief Function for clearing the specified event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event The specified event.
 */
NRF_STATIC_INLINE void nrf_twis_event_clear(NRF_TWIS_Type *  p_reg,
                                            nrf_twis_event_t event);

/**
 * @brief Function for retrieving the state of the TWIS event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_twis_event_check(NRF_TWIS_Type const * p_reg,
                                            nrf_twis_event_t      event);

/**
 * @brief Function for getting and clearing the state of the specified event.
 *
 * This function checks the state of the event and clears it.
 *
 * @param[in,out] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in]     event Event.
 *
 * @retval true  The event was set.
 * @retval false The event was not set.
 */
NRF_STATIC_INLINE bool nrf_twis_event_get_and_clear(NRF_TWIS_Type *  p_reg,
                                                    nrf_twis_event_t event);

/**
 * @brief Function for returning the address of the specified TWIS event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event.
 *
 * @return Address.
 */
NRF_STATIC_INLINE uint32_t nrf_twis_event_address_get(NRF_TWIS_Type const * p_reg,
                                                      nrf_twis_event_t      event);

/**
 * @brief Function for setting a shortcut.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of shortcuts to be enabled.
 */
NRF_STATIC_INLINE void nrf_twis_shorts_enable(NRF_TWIS_Type * p_reg, uint32_t mask);

/**
 * @brief Function for clearing shortcuts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of shortcuts to be disabled.
 */
NRF_STATIC_INLINE void nrf_twis_shorts_disable(NRF_TWIS_Type * p_reg, uint32_t mask);

/**
 * @brief Function for getting the shorts mask.
 *
 * Function returns shorts register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Flags of currently enabled shortcuts
 */
NRF_STATIC_INLINE uint32_t nrf_twis_shorts_get(NRF_TWIS_Type const * p_reg);

/**
 * @brief Function for enabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
NRF_STATIC_INLINE void nrf_twis_int_enable(NRF_TWIS_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_twis_int_enable_check(NRF_TWIS_Type const * p_reg, uint32_t mask);

/**
 * @brief Function for disabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
NRF_STATIC_INLINE void nrf_twis_int_disable(NRF_TWIS_Type * p_reg, uint32_t mask);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        TWIS task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
NRF_STATIC_INLINE void nrf_twis_subscribe_set(NRF_TWIS_Type * p_reg,
                                              nrf_twis_task_t task,
                                              uint8_t         channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        TWIS task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_twis_subscribe_clear(NRF_TWIS_Type * p_reg,
                                                nrf_twis_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        TWIS event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
NRF_STATIC_INLINE void nrf_twis_publish_set(NRF_TWIS_Type *  p_reg,
                                            nrf_twis_event_t event,
                                            uint8_t         channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        TWIS event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_twis_publish_clear(NRF_TWIS_Type *  p_reg,
                                              nrf_twis_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for retrieving and clearing the TWIS error source.
 *
 * @attention Error sources are cleared after read.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Error source mask with values from @ref nrf_twis_error_t.
 */
NRF_STATIC_INLINE uint32_t nrf_twis_error_source_get_and_clear(NRF_TWIS_Type * p_reg);

/**
 * @brief Function for getting information about which of the addresses matched.
 *
 * Function returns index in the address table
 * that points to the address that already matched.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Index of matched address.
 */
NRF_STATIC_INLINE uint_fast8_t nrf_twis_match_get(NRF_TWIS_Type const * p_reg);

/**
 * @brief Function for enabling TWIS.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_twis_enable(NRF_TWIS_Type * p_reg);

/**
 * @brief Function for disabling TWIS.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_twis_disable(NRF_TWIS_Type * p_reg);

/**
 * @brief Function for configuring TWIS pins.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] scl   SCL pin number.
 * @param[in] sda   SDA pin number.
 */
NRF_STATIC_INLINE void nrf_twis_pins_set(NRF_TWIS_Type * p_reg, uint32_t scl, uint32_t sda);

/**
 * @brief Function for retrieving the SCL pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return SCL pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_twis_scl_pin_get(NRF_TWIS_Type const * p_reg);

/**
 * @brief Function for retrieving the SDA pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return SDA pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_twis_sda_pin_get(NRF_TWIS_Type const * p_reg);

/**
 * @brief Function for setting the receive buffer.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] p_buf  Pointer to the buffer for received data.
 * @param[in] length Maximum number of data bytes to receive.
 */
NRF_STATIC_INLINE void nrf_twis_rx_buffer_set(NRF_TWIS_Type * p_reg,
                                              uint8_t       * p_buf,
                                              size_t          length);

/**
 * @brief Function that prepares TWIS for receiving
 *
 * This function sets receive buffer and then sets NRF_TWIS_TASK_PREPARERX task.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] p_buf  Pointer to the buffer for received data.
 * @param[in] length Maximum number of data bytes to receive.
 */
NRF_STATIC_INLINE void nrf_twis_rx_prepare(NRF_TWIS_Type * p_reg,
                                           uint8_t       * p_buf,
                                           size_t          length);

/**
 * @brief Function for getting number of bytes received in the last transaction.
 *
 * @param[in] p_reg TWIS instance.
 *
 * @return Amount of bytes received.
 * */
NRF_STATIC_INLINE size_t nrf_twis_rx_amount_get(NRF_TWIS_Type const * p_reg);

/**
 * @brief Function for setting the transmit buffer.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] p_buf  Pointer to the buffer with data to send.
 * @param[in] length Maximum number of data bytes to transmit.
 */
NRF_STATIC_INLINE void nrf_twis_tx_buffer_set(NRF_TWIS_Type * p_reg,
                                              uint8_t const * p_buf,
                                              size_t          length);

/**
 * @brief Function for preparing TWIS for transmitting.
 *
 * This function sets transmit buffer and then sets NRF_TWIS_TASK_PREPARETX task.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] p_buf  Pointer to the buffer with data to send.
 * @param[in] length Maximum number of data bytes to transmit.
 */
NRF_STATIC_INLINE void nrf_twis_tx_prepare(NRF_TWIS_Type * p_reg,
                                           uint8_t const * p_buf,
                                           size_t          length);

/**
 * @brief Function for getting the number of bytes transmitted in the last transaction.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Amount of bytes transmitted.
 */
NRF_STATIC_INLINE size_t nrf_twis_tx_amount_get(NRF_TWIS_Type const * p_reg);

/**
 * @brief Function for setting the slave address.
 *
 * Function sets the selected address for this TWI interface.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] n     Index of address to be set.
 * @param[in] addr  Addres to be set.
 *
 * @sa nrf_twis_config_address_set
 * @sa nrf_twis_config_address_get
 */
NRF_STATIC_INLINE void nrf_twis_address_set(NRF_TWIS_Type *    p_reg,
                                            uint_fast8_t       n,
                                            nrf_twis_address_t addr);

/**
 * @brief Function for retrieving configured slave address.
 *
 * Function gets the selected address for this TWI interface.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] n     Index of address to get.
 *
 * @return Configured slave address.
 */
NRF_STATIC_INLINE nrf_twis_address_t nrf_twis_address_get(NRF_TWIS_Type const * p_reg,
                                                          uint_fast8_t          n);

/**
 * @brief Function for setting the device address configuration.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] addr_mask Mask of address indexes of what device should answer to.
 *
 * @sa nrf_twis_address_set
 */
NRF_STATIC_INLINE void nrf_twis_config_address_set(NRF_TWIS_Type *             p_reg,
                                                   nrf_twis_config_addr_mask_t addr_mask);

/**
 * @brief Function for retrieving the device address configuration.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Mask of address indexes of what device should answer to.
 */
NRF_STATIC_INLINE nrf_twis_config_addr_mask_t nrf_twis_config_address_get(
        NRF_TWIS_Type const * p_reg);

/**
 * @brief Function for setting the over-read character.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] orc   Over-read character. Character clocked out in case of
 *                  over-read of the TXD buffer.
 */
NRF_STATIC_INLINE void nrf_twis_orc_set(NRF_TWIS_Type * p_reg,
                                        uint8_t         orc);

/**
 * @brief Function for setting the over-read character.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Over-read character configured for selected instance.
 */
NRF_STATIC_INLINE uint8_t nrf_twis_orc_get(NRF_TWIS_Type const * p_reg);

#if defined(TWIS_TXD_LIST_LIST_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for enabling the TX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_twis_tx_list_enable(NRF_TWIS_Type * p_reg);

/**
 * @brief Function for disabling the TX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_twis_tx_list_disable(NRF_TWIS_Type * p_reg);
#endif // defined(TWIS_TXD_LIST_LIST_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(TWIS_RXD_LIST_LIST_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for enabling the RX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_twis_rx_list_enable(NRF_TWIS_Type * p_reg);

/**
 * @brief Function for disabling the RX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_twis_rx_list_disable(NRF_TWIS_Type * p_reg);
#endif // defined(TWIS_RXD_LIST_LIST_Msk) || defined(__NRFX_DOXYGEN__)

/** @} */ /*  End of nrf_twis_hal */

#ifndef NRF_DECLARE_ONLY

/* ------------------------------------------------------------------------------------------------
 *  Internal functions
 */

/**
 * @internal
 * @brief Internal function for getting task or event register address.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] offset Offset of the register from the beginning of the instance.
 *
 * @attention Offset must be modulo 4 value. In other case, hardware fault can occur.
 * @return Pointer to the register.
 */
NRF_STATIC_INLINE volatile uint32_t* nrf_twis_getRegPtr(NRF_TWIS_Type * p_reg, uint32_t offset)
{
    return (volatile uint32_t*)((uint8_t *)p_reg + (uint32_t)offset);
}

/**
 * @internal
 * @brief Internal function for getting task/event register address - constant version.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] offset Offset of the register from the beginning of the instance.
 *
 * @attention Offset must be modulo 4 value. In other case, hardware fault can occur.
 * @return Pointer to the register.
 */
NRF_STATIC_INLINE volatile const uint32_t* nrf_twis_getRegPtr_c(NRF_TWIS_Type const * p_reg,
                                                                uint32_t              offset)
{
    return (volatile const uint32_t*)((uint8_t *)p_reg + (uint32_t)offset);
}


/* ------------------------------------------------------------------------------------------------
 *  Interface functions definitions
 */


NRF_STATIC_INLINE void nrf_twis_task_trigger(NRF_TWIS_Type * p_reg, nrf_twis_task_t task)
{
    *(nrf_twis_getRegPtr(p_reg, (uint32_t)task)) = 1UL;
}

NRF_STATIC_INLINE uint32_t nrf_twis_task_address_get(NRF_TWIS_Type const * p_reg,
                                                     nrf_twis_task_t       task)
{
    return (uint32_t)nrf_twis_getRegPtr_c(p_reg, (uint32_t)task);
}

NRF_STATIC_INLINE void nrf_twis_event_clear(NRF_TWIS_Type * p_reg, nrf_twis_event_t event)
{
    *(nrf_twis_getRegPtr(p_reg, (uint32_t)event)) = 0UL;
    nrf_event_readback((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE bool nrf_twis_event_check(NRF_TWIS_Type const * p_reg, nrf_twis_event_t event)
{
    return (bool)*nrf_twis_getRegPtr_c(p_reg, (uint32_t)event);
}

NRF_STATIC_INLINE bool nrf_twis_event_get_and_clear(NRF_TWIS_Type * p_reg, nrf_twis_event_t event)
{
    bool ret = nrf_twis_event_check(p_reg, event);
    if (ret)
    {
        nrf_twis_event_clear(p_reg, event);
    }
    return ret;
}

NRF_STATIC_INLINE uint32_t nrf_twis_event_address_get(NRF_TWIS_Type const * p_reg,
                                                      nrf_twis_event_t      event)
{
    return (uint32_t)nrf_twis_getRegPtr_c(p_reg, (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_twis_shorts_enable(NRF_TWIS_Type * p_reg, uint32_t mask)
{
    p_reg->SHORTS |= mask;
}

NRF_STATIC_INLINE void nrf_twis_shorts_disable(NRF_TWIS_Type * p_reg, uint32_t mask)
{
    if (~0U == mask)
    {
        /* Optimized version for "disable all" */
        p_reg->SHORTS = 0;
    }
    else
    {
        p_reg->SHORTS &= ~mask;
    }
}

NRF_STATIC_INLINE uint32_t nrf_twis_shorts_get(NRF_TWIS_Type const * p_reg)
{
    return p_reg->SHORTS;
}

NRF_STATIC_INLINE void nrf_twis_int_enable(NRF_TWIS_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE uint32_t nrf_twis_int_enable_check(NRF_TWIS_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

NRF_STATIC_INLINE void nrf_twis_int_disable(NRF_TWIS_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

#if defined(DPPI_PRESENT)
NRF_STATIC_INLINE void nrf_twis_subscribe_set(NRF_TWIS_Type * p_reg,
                                              nrf_twis_task_t task,
                                              uint8_t         channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | TWIS_SUBSCRIBE_STOP_EN_Msk);
}

NRF_STATIC_INLINE void nrf_twis_subscribe_clear(NRF_TWIS_Type * p_reg,
                                                nrf_twis_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

NRF_STATIC_INLINE void nrf_twis_publish_set(NRF_TWIS_Type *  p_reg,
                                            nrf_twis_event_t event,
                                            uint8_t          channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | TWIS_PUBLISH_STOPPED_EN_Msk);
}

NRF_STATIC_INLINE void nrf_twis_publish_clear(NRF_TWIS_Type *  p_reg,
                                              nrf_twis_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

NRF_STATIC_INLINE uint32_t nrf_twis_error_source_get_and_clear(NRF_TWIS_Type * p_reg)
{
    uint32_t ret = p_reg->ERRORSRC;
    p_reg->ERRORSRC = ret;
    return ret;
}

NRF_STATIC_INLINE uint_fast8_t nrf_twis_match_get(NRF_TWIS_Type const * p_reg)
{
    return (uint_fast8_t)p_reg->MATCH;
}

NRF_STATIC_INLINE void nrf_twis_enable(NRF_TWIS_Type * p_reg)
{
    p_reg->ENABLE = (TWIS_ENABLE_ENABLE_Enabled << TWIS_ENABLE_ENABLE_Pos);
}

NRF_STATIC_INLINE void nrf_twis_disable(NRF_TWIS_Type * p_reg)
{
    p_reg->ENABLE = (TWIS_ENABLE_ENABLE_Disabled << TWIS_ENABLE_ENABLE_Pos);
}

NRF_STATIC_INLINE void nrf_twis_pins_set(NRF_TWIS_Type * p_reg, uint32_t scl, uint32_t sda)
{
    p_reg->PSEL.SCL = scl;
    p_reg->PSEL.SDA = sda;
}

NRF_STATIC_INLINE uint32_t nrf_twis_scl_pin_get(NRF_TWIS_Type const * p_reg)
{
    return p_reg->PSEL.SCL;
}

NRF_STATIC_INLINE uint32_t nrf_twis_sda_pin_get(NRF_TWIS_Type const * p_reg)
{
    return p_reg->PSEL.SDA;
}

NRF_STATIC_INLINE void nrf_twis_rx_buffer_set(NRF_TWIS_Type * p_reg,
                                              uint8_t *       p_buf,
                                              size_t          length)
{
    p_reg->RXD.PTR    = (uint32_t)p_buf;
    p_reg->RXD.MAXCNT = length;
}

NRF_STATIC_INLINE void nrf_twis_rx_prepare(NRF_TWIS_Type *   p_reg,
                                           uint8_t *         p_buf,
                                           size_t length)
{
    nrf_twis_rx_buffer_set(p_reg, p_buf, length);
    nrf_twis_task_trigger(p_reg, NRF_TWIS_TASK_PREPARERX);
}

NRF_STATIC_INLINE size_t nrf_twis_rx_amount_get(NRF_TWIS_Type const * p_reg)
{
    return p_reg->RXD.AMOUNT;
}

NRF_STATIC_INLINE void nrf_twis_tx_buffer_set(NRF_TWIS_Type * p_reg,
                                              uint8_t const * p_buf,
                                              size_t          length)
{
    p_reg->TXD.PTR    = (uint32_t)p_buf;
    p_reg->TXD.MAXCNT = length;
}

NRF_STATIC_INLINE void nrf_twis_tx_prepare(NRF_TWIS_Type * p_reg,
                                           uint8_t const * p_buf,
                                           size_t          length)
{
    nrf_twis_tx_buffer_set(p_reg, p_buf, length);
    nrf_twis_task_trigger(p_reg, NRF_TWIS_TASK_PREPARETX);
}

NRF_STATIC_INLINE size_t nrf_twis_tx_amount_get(NRF_TWIS_Type const * p_reg)
{
    return p_reg->TXD.AMOUNT;
}

NRF_STATIC_INLINE void nrf_twis_address_set(NRF_TWIS_Type *    p_reg,
                                            uint_fast8_t       n,
                                            nrf_twis_address_t addr)
{
    p_reg->ADDRESS[n] = addr;
}

NRF_STATIC_INLINE nrf_twis_address_t nrf_twis_address_get(NRF_TWIS_Type const * p_reg,
                                                          uint_fast8_t          n)
{
    return (nrf_twis_address_t)p_reg->ADDRESS[n];
}
NRF_STATIC_INLINE void nrf_twis_config_address_set(NRF_TWIS_Type *             p_reg,
                                                   nrf_twis_config_addr_mask_t addr_mask)
{
    /* This is the only configuration in TWIS - just write it without masking */
    p_reg->CONFIG = addr_mask;
}

NRF_STATIC_INLINE nrf_twis_config_addr_mask_t nrf_twis_config_address_get(
        NRF_TWIS_Type const * p_reg)
{
    return (nrf_twis_config_addr_mask_t)(p_reg->CONFIG & TWIS_ADDRESS_ADDRESS_Msk);
}

NRF_STATIC_INLINE void nrf_twis_orc_set(NRF_TWIS_Type * p_reg, uint8_t orc)
{
    p_reg->ORC = orc;
}

NRF_STATIC_INLINE uint8_t nrf_twis_orc_get(NRF_TWIS_Type const * p_reg)
{
    return (uint8_t)p_reg->ORC;
}

#if defined(TWIS_TXD_LIST_LIST_Msk)
NRF_STATIC_INLINE void nrf_twis_tx_list_enable(NRF_TWIS_Type * p_reg)
{
    p_reg->TXD.LIST = TWIS_TXD_LIST_LIST_ArrayList << TWIS_TXD_LIST_LIST_Pos;
}

NRF_STATIC_INLINE void nrf_twis_tx_list_disable(NRF_TWIS_Type * p_reg)
{
    p_reg->TXD.LIST = TWIS_TXD_LIST_LIST_Disabled << TWIS_TXD_LIST_LIST_Pos;
}

#endif // defined(TWIS_TXD_LIST_LIST_Msk)

#if defined(TWIS_RXD_LIST_LIST_Msk)
NRF_STATIC_INLINE void nrf_twis_rx_list_enable(NRF_TWIS_Type * p_reg)
{
    p_reg->RXD.LIST = TWIS_RXD_LIST_LIST_ArrayList << TWIS_RXD_LIST_LIST_Pos;
}

NRF_STATIC_INLINE void nrf_twis_rx_list_disable(NRF_TWIS_Type * p_reg)
{
    p_reg->RXD.LIST = TWIS_RXD_LIST_LIST_Disabled << TWIS_RXD_LIST_LIST_Pos;
}
#endif // defined(TWIS_RXD_LIST_LIST_Msk)

#endif /* NRF_DECLARE_ONLY */


#ifdef __cplusplus
}
#endif

#endif /* NRF_TWIS_H__ */
