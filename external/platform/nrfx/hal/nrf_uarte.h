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

#ifndef NRF_UARTE_H__
#define NRF_UARTE_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NRF_UARTE_PSEL_DISCONNECTED 0xFFFFFFFF

/**
 * @defgroup nrf_uarte_hal UARTE HAL
 * @{
 * @ingroup nrf_uarte
 * @brief   Hardware access layer for managing the UARTE peripheral.
 */

/** @brief UARTE tasks. */
typedef enum
{
    NRF_UARTE_TASK_STARTRX   = offsetof(NRF_UARTE_Type, TASKS_STARTRX), ///< Start UART receiver.
    NRF_UARTE_TASK_STOPRX    = offsetof(NRF_UARTE_Type, TASKS_STOPRX),  ///< Stop UART receiver.
    NRF_UARTE_TASK_STARTTX   = offsetof(NRF_UARTE_Type, TASKS_STARTTX), ///< Start UART transmitter.
    NRF_UARTE_TASK_STOPTX    = offsetof(NRF_UARTE_Type, TASKS_STOPTX),  ///< Stop UART transmitter.
    NRF_UARTE_TASK_FLUSHRX   = offsetof(NRF_UARTE_Type, TASKS_FLUSHRX)  ///< Flush RX FIFO in RX buffer.
} nrf_uarte_task_t;

/** @brief UARTE events. */
typedef enum
{
    NRF_UARTE_EVENT_CTS       = offsetof(NRF_UARTE_Type, EVENTS_CTS),       ///< CTS is activated.
    NRF_UARTE_EVENT_NCTS      = offsetof(NRF_UARTE_Type, EVENTS_NCTS),      ///< CTS is deactivated.
    NRF_UARTE_EVENT_RXDRDY    = offsetof(NRF_UARTE_Type, EVENTS_RXDRDY),    ///< Data received in RXD (but potentially not yet transferred to Data RAM).
    NRF_UARTE_EVENT_ENDRX     = offsetof(NRF_UARTE_Type, EVENTS_ENDRX),     ///< Receive buffer is filled up.
    NRF_UARTE_EVENT_TXDRDY    = offsetof(NRF_UARTE_Type, EVENTS_TXDRDY),    ///< Data sent from TXD.
    NRF_UARTE_EVENT_ENDTX     = offsetof(NRF_UARTE_Type, EVENTS_ENDTX),     ///< Last TX byte transmitted.
    NRF_UARTE_EVENT_ERROR     = offsetof(NRF_UARTE_Type, EVENTS_ERROR),     ///< Error detected.
    NRF_UARTE_EVENT_RXTO      = offsetof(NRF_UARTE_Type, EVENTS_RXTO),      ///< Receiver timeout.
    NRF_UARTE_EVENT_RXSTARTED = offsetof(NRF_UARTE_Type, EVENTS_RXSTARTED), ///< Receiver has started.
    NRF_UARTE_EVENT_TXSTARTED = offsetof(NRF_UARTE_Type, EVENTS_TXSTARTED), ///< Transmitter has started.
    NRF_UARTE_EVENT_TXSTOPPED = offsetof(NRF_UARTE_Type, EVENTS_TXSTOPPED)  ///< Transmitted stopped.
} nrf_uarte_event_t;

/** @brief Types of UARTE shortcuts. */
typedef enum
{
    NRF_UARTE_SHORT_ENDRX_STARTRX = UARTE_SHORTS_ENDRX_STARTRX_Msk, ///< Shortcut between ENDRX event and STARTRX task.
    NRF_UARTE_SHORT_ENDRX_STOPRX  = UARTE_SHORTS_ENDRX_STOPRX_Msk   ///< Shortcut between ENDRX event and STOPRX task.
} nrf_uarte_short_t;


