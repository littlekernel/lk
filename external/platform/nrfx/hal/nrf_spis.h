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

#ifndef NRF_SPIS_H__
#define NRF_SPIS_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_spis_hal SPIS HAL
 * @{
 * @ingroup nrf_spis
 * @brief   Hardware access layer for managing the SPIS peripheral.
 */

/**
 * @brief This value can be used as a parameter for the @ref nrf_spis_pins_set
 *        function to specify that a given SPI signal (SCK, MOSI, or MISO)
 *        shall not be connected to a physical pin.
 */
#define NRF_SPIS_PIN_NOT_CONNECTED  0xFFFFFFFF


/** @brief SPIS tasks. */
typedef enum
{
    NRF_SPIS_TASK_ACQUIRE = offsetof(NRF_SPIS_Type, TASKS_ACQUIRE), ///< Acquire SPI semaphore.
    NRF_SPIS_TASK_RELEASE = offsetof(NRF_SPIS_Type, TASKS_RELEASE), ///< Release SPI semaphore, enabling the SPI slave to acquire it.
} nrf_spis_task_t;

/** @brief SPIS events. */
typedef enum
{
    NRF_SPIS_EVENT_END      = offsetof(NRF_SPIS_Type, EVENTS_END),     ///< Granted transaction completed.
    NRF_SPIS_EVENT_ACQUIRED = offsetof(NRF_SPIS_Type, EVENTS_ACQUIRED) ///< Semaphore acquired.
} nrf_spis_event_t;

/** @brief SPIS shortcuts. */
typedef enum
{
    NRF_SPIS_SHORT_END_ACQUIRE = SPIS_SHORTS_END_ACQUIRE_Msk ///< Shortcut between END event and ACQUIRE task.
} nrf_spis_short_mask_t;

/** @brief SPIS interrupts. */
typedef enum
{
    NRF_SPIS_INT_END_MASK      = SPIS_INTENSET_END_Msk,     ///< Interrupt on END event.
    NRF_SPIS_INT_ACQUIRED_MASK = SPIS_INTENSET_ACQUIRED_Msk ///< Interrupt on ACQUIRED event.
} nrf_spis_int_mask_t;

/** @brief SPI modes. */
typedef enum
{
    NRF_SPIS_MODE_0, ///< SCK active high, sample on leading edge of clock.
    NRF_SPIS_MODE_1, ///< SCK active high, sample on trailing edge of clock.
    NRF_SPIS_MODE_2, ///< SCK active low, sample on leading edge of clock.
    NRF_SPIS_MODE_3  ///< SCK active low, sample on trailing edge of clock.
} nrf_spis_mode_t;

/** @brief SPI bit orders. */
typedef enum
{
    NRF_SPIS_BIT_ORDER_MSB_FIRST = SPIS_CONFIG_ORDER_MsbFirst, ///< Most significant bit shifted out first.
    NRF_SPIS_BIT_ORDER_LSB_FIRST = SPIS_CONFIG_ORDER_LsbFirst  ///< Least significant bit shifted out first.
} nrf_spis_bit_order_t;

/** @brief SPI semaphore status. */
typedef enum
{
    NRF_SPIS_SEMSTAT_FREE       = 0, ///< Semaphore is free.
    NRF_SPIS_SEMSTAT_CPU        = 1, ///< Semaphore is assigned to the CPU.
    NRF_SPIS_SEMSTAT_SPIS       = 2, ///< Semaphore is assigned to the SPI slave.
    NRF_SPIS_SEMSTAT_CPUPENDING = 3  ///< Semaphore is assigned to the SPI, but a handover to the CPU is pending.
} nrf_spis_semstat_t;

/** @brief SPIS status. */
typedef enum
{
    NRF_SPIS_STATUS_OVERREAD = SPIS_STATUS_OVERREAD_Msk, ///< TX buffer over-read detected and prevented.
    NRF_SPIS_STATUS_OVERFLOW = SPIS_STATUS_OVERFLOW_Msk  ///< RX buffer overflow detected and prevented.
} nrf_spis_status_mask_t;


/**
 * @brief Function for activating the specified SPIS task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task to be activated.
 */
NRF_STATIC_INLINE void nrf_spis_task_trigger(NRF_SPIS_Type * p_reg,
                                             nrf_spis_task_t task);

/**
 * @brief Function for getting the address of the specified SPIS task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  The specified task.
 *
 * @return Address of the specified task register.
 */
NRF_STATIC_INLINE uint32_t nrf_spis_task_address_get(NRF_SPIS_Type const * p_reg,
                                                     nrf_spis_task_t       task);

/**
 * @brief Function for clearing the specified SPIS event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be cleared.
 */
NRF_STATIC_INLINE void nrf_spis_event_clear(NRF_SPIS_Type *  p_reg,
                                            nrf_spis_event_t event);

/**
 * @brief Function for retrieving the state of the SPIS event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_spis_event_check(NRF_SPIS_Type const * p_reg,
                                            nrf_spis_event_t      event);

/**
 * @brief Function for getting the address of the specified SPIS event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event The specified event.
 *
 * @return Address of the specified event register.
 */
NRF_STATIC_INLINE uint32_t nrf_spis_event_address_get(NRF_SPIS_Type const * p_reg,
                                                      nrf_spis_event_t      event);

/**
 * @brief Function for enabling the specified shortcuts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Shortcuts to be enabled.
 */
NRF_STATIC_INLINE void nrf_spis_shorts_enable(NRF_SPIS_Type * p_reg,
                                              uint32_t        mask);

/**
 * @brief Function for disabling the specified shortcuts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Shortcuts to be disabled.
 */
NRF_STATIC_INLINE void nrf_spis_shorts_disable(NRF_SPIS_Type * p_reg,
                                               uint32_t        mask);

/**
 * @brief Function for enabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
NRF_STATIC_INLINE void nrf_spis_int_enable(NRF_SPIS_Type * p_reg,
                                           uint32_t        mask);

/**
 * @brief Function for disabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
NRF_STATIC_INLINE void nrf_spis_int_disable(NRF_SPIS_Type * p_reg,
                                            uint32_t        mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_spis_int_enable_check(NRF_SPIS_Type const * p_reg, uint32_t mask);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        SPIS task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
NRF_STATIC_INLINE void nrf_spis_subscribe_set(NRF_SPIS_Type * p_reg,
                                              nrf_spis_task_t task,
                                              uint8_t         channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        SPIS task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_spis_subscribe_clear(NRF_SPIS_Type * p_reg,
                                                nrf_spis_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        SPIS event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
NRF_STATIC_INLINE void nrf_spis_publish_set(NRF_SPIS_Type *  p_reg,
                                            nrf_spis_event_t event,
                                            uint8_t          channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        SPIS event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_spis_publish_clear(NRF_SPIS_Type *  p_reg,
                                              nrf_spis_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for enabling the SPIS peripheral.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spis_enable(NRF_SPIS_Type * p_reg);

/**
 * @brief Function for disabling the SPIS peripheral.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spis_disable(NRF_SPIS_Type * p_reg);

/**
 * @brief Function for retrieving the SPIS semaphore status.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @returns Current semaphore status.
 */
NRF_STATIC_INLINE nrf_spis_semstat_t nrf_spis_semaphore_status_get(NRF_SPIS_Type const * p_reg);

/**
 * @brief Function for retrieving the SPIS status.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @returns Current SPIS status.
 */
NRF_STATIC_INLINE nrf_spis_status_mask_t nrf_spis_status_get(NRF_SPIS_Type const * p_reg);

/**
 * @brief Function for configuring SPIS pins.
 *
 * If a given signal is not needed, pass the @ref NRF_SPIS_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] sck_pin  SCK pin number.
 * @param[in] mosi_pin MOSI pin number.
 * @param[in] miso_pin MISO pin number.
 * @param[in] csn_pin  CSN pin number.
 */
NRF_STATIC_INLINE void nrf_spis_pins_set(NRF_SPIS_Type * p_reg,
                                         uint32_t        sck_pin,
                                         uint32_t        mosi_pin,
                                         uint32_t        miso_pin,
                                         uint32_t        csn_pin);

/**
 * @brief Function for getting the SCK pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return SCK pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_spis_sck_pin_get(NRF_SPIS_Type const * p_reg);

/**
 * @brief Function for getting the MOSI pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return MOSI pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_spis_mosi_pin_get(NRF_SPIS_Type const * p_reg);

/**
 * @brief Function for getting the MISO pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return MISO pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_spis_miso_pin_get(NRF_SPIS_Type const * p_reg);

/**
 * @brief Function for getting the CSN pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return CSN pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_spis_csn_pin_get(NRF_SPIS_Type const * p_reg);

/**
 * @brief Function for setting the transmit buffer.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_buffer Pointer to the buffer that contains the data to send.
 * @param[in] length   Maximum number of data bytes to transmit.
 */
NRF_STATIC_INLINE void nrf_spis_tx_buffer_set(NRF_SPIS_Type * p_reg,
                                              uint8_t const * p_buffer,
                                              size_t          length);

/**
 * @brief Function for setting the receive buffer.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_buffer Pointer to the buffer for received data.
 * @param[in] length   Maximum number of data bytes to receive.
 */
NRF_STATIC_INLINE void nrf_spis_rx_buffer_set(NRF_SPIS_Type * p_reg,
                                              uint8_t *       p_buffer,
                                              size_t          length);

/**
 * @brief Function for getting the number of bytes transmitted
 *        in the last granted transaction.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @returns Number of bytes transmitted.
 */
NRF_STATIC_INLINE size_t nrf_spis_tx_amount_get(NRF_SPIS_Type const * p_reg);

/**
 * @brief Function for getting the number of bytes received
 *        in the last granted transaction.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @returns Number of bytes received.
 */
NRF_STATIC_INLINE size_t nrf_spis_rx_amount_get(NRF_SPIS_Type const * p_reg);

/**
 * @brief Function for setting the SPI configuration.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] spi_mode      SPI mode.
 * @param[in] spi_bit_order SPI bit order.
 */
NRF_STATIC_INLINE void nrf_spis_configure(NRF_SPIS_Type *      p_reg,
                                          nrf_spis_mode_t      spi_mode,
                                          nrf_spis_bit_order_t spi_bit_order);

/**
 * @brief Function for setting the default character.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] def   Default character that is clocked out in case of
 *                  an overflow of the RXD buffer.
 */
NRF_STATIC_INLINE void nrf_spis_def_set(NRF_SPIS_Type * p_reg,
                                        uint8_t         def);

/**
 * @brief Function for setting the over-read character.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] orc   Over-read character that is clocked out in case of
 *                  an over-read of the TXD buffer.
 */
NRF_STATIC_INLINE void nrf_spis_orc_set(NRF_SPIS_Type * p_reg,
                                        uint8_t         orc);

#if defined(SPIS_TXD_LIST_LIST_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for enabling the TX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spis_tx_list_enable(NRF_SPIS_Type * p_reg);

/**
 * @brief Function for disabling the TX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spis_tx_list_disable(NRF_SPIS_Type * p_reg);
#endif // defined(SPIS_TXD_LIST_LIST_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(SPIS_RXD_LIST_LIST_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for enabling the RX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spis_rx_list_enable(NRF_SPIS_Type * p_reg);

/**
 * @brief Function for disabling the RX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spis_rx_list_disable(NRF_SPIS_Type * p_reg);
#endif // defined(SPIS_RXD_LIST_LIST_Msk) || defined(__NRFX_DOXYGEN__)

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_spis_task_trigger(NRF_SPIS_Type * p_reg,
                                             nrf_spis_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

NRF_STATIC_INLINE uint32_t nrf_spis_task_address_get(NRF_SPIS_Type const * p_reg,
                                                     nrf_spis_task_t       task)
{
    return (uint32_t)p_reg + (uint32_t)task;
}

NRF_STATIC_INLINE void nrf_spis_event_clear(NRF_SPIS_Type *  p_reg,
                                            nrf_spis_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
    nrf_event_readback((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE bool nrf_spis_event_check(NRF_SPIS_Type const * p_reg,
                                            nrf_spis_event_t      event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE uint32_t nrf_spis_event_address_get(NRF_SPIS_Type const * p_reg,
                                                      nrf_spis_event_t      event)
{
    return (uint32_t)p_reg + (uint32_t)event;
}

NRF_STATIC_INLINE void nrf_spis_shorts_enable(NRF_SPIS_Type * p_reg,
                                              uint32_t        mask)
{
    p_reg->SHORTS |= mask;
}

NRF_STATIC_INLINE void nrf_spis_shorts_disable(NRF_SPIS_Type * p_reg,
                                               uint32_t        mask)
{
    p_reg->SHORTS &= ~(mask);
}

NRF_STATIC_INLINE void nrf_spis_int_enable(NRF_SPIS_Type * p_reg,
                                           uint32_t        mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE void nrf_spis_int_disable(NRF_SPIS_Type * p_reg,
                                            uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

NRF_STATIC_INLINE uint32_t nrf_spis_int_enable_check(NRF_SPIS_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

#if defined(DPPI_PRESENT)
NRF_STATIC_INLINE void nrf_spis_subscribe_set(NRF_SPIS_Type * p_reg,
                                              nrf_spis_task_t task,
                                              uint8_t         channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | SPIS_SUBSCRIBE_ACQUIRE_EN_Msk);
}

NRF_STATIC_INLINE void nrf_spis_subscribe_clear(NRF_SPIS_Type * p_reg,
                                                nrf_spis_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

NRF_STATIC_INLINE void nrf_spis_publish_set(NRF_SPIS_Type *  p_reg,
                                            nrf_spis_event_t event,
                                            uint8_t          channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | SPIS_PUBLISH_END_EN_Msk);
}

NRF_STATIC_INLINE void nrf_spis_publish_clear(NRF_SPIS_Type *  p_reg,
                                              nrf_spis_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

NRF_STATIC_INLINE void nrf_spis_enable(NRF_SPIS_Type * p_reg)
{
    p_reg->ENABLE = (SPIS_ENABLE_ENABLE_Enabled << SPIS_ENABLE_ENABLE_Pos);
}

NRF_STATIC_INLINE void nrf_spis_disable(NRF_SPIS_Type * p_reg)
{
    p_reg->ENABLE = (SPIS_ENABLE_ENABLE_Disabled << SPIS_ENABLE_ENABLE_Pos);
}

NRF_STATIC_INLINE nrf_spis_semstat_t nrf_spis_semaphore_status_get(NRF_SPIS_Type const * p_reg)
{
    return (nrf_spis_semstat_t) ((p_reg->SEMSTAT & SPIS_SEMSTAT_SEMSTAT_Msk)
                                 >> SPIS_SEMSTAT_SEMSTAT_Pos);
}

NRF_STATIC_INLINE nrf_spis_status_mask_t nrf_spis_status_get(NRF_SPIS_Type const * p_reg)
{
    return (nrf_spis_status_mask_t) p_reg->STATUS;
}

NRF_STATIC_INLINE void nrf_spis_pins_set(NRF_SPIS_Type * p_reg,
                                         uint32_t        sck_pin,
                                         uint32_t        mosi_pin,
                                         uint32_t        miso_pin,
                                         uint32_t        csn_pin)
{
#if defined (NRF51)
    p_reg->PSELSCK  = sck_pin;
    p_reg->PSELMOSI = mosi_pin;
    p_reg->PSELMISO = miso_pin;
    p_reg->PSELCSN  = csn_pin;
#else
    p_reg->PSEL.SCK  = sck_pin;
    p_reg->PSEL.MOSI = mosi_pin;
    p_reg->PSEL.MISO = miso_pin;
    p_reg->PSEL.CSN  = csn_pin;
#endif
}

NRF_STATIC_INLINE uint32_t nrf_spis_sck_pin_get(NRF_SPIS_Type const * p_reg)
{
#if defined (NRF51)
    return p_reg->PSELSCK;
#else
    return p_reg->PSEL.SCK;
#endif
}

NRF_STATIC_INLINE uint32_t nrf_spis_mosi_pin_get(NRF_SPIS_Type const * p_reg)
{
#if defined (NRF51)
    return p_reg->PSELMOSI;
#else
    return p_reg->PSEL.MOSI;
#endif
}

NRF_STATIC_INLINE uint32_t nrf_spis_miso_pin_get(NRF_SPIS_Type const * p_reg)
{
#if defined (NRF51)
    return p_reg->PSELMISO;
#else
    return p_reg->PSEL.MISO;
#endif
}

NRF_STATIC_INLINE uint32_t nrf_spis_csn_pin_get(NRF_SPIS_Type const * p_reg)
{
#if defined (NRF51)
    return p_reg->PSELCSN;
#else
    return p_reg->PSEL.CSN;
#endif
}

NRF_STATIC_INLINE void nrf_spis_tx_buffer_set(NRF_SPIS_Type * p_reg,
                                              uint8_t const * p_buffer,
                                              size_t          length)
{
#if defined (NRF51)
    p_reg->TXDPTR = (uint32_t)p_buffer;
    p_reg->MAXTX  = length;
#else
    p_reg->TXD.PTR    = (uint32_t)p_buffer;
    p_reg->TXD.MAXCNT = length;
#endif
}

NRF_STATIC_INLINE void nrf_spis_rx_buffer_set(NRF_SPIS_Type * p_reg,
                                              uint8_t *       p_buffer,
                                              size_t          length)
{
#if defined (NRF51)
    p_reg->RXDPTR = (uint32_t)p_buffer;
    p_reg->MAXRX  = length;
#else
    p_reg->RXD.PTR    = (uint32_t)p_buffer;
    p_reg->RXD.MAXCNT = length;
#endif
}

NRF_STATIC_INLINE size_t nrf_spis_tx_amount_get(NRF_SPIS_Type const * p_reg)
{
#if defined (NRF51)
    return p_reg->AMOUNTTX;
#else
    return p_reg->TXD.AMOUNT;
#endif
}

NRF_STATIC_INLINE size_t nrf_spis_rx_amount_get(NRF_SPIS_Type const * p_reg)
{
#if defined (NRF51)
    return p_reg->AMOUNTRX;
#else
    return p_reg->RXD.AMOUNT;
#endif
}

NRF_STATIC_INLINE void nrf_spis_configure(NRF_SPIS_Type *      p_reg,
                                          nrf_spis_mode_t      spi_mode,
                                          nrf_spis_bit_order_t spi_bit_order)
{
    uint32_t config = (spi_bit_order == NRF_SPIS_BIT_ORDER_MSB_FIRST ?
        SPIS_CONFIG_ORDER_MsbFirst : SPIS_CONFIG_ORDER_LsbFirst);

    switch (spi_mode)
    {
    default:
    case NRF_SPIS_MODE_0:
        config |= (SPIS_CONFIG_CPOL_ActiveHigh << SPIS_CONFIG_CPOL_Pos) |
                  (SPIS_CONFIG_CPHA_Leading    << SPIS_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIS_MODE_1:
        config |= (SPIS_CONFIG_CPOL_ActiveHigh << SPIS_CONFIG_CPOL_Pos) |
                  (SPIS_CONFIG_CPHA_Trailing   << SPIS_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIS_MODE_2:
        config |= (SPIS_CONFIG_CPOL_ActiveLow  << SPIS_CONFIG_CPOL_Pos) |
                  (SPIS_CONFIG_CPHA_Leading    << SPIS_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIS_MODE_3:
        config |= (SPIS_CONFIG_CPOL_ActiveLow  << SPIS_CONFIG_CPOL_Pos) |
                  (SPIS_CONFIG_CPHA_Trailing   << SPIS_CONFIG_CPHA_Pos);
        break;
    }
    p_reg->CONFIG = config;
}

NRF_STATIC_INLINE void nrf_spis_orc_set(NRF_SPIS_Type * p_reg,
                                        uint8_t         orc)
{
    p_reg->ORC = orc;
}

NRF_STATIC_INLINE void nrf_spis_def_set(NRF_SPIS_Type * p_reg,
                                        uint8_t         def)
{
    p_reg->DEF = def;
}

#if defined(SPIS_TXD_LIST_LIST_Msk)
NRF_STATIC_INLINE void nrf_spis_tx_list_enable(NRF_SPIS_Type * p_reg)
{
    p_reg->TXD.LIST = SPIS_TXD_LIST_LIST_ArrayList << SPIS_TXD_LIST_LIST_Pos;
}

NRF_STATIC_INLINE void nrf_spis_tx_list_disable(NRF_SPIS_Type * p_reg)
{
    p_reg->TXD.LIST = SPIS_TXD_LIST_LIST_Disabled << SPIS_TXD_LIST_LIST_Pos;
}
#endif // defined(SPIS_TXD_LIST_LIST_Msk)

#if defined(SPIS_RXD_LIST_LIST_Msk)
NRF_STATIC_INLINE void nrf_spis_rx_list_enable(NRF_SPIS_Type * p_reg)
{
    p_reg->RXD.LIST = SPIS_RXD_LIST_LIST_ArrayList << SPIS_RXD_LIST_LIST_Pos;
}

NRF_STATIC_INLINE void nrf_spis_rx_list_disable(NRF_SPIS_Type * p_reg)
{
    p_reg->RXD.LIST = SPIS_RXD_LIST_LIST_Disabled << SPIS_RXD_LIST_LIST_Pos;
}
#endif // defined(SPIS_RXD_LIST_LIST_Msk)

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_SPIS_H__