/** @brief UARTE interrupts. */
typedef enum
{
    NRF_UARTE_INT_CTS_MASK       = UARTE_INTENSET_CTS_Msk,       ///< Interrupt on CTS event.
    NRF_UARTE_INT_NCTS_MASK      = UARTE_INTENSET_NCTS_Msk,      ///< Interrupt on NCTS event.
    NRF_UARTE_INT_RXDRDY_MASK    = UARTE_INTENSET_RXDRDY_Msk,    ///< Interrupt on RXDRDY event.
    NRF_UARTE_INT_ENDRX_MASK     = UARTE_INTENSET_ENDRX_Msk,     ///< Interrupt on ENDRX event.
    NRF_UARTE_INT_TXDRDY_MASK    = UARTE_INTENSET_TXDRDY_Msk,    ///< Interrupt on TXDRDY event.
    NRF_UARTE_INT_ENDTX_MASK     = UARTE_INTENSET_ENDTX_Msk,     ///< Interrupt on ENDTX event.
    NRF_UARTE_INT_ERROR_MASK     = UARTE_INTENSET_ERROR_Msk,     ///< Interrupt on ERROR event.
    NRF_UARTE_INT_RXTO_MASK      = UARTE_INTENSET_RXTO_Msk,      ///< Interrupt on RXTO event.
    NRF_UARTE_INT_RXSTARTED_MASK = UARTE_INTENSET_RXSTARTED_Msk, ///< Interrupt on RXSTARTED event.
    NRF_UARTE_INT_TXSTARTED_MASK = UARTE_INTENSET_TXSTARTED_Msk, ///< Interrupt on TXSTARTED event.
    NRF_UARTE_INT_TXSTOPPED_MASK = UARTE_INTENSET_TXSTOPPED_Msk  ///< Interrupt on TXSTOPPED event.
} nrf_uarte_int_mask_t;

/** @brief Baudrates supported by UARTE. */
typedef enum
{
    NRF_UARTE_BAUDRATE_1200    = UARTE_BAUDRATE_BAUDRATE_Baud1200,   ///< 1200 baud.
    NRF_UARTE_BAUDRATE_2400    = UARTE_BAUDRATE_BAUDRATE_Baud2400,   ///< 2400 baud.
    NRF_UARTE_BAUDRATE_4800    = UARTE_BAUDRATE_BAUDRATE_Baud4800,   ///< 4800 baud.
    NRF_UARTE_BAUDRATE_9600    = UARTE_BAUDRATE_BAUDRATE_Baud9600,   ///< 9600 baud.
    NRF_UARTE_BAUDRATE_14400   = UARTE_BAUDRATE_BAUDRATE_Baud14400,  ///< 14400 baud.
    NRF_UARTE_BAUDRATE_19200   = UARTE_BAUDRATE_BAUDRATE_Baud19200,  ///< 19200 baud.
    NRF_UARTE_BAUDRATE_28800   = UARTE_BAUDRATE_BAUDRATE_Baud28800,  ///< 28800 baud.
    NRF_UARTE_BAUDRATE_31250   = UARTE_BAUDRATE_BAUDRATE_Baud31250,  ///< 31250 baud.
    NRF_UARTE_BAUDRATE_38400   = UARTE_BAUDRATE_BAUDRATE_Baud38400,  ///< 38400 baud.
    NRF_UARTE_BAUDRATE_56000   = UARTE_BAUDRATE_BAUDRATE_Baud56000,  ///< 56000 baud.
    NRF_UARTE_BAUDRATE_57600   = UARTE_BAUDRATE_BAUDRATE_Baud57600,  ///< 57600 baud.
    NRF_UARTE_BAUDRATE_76800   = UARTE_BAUDRATE_BAUDRATE_Baud76800,  ///< 76800 baud.
    NRF_UARTE_BAUDRATE_115200  = UARTE_BAUDRATE_BAUDRATE_Baud115200, ///< 115200 baud.
    NRF_UARTE_BAUDRATE_230400  = UARTE_BAUDRATE_BAUDRATE_Baud230400, ///< 230400 baud.
    NRF_UARTE_BAUDRATE_250000  = UARTE_BAUDRATE_BAUDRATE_Baud250000, ///< 250000 baud.
    NRF_UARTE_BAUDRATE_460800  = UARTE_BAUDRATE_BAUDRATE_Baud460800, ///< 460800 baud.
    NRF_UARTE_BAUDRATE_921600  = UARTE_BAUDRATE_BAUDRATE_Baud921600, ///< 921600 baud.
    NRF_UARTE_BAUDRATE_1000000 = UARTE_BAUDRATE_BAUDRATE_Baud1M      ///< 1000000 baud.
} nrf_uarte_baudrate_t;

/** @brief Types of UARTE error masks. */
typedef enum
{
    NRF_UARTE_ERROR_OVERRUN_MASK = UARTE_ERRORSRC_OVERRUN_Msk, ///< Overrun error.
    NRF_UARTE_ERROR_PARITY_MASK  = UARTE_ERRORSRC_PARITY_Msk,  ///< Parity error.
    NRF_UARTE_ERROR_FRAMING_MASK = UARTE_ERRORSRC_FRAMING_Msk, ///< Framing error.
    NRF_UARTE_ERROR_BREAK_MASK   = UARTE_ERRORSRC_BREAK_Msk    ///< Break error.
} nrf_uarte_error_mask_t;

/** @brief Types of UARTE parity modes. */
typedef enum
{
    NRF_UARTE_PARITY_EXCLUDED = UARTE_CONFIG_PARITY_Excluded << UARTE_CONFIG_PARITY_Pos, ///< Parity excluded.
    NRF_UARTE_PARITY_INCLUDED = UARTE_CONFIG_PARITY_Included << UARTE_CONFIG_PARITY_Pos  ///< Parity included.
} nrf_uarte_parity_t;

/** @brief Types of UARTE flow control modes. */
typedef enum
{
    NRF_UARTE_HWFC_DISABLED = UARTE_CONFIG_HWFC_Disabled << UARTE_CONFIG_HWFC_Pos, ///< Hardware flow control disabled.
    NRF_UARTE_HWFC_ENABLED  = UARTE_CONFIG_HWFC_Enabled  << UARTE_CONFIG_HWFC_Pos  ///< Hardware flow control enabled.
} nrf_uarte_hwfc_t;

#if defined(UARTE_CONFIG_STOP_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Types of UARTE stop bit modes. */
typedef enum
{
    NRF_UARTE_STOP_ONE = UARTE_CONFIG_STOP_One << UARTE_CONFIG_STOP_Pos, ///< One stop bit.
    NRF_UARTE_STOP_TWO = UARTE_CONFIG_STOP_Two << UARTE_CONFIG_STOP_Pos  ///< Two stop bits.
} nrf_uarte_stop_t;
#endif

#if defined(UARTE_CONFIG_PARITYTYPE_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Types of UARTE parity types. */
typedef enum
{
    NRF_UARTE_PARITYTYPE_EVEN = UARTE_CONFIG_PARITYTYPE_Even << UARTE_CONFIG_PARITYTYPE_Pos, ///< Parity even.
    NRF_UARTE_PARITYTYPE_ODD  = UARTE_CONFIG_PARITYTYPE_Odd << UARTE_CONFIG_PARITYTYPE_Pos,  ///< Parity odd.
} nrf_uarte_paritytype_t;
#endif

/** @brief Structure for UARTE transmission configuration. */
typedef struct
{
    nrf_uarte_hwfc_t       hwfc;       ///< Flow control configuration.
    nrf_uarte_parity_t     parity;     ///< Parity configuration.
#if defined(UARTE_CONFIG_STOP_Msk) || defined(__NRFX_DOXYGEN__)
    nrf_uarte_stop_t       stop;       ///< Stop bits.
#endif
#if defined(UARTE_CONFIG_PARITYTYPE_Msk) || defined(__NRFX_DOXYGEN__)
    nrf_uarte_paritytype_t paritytype; ///< Parity type.
#endif
} nrf_uarte_config_t;

/**
 * @brief Function for clearing a specific UARTE event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to clear.
 */
NRF_STATIC_INLINE void nrf_uarte_event_clear(NRF_UARTE_Type * p_reg, nrf_uarte_event_t event);

/**
 * @brief Function for retrieving the state of the UARTE event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_uarte_event_check(NRF_UARTE_Type const * p_reg,
                                             nrf_uarte_event_t      event);

/**
 * @brief Function for returning the address of the specified UARTE event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event The specified event.
 *
 * @return Address of specified event register.
 */
NRF_STATIC_INLINE uint32_t nrf_uarte_event_address_get(NRF_UARTE_Type const * p_reg,
                                                       nrf_uarte_event_t      event);

/**
 * @brief Function for enabling UARTE shortcuts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Shortcuts to be enabled.
 */
NRF_STATIC_INLINE void nrf_uarte_shorts_enable(NRF_UARTE_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling UARTE shortcuts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Shortcuts to be disabled.
 */
NRF_STATIC_INLINE void nrf_uarte_shorts_disable(NRF_UARTE_Type * p_reg, uint32_t mask);

/**
 * @brief Function for enabling UARTE interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
NRF_STATIC_INLINE void nrf_uarte_int_enable(NRF_UARTE_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_uarte_int_enable_check(NRF_UARTE_Type const * p_reg, uint32_t mask);

/**
 * @brief Function for disabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
NRF_STATIC_INLINE void nrf_uarte_int_disable(NRF_UARTE_Type * p_reg, uint32_t mask);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        UARTE task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
NRF_STATIC_INLINE void nrf_uarte_subscribe_set(NRF_UARTE_Type * p_reg,
                                               nrf_uarte_task_t task,
                                               uint8_t          channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        UARTE task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_uarte_subscribe_clear(NRF_UARTE_Type * p_reg,
                                                 nrf_uarte_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        UARTE event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
NRF_STATIC_INLINE void nrf_uarte_publish_set(NRF_UARTE_Type *  p_reg,
                                             nrf_uarte_event_t event,
                                             uint8_t           channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        UARTE event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_uarte_publish_clear(NRF_UARTE_Type *  p_reg,
                                               nrf_uarte_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for getting error source mask. Function is clearing error source flags after reading.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Mask with error source flags.
 */
NRF_STATIC_INLINE uint32_t nrf_uarte_errorsrc_get_and_clear(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for enabling UARTE.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_uarte_enable(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for disabling UARTE.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_uarte_disable(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for configuring TX/RX pins.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] pseltxd TXD pin number.
 * @param[in] pselrxd RXD pin number.
 */
NRF_STATIC_INLINE void nrf_uarte_txrx_pins_set(NRF_UARTE_Type * p_reg,
                                               uint32_t         pseltxd,
                                               uint32_t         pselrxd);

/**
 * @brief Function for disconnecting TX/RX pins.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_uarte_txrx_pins_disconnect(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for getting TX pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return TX pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_uarte_tx_pin_get(NRF_UARTE_Type const * p_reg);

/**
 * @brief Function for getting RX pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return RX pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_uarte_rx_pin_get(NRF_UARTE_Type const * p_reg);

/**
 * @brief Function for getting RTS pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return RTS pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_uarte_rts_pin_get(NRF_UARTE_Type const * p_reg);

/**
 * @brief Function for getting CTS pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return CTS pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_uarte_cts_pin_get(NRF_UARTE_Type const * p_reg);

/**
 * @brief Function for configuring flow control pins.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] pselrts RTS pin number.
 * @param[in] pselcts CTS pin number.
 */
NRF_STATIC_INLINE void nrf_uarte_hwfc_pins_set(NRF_UARTE_Type * p_reg,
                                               uint32_t         pselrts,
                                               uint32_t         pselcts);

/**
 * @brief Function for disconnecting flow control pins.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_uarte_hwfc_pins_disconnect(NRF_UARTE_Type * p_reg);

/**
 * @brief Function for starting an UARTE task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task.
 */
NRF_STATIC_INLINE void nrf_uarte_task_trigger(NRF_UARTE_Type * p_reg, nrf_uarte_task_t task);

/**
 * @brief Function for returning the address of the specified task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task.
 *
 * @return Task address.
 */
NRF_STATIC_INLINE uint32_t nrf_uarte_task_address_get(NRF_UARTE_Type const * p_reg,
                                                      nrf_uarte_task_t       task);

/**
 * @brief Function for configuring UARTE.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] p_cfg Pointer to UARTE settings structure.
 */
NRF_STATIC_INLINE void nrf_uarte_configure(NRF_UARTE_Type           * p_reg,
                                           nrf_uarte_config_t const * p_cfg);

/**
 * @brief Function for setting UARTE baud rate.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] baudrate Baud rate.
 */
NRF_STATIC_INLINE void nrf_uarte_baudrate_set(NRF_UARTE_Type *     p_reg,
                                              nrf_uarte_baudrate_t baudrate);

/**
 * @brief Function for setting the transmit buffer.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_buffer Pointer to the buffer with data to send.
 * @param[in] length   Maximum number of data bytes to transmit.
 */
NRF_STATIC_INLINE void nrf_uarte_tx_buffer_set(NRF_UARTE_Type * p_reg,
                                               uint8_t  const * p_buffer,
                                               size_t           length);

/**
 * @brief Function for getting number of bytes transmitted in the last transaction.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval Amount of bytes transmitted.
 */
NRF_STATIC_INLINE uint32_t nrf_uarte_tx_amount_get(NRF_UARTE_Type const * p_reg);

/**
 * @brief Function for setting the receive buffer.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_buffer Pointer to the buffer for received data.
 * @param[in] length   Maximum number of data bytes to receive.
 */
NRF_STATIC_INLINE void nrf_uarte_rx_buffer_set(NRF_UARTE_Type * p_reg,
                                               uint8_t *        p_buffer,
                                               size_t           length);

/**
 * @brief Function for getting number of bytes received in the last transaction.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval Amount of bytes received.
 */
NRF_STATIC_INLINE uint32_t nrf_uarte_rx_amount_get(NRF_UARTE_Type const * p_reg);

#ifndef NRF_DECLARE_ONLY
NRF_STATIC_INLINE void nrf_uarte_event_clear(NRF_UARTE_Type * p_reg, nrf_uarte_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
    nrf_event_readback((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE bool nrf_uarte_event_check(NRF_UARTE_Type const * p_reg,
                                             nrf_uarte_event_t      event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE uint32_t nrf_uarte_event_address_get(NRF_UARTE_Type const * p_reg,
                                                       nrf_uarte_event_t      event)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_uarte_shorts_enable(NRF_UARTE_Type * p_reg, uint32_t mask)
{
    p_reg->SHORTS |= mask;
}

NRF_STATIC_INLINE void nrf_uarte_shorts_disable(NRF_UARTE_Type * p_reg, uint32_t mask)
{
    p_reg->SHORTS &= ~(mask);
}

NRF_STATIC_INLINE void nrf_uarte_int_enable(NRF_UARTE_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE uint32_t nrf_uarte_int_enable_check(NRF_UARTE_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

NRF_STATIC_INLINE void nrf_uarte_int_disable(NRF_UARTE_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

#if defined(DPPI_PRESENT)
NRF_STATIC_INLINE void nrf_uarte_subscribe_set(NRF_UARTE_Type * p_reg,
                                               nrf_uarte_task_t task,
                                               uint8_t          channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | UARTE_SUBSCRIBE_STARTRX_EN_Msk);
}

NRF_STATIC_INLINE void nrf_uarte_subscribe_clear(NRF_UARTE_Type * p_reg,
                                                 nrf_uarte_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

NRF_STATIC_INLINE void nrf_uarte_publish_set(NRF_UARTE_Type *  p_reg,
                                             nrf_uarte_event_t event,
                                             uint8_t           channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | UARTE_PUBLISH_CTS_EN_Msk);
}

NRF_STATIC_INLINE void nrf_uarte_publish_clear(NRF_UARTE_Type *  p_reg,
                                               nrf_uarte_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

NRF_STATIC_INLINE uint32_t nrf_uarte_errorsrc_get_and_clear(NRF_UARTE_Type * p_reg)
{
    uint32_t errsrc_mask = p_reg->ERRORSRC;
    p_reg->ERRORSRC = errsrc_mask;
    return errsrc_mask;
}

NRF_STATIC_INLINE void nrf_uarte_enable(NRF_UARTE_Type * p_reg)
{
    p_reg->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
}

NRF_STATIC_INLINE void nrf_uarte_disable(NRF_UARTE_Type * p_reg)
{
    p_reg->ENABLE = UARTE_ENABLE_ENABLE_Disabled;
}

NRF_STATIC_INLINE void nrf_uarte_txrx_pins_set(NRF_UARTE_Type * p_reg,
                                               uint32_t         pseltxd,
                                               uint32_t         pselrxd)
{
    p_reg->PSEL.TXD = pseltxd;
    p_reg->PSEL.RXD = pselrxd;
}

NRF_STATIC_INLINE void nrf_uarte_txrx_pins_disconnect(NRF_UARTE_Type * p_reg)
{
    nrf_uarte_txrx_pins_set(p_reg, NRF_UARTE_PSEL_DISCONNECTED, NRF_UARTE_PSEL_DISCONNECTED);
}

NRF_STATIC_INLINE uint32_t nrf_uarte_tx_pin_get(NRF_UARTE_Type const * p_reg)
{
    return p_reg->PSEL.TXD;
}

NRF_STATIC_INLINE uint32_t nrf_uarte_rx_pin_get(NRF_UARTE_Type const * p_reg)
{
    return p_reg->PSEL.RXD;
}

NRF_STATIC_INLINE uint32_t nrf_uarte_rts_pin_get(NRF_UARTE_Type const * p_reg)
{
    return p_reg->PSEL.RTS;
}

NRF_STATIC_INLINE uint32_t nrf_uarte_cts_pin_get(NRF_UARTE_Type const * p_reg)
{
    return p_reg->PSEL.CTS;
}

NRF_STATIC_INLINE void nrf_uarte_hwfc_pins_set(NRF_UARTE_Type * p_reg,
                                               uint32_t         pselrts,
                                               uint32_t         pselcts)
{
    p_reg->PSEL.RTS = pselrts;
    p_reg->PSEL.CTS = pselcts;
}

NRF_STATIC_INLINE void nrf_uarte_hwfc_pins_disconnect(NRF_UARTE_Type * p_reg)
{
    nrf_uarte_hwfc_pins_set(p_reg, NRF_UARTE_PSEL_DISCONNECTED, NRF_UARTE_PSEL_DISCONNECTED);
}

NRF_STATIC_INLINE void nrf_uarte_task_trigger(NRF_UARTE_Type * p_reg, nrf_uarte_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

NRF_STATIC_INLINE uint32_t nrf_uarte_task_address_get(NRF_UARTE_Type const * p_reg,
                                                      nrf_uarte_task_t       task)
{
    return (uint32_t)p_reg + (uint32_t)task;
}

NRF_STATIC_INLINE void nrf_uarte_configure(NRF_UARTE_Type           * p_reg,
                                           nrf_uarte_config_t const * p_cfg)
{
    p_reg->CONFIG = (uint32_t)p_cfg->parity
#if defined(UARTE_CONFIG_STOP_Msk)
                    | (uint32_t)p_cfg->stop
#endif
#if defined(UARTE_CONFIG_PARITYTYPE_Msk)
                    | (uint32_t)p_cfg->paritytype
#endif
                    | (uint32_t)p_cfg->hwfc;
}

NRF_STATIC_INLINE void nrf_uarte_baudrate_set(NRF_UARTE_Type * p_reg, nrf_uarte_baudrate_t baudrate)
{
    p_reg->BAUDRATE = baudrate;
}

NRF_STATIC_INLINE void nrf_uarte_tx_buffer_set(NRF_UARTE_Type * p_reg,
                                               uint8_t  const * p_buffer,
                                               size_t           length)
{
    p_reg->TXD.PTR    = (uint32_t)p_buffer;
    p_reg->TXD.MAXCNT = length;
}

NRF_STATIC_INLINE uint32_t nrf_uarte_tx_amount_get(NRF_UARTE_Type const * p_reg)
{
    return p_reg->TXD.AMOUNT;
}

NRF_STATIC_INLINE void nrf_uarte_rx_buffer_set(NRF_UARTE_Type * p_reg,
                                               uint8_t *        p_buffer,
                                               size_t           length)
{
    p_reg->RXD.PTR    = (uint32_t)p_buffer;
    p_reg->RXD.MAXCNT = length;
}

NRF_STATIC_INLINE uint32_t nrf_uarte_rx_amount_get(NRF_UARTE_Type const * p_reg)
{
    return p_reg->RXD.AMOUNT;
}
#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_UARTE_H__
