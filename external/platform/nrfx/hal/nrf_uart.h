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

#ifndef NRF_UART_H__
#define NRF_UART_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_uart_hal UART HAL
 * @{
 * @ingroup nrf_uart
 * @brief   Hardware access layer for managing the UART peripheral.
 */

/** @brief Pin disconnected value. */
#define NRF_UART_PSEL_DISCONNECTED 0xFFFFFFFF

/** @brief UART tasks. */
typedef enum
{
    NRF_UART_TASK_STARTRX = offsetof(NRF_UART_Type, TASKS_STARTRX), /**< Task for starting reception. */
    NRF_UART_TASK_STOPRX  = offsetof(NRF_UART_Type, TASKS_STOPRX),  /**< Task for stopping reception. */
    NRF_UART_TASK_STARTTX = offsetof(NRF_UART_Type, TASKS_STARTTX), /**< Task for starting transmission. */
    NRF_UART_TASK_STOPTX  = offsetof(NRF_UART_Type, TASKS_STOPTX),  /**< Task for stopping transmission. */
    NRF_UART_TASK_SUSPEND = offsetof(NRF_UART_Type, TASKS_SUSPEND), /**< Task for suspending UART. */
} nrf_uart_task_t;

/** @brief UART events. */
typedef enum
{
    NRF_UART_EVENT_CTS    = offsetof(NRF_UART_Type, EVENTS_CTS),   /**< Event from CTS line activation. */
    NRF_UART_EVENT_NCTS   = offsetof(NRF_UART_Type, EVENTS_NCTS),  /**< Event from CTS line deactivation. */
    NRF_UART_EVENT_RXDRDY = offsetof(NRF_UART_Type, EVENTS_RXDRDY),/**< Event from data ready in RXD. */
    NRF_UART_EVENT_TXDRDY = offsetof(NRF_UART_Type, EVENTS_TXDRDY),/**< Event from data sent from TXD. */
    NRF_UART_EVENT_ERROR  = offsetof(NRF_UART_Type, EVENTS_ERROR), /**< Event from error detection. */
    NRF_UART_EVENT_RXTO   = offsetof(NRF_UART_Type, EVENTS_RXTO)   /**< Event from receiver timeout. */
} nrf_uart_event_t;

/** @brief UART interrupts. */
typedef enum
{
    NRF_UART_INT_MASK_CTS    = UART_INTENCLR_CTS_Msk,    /**< CTS line activation interrupt. */
    NRF_UART_INT_MASK_NCTS   = UART_INTENCLR_NCTS_Msk,   /**< CTS line deactivation interrupt. */
    NRF_UART_INT_MASK_RXDRDY = UART_INTENCLR_RXDRDY_Msk, /**< Data ready in RXD interrupt. */
    NRF_UART_INT_MASK_TXDRDY = UART_INTENCLR_TXDRDY_Msk, /**< Data sent from TXD interrupt. */
    NRF_UART_INT_MASK_ERROR  = UART_INTENCLR_ERROR_Msk,  /**< Error detection interrupt. */
    NRF_UART_INT_MASK_RXTO   = UART_INTENCLR_RXTO_Msk    /**< Receiver timeout interrupt. */
} nrf_uart_int_mask_t;

/** @brief Baudrates supported by UART. */
typedef enum
{
    NRF_UART_BAUDRATE_1200    = UART_BAUDRATE_BAUDRATE_Baud1200,   /**< 1200 baud. */
    NRF_UART_BAUDRATE_2400    = UART_BAUDRATE_BAUDRATE_Baud2400,   /**< 2400 baud. */
    NRF_UART_BAUDRATE_4800    = UART_BAUDRATE_BAUDRATE_Baud4800,   /**< 4800 baud. */
    NRF_UART_BAUDRATE_9600    = UART_BAUDRATE_BAUDRATE_Baud9600,   /**< 9600 baud. */
    NRF_UART_BAUDRATE_14400   = UART_BAUDRATE_BAUDRATE_Baud14400,  /**< 14400 baud. */
    NRF_UART_BAUDRATE_19200   = UART_BAUDRATE_BAUDRATE_Baud19200,  /**< 19200 baud. */
    NRF_UART_BAUDRATE_28800   = UART_BAUDRATE_BAUDRATE_Baud28800,  /**< 28800 baud. */
    NRF_UART_BAUDRATE_31250   = UART_BAUDRATE_BAUDRATE_Baud31250,  /**< 31250 baud. */
    NRF_UART_BAUDRATE_38400   = UART_BAUDRATE_BAUDRATE_Baud38400,  /**< 38400 baud. */
    NRF_UART_BAUDRATE_56000   = UART_BAUDRATE_BAUDRATE_Baud56000,  /**< 56000 baud. */
    NRF_UART_BAUDRATE_57600   = UART_BAUDRATE_BAUDRATE_Baud57600,  /**< 57600 baud. */
    NRF_UART_BAUDRATE_76800   = UART_BAUDRATE_BAUDRATE_Baud76800,  /**< 76800 baud. */
    NRF_UART_BAUDRATE_115200  = UART_BAUDRATE_BAUDRATE_Baud115200, /**< 115200 baud. */
    NRF_UART_BAUDRATE_230400  = UART_BAUDRATE_BAUDRATE_Baud230400, /**< 230400 baud. */
    NRF_UART_BAUDRATE_250000  = UART_BAUDRATE_BAUDRATE_Baud250000, /**< 250000 baud. */
    NRF_UART_BAUDRATE_460800  = UART_BAUDRATE_BAUDRATE_Baud460800, /**< 460800 baud. */
    NRF_UART_BAUDRATE_921600  = UART_BAUDRATE_BAUDRATE_Baud921600, /**< 921600 baud. */
    NRF_UART_BAUDRATE_1000000 = UART_BAUDRATE_BAUDRATE_Baud1M,     /**< 1000000 baud. */
} nrf_uart_baudrate_t;

/** @brief Types of UART error masks. */
typedef enum
{
    NRF_UART_ERROR_OVERRUN_MASK = UART_ERRORSRC_OVERRUN_Msk,   /**< Overrun error. */
    NRF_UART_ERROR_PARITY_MASK  = UART_ERRORSRC_PARITY_Msk,    /**< Parity error. */
    NRF_UART_ERROR_FRAMING_MASK = UART_ERRORSRC_FRAMING_Msk,   /**< Framing error. */
    NRF_UART_ERROR_BREAK_MASK   = UART_ERRORSRC_BREAK_Msk,     /**< Break error. */
} nrf_uart_error_mask_t;

/** @brief Types of UART parity modes. */
typedef enum
{
    NRF_UART_PARITY_EXCLUDED = UART_CONFIG_PARITY_Excluded << UART_CONFIG_PARITY_Pos, /**< Parity excluded. */
    NRF_UART_PARITY_INCLUDED = UART_CONFIG_PARITY_Included << UART_CONFIG_PARITY_Pos, /**< Parity included. */
} nrf_uart_parity_t;

/** @brief Types of UART flow control modes. */
typedef enum
{
    NRF_UART_HWFC_DISABLED = UART_CONFIG_HWFC_Disabled, /**< Hardware flow control disabled. */
    NRF_UART_HWFC_ENABLED  = UART_CONFIG_HWFC_Enabled,  /**< Hardware flow control enabled. */
} nrf_uart_hwfc_t;

#if defined(UART_CONFIG_STOP_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Types of UART stop bit modes. */
typedef enum
{
    NRF_UART_STOP_ONE = UART_CONFIG_STOP_One << UART_CONFIG_STOP_Pos, ///< One stop bit.
    NRF_UART_STOP_TWO = UART_CONFIG_STOP_Two << UART_CONFIG_STOP_Pos  ///< Two stop bits.
} nrf_uart_stop_t;
#endif

#if defined(UART_CONFIG_PARITYTYPE_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Types of UART parity types. */
typedef enum
{
    NRF_UART_PARITYTYPE_EVEN = UART_CONFIG_PARITYTYPE_Even << UART_CONFIG_PARITYTYPE_Pos, /**< Parity even. */
    NRF_UART_PARITYTYPE_ODD  = UART_CONFIG_PARITYTYPE_Odd << UART_CONFIG_PARITYTYPE_Pos,  /**< Parity odd. */
} nrf_uart_paritytype_t;
#endif

/** @brief Structure for UART transmission configuration. */
typedef struct
{
    nrf_uart_hwfc_t       hwfc;       ///< Flow control configuration.
    nrf_uart_parity_t     parity;     ///< Parity configuration.
#if defined(UART_CONFIG_STOP_Msk) || defined(__NRFX_DOXYGEN__)
    nrf_uart_stop_t       stop;       ///< Stop bits.
#endif
#if defined(UART_CONFIG_PARITYTYPE_Msk) || defined(__NRFX_DOXYGEN__)
    nrf_uart_paritytype_t paritytype; ///< Parity type.
#endif
} nrf_uart_config_t;

/**
 * @brief Function for clearing the specified UART event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to clear.
 */
NRF_STATIC_INLINE void nrf_uart_event_clear(NRF_UART_Type * p_reg, nrf_uart_event_t event);

/**
 * @brief Function for retrieving the state of the UART event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_uart_event_check(NRF_UART_Type const * p_reg, nrf_uart_event_t event);

/**
 * @brief Function for returning the address of the specified UART event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Desired event.
 *
 * @return Address of the specified event register.
 */
NRF_STATIC_INLINE uint32_t nrf_uart_event_address_get(NRF_UART_Type const * p_reg,
                                                      nrf_uart_event_t      event);

/**
 * @brief Function for enabling the specified interrupt.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
NRF_STATIC_INLINE void nrf_uart_int_enable(NRF_UART_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_uart_int_enable_check(NRF_UART_Type const * p_reg, uint32_t mask);

/**
 * @brief Function for disabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
NRF_STATIC_INLINE void nrf_uart_int_disable(NRF_UART_Type * p_reg, uint32_t mask);

/**
 * @brief Function for getting error source mask. Function is clearing error source flags after reading.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Mask with error source flags.
 */
NRF_STATIC_INLINE uint32_t nrf_uart_errorsrc_get_and_clear(NRF_UART_Type * p_reg);

/**
 * @brief Function for enabling UART.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_uart_enable(NRF_UART_Type * p_reg);

/**
 * @brief Function for disabling UART.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_uart_disable(NRF_UART_Type * p_reg);

/**
 * @brief Function for configuring TX/RX pins.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] pseltxd TXD pin number.
 * @param[in] pselrxd RXD pin number.
 */
NRF_STATIC_INLINE void nrf_uart_txrx_pins_set(NRF_UART_Type * p_reg,
                                              uint32_t        pseltxd,
                                              uint32_t        pselrxd);

/**
 * @brief Function for disconnecting TX/RX pins.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_uart_txrx_pins_disconnect(NRF_UART_Type * p_reg);

/**
 * @brief Function for getting TX pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return TX pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_uart_tx_pin_get(NRF_UART_Type const * p_reg);

/**
 * @brief Function for getting RX pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return RX pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_uart_rx_pin_get(NRF_UART_Type const * p_reg);

/**
 * @brief Function for getting RTS pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return RTS pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_uart_rts_pin_get(NRF_UART_Type const * p_reg);

/**
 * @brief Function for getting CTS pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return CTS pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_uart_cts_pin_get(NRF_UART_Type const * p_reg);

/**
 * @brief Function for configuring flow control pins.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] pselrts RTS pin number.
 * @param[in] pselcts CTS pin number.
 */
NRF_STATIC_INLINE void nrf_uart_hwfc_pins_set(NRF_UART_Type * p_reg,
                                              uint32_t        pselrts,
                                              uint32_t        pselcts);

/**
 * @brief Function for disconnecting flow control pins.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_uart_hwfc_pins_disconnect(NRF_UART_Type * p_reg);

/**
 * @brief Function for reading RX data.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Received byte.
 */
NRF_STATIC_INLINE uint8_t nrf_uart_rxd_get(NRF_UART_Type const * p_reg);

/**
 * @brief Function for setting Tx data.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] txd   Byte.
 */
NRF_STATIC_INLINE void nrf_uart_txd_set(NRF_UART_Type * p_reg, uint8_t txd);

/**
 * @brief Function for starting an UART task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task.
 */
NRF_STATIC_INLINE void nrf_uart_task_trigger(NRF_UART_Type * p_reg, nrf_uart_task_t task);

/**
 * @brief Function for returning the address of the specified task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task.
 *
 * @return Task address.
 */
NRF_STATIC_INLINE uint32_t nrf_uart_task_address_get(NRF_UART_Type const * p_reg,
                                                     nrf_uart_task_t       task);

/**
 * @brief Function for configuring UART.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] p_cfg Pointer to UART settings structure.
 */
NRF_STATIC_INLINE void nrf_uart_configure(NRF_UART_Type           * p_reg,
                                          nrf_uart_config_t const * p_cfg);

/**
 * @brief Function for setting UART baud rate.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] baudrate Baud rate.
 */
NRF_STATIC_INLINE void nrf_uart_baudrate_set(NRF_UART_Type * p_reg, nrf_uart_baudrate_t baudrate);


#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_uart_event_clear(NRF_UART_Type * p_reg, nrf_uart_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
    nrf_event_readback((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE bool nrf_uart_event_check(NRF_UART_Type const * p_reg, nrf_uart_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE uint32_t nrf_uart_event_address_get(NRF_UART_Type const * p_reg,
                                                      nrf_uart_event_t      event)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_uart_int_enable(NRF_UART_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE uint32_t nrf_uart_int_enable_check(NRF_UART_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

NRF_STATIC_INLINE void nrf_uart_int_disable(NRF_UART_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

NRF_STATIC_INLINE uint32_t nrf_uart_errorsrc_get_and_clear(NRF_UART_Type * p_reg)
{
    uint32_t errsrc_mask = p_reg->ERRORSRC;
    p_reg->ERRORSRC = errsrc_mask;
    return errsrc_mask;
}

NRF_STATIC_INLINE void nrf_uart_enable(NRF_UART_Type * p_reg)
{
    p_reg->ENABLE = UART_ENABLE_ENABLE_Enabled;
}

NRF_STATIC_INLINE void nrf_uart_disable(NRF_UART_Type * p_reg)
{
    p_reg->ENABLE = UART_ENABLE_ENABLE_Disabled;
}

NRF_STATIC_INLINE void nrf_uart_txrx_pins_set(NRF_UART_Type * p_reg,
                                              uint32_t        pseltxd,
                                              uint32_t        pselrxd)
{
#if defined(UART_PSEL_RXD_CONNECT_Pos)
    p_reg->PSEL.RXD = pselrxd;
#else
    p_reg->PSELRXD = pselrxd;
#endif
#if defined(UART_PSEL_TXD_CONNECT_Pos)
    p_reg->PSEL.TXD = pseltxd;
#else
    p_reg->PSELTXD = pseltxd;
#endif
}

NRF_STATIC_INLINE void nrf_uart_txrx_pins_disconnect(NRF_UART_Type * p_reg)
{
    nrf_uart_txrx_pins_set(p_reg, NRF_UART_PSEL_DISCONNECTED, NRF_UART_PSEL_DISCONNECTED);
}

NRF_STATIC_INLINE uint32_t nrf_uart_tx_pin_get(NRF_UART_Type const * p_reg)
{
#if defined(UART_PSEL_TXD_CONNECT_Pos)
    return p_reg->PSEL.TXD;
#else
    return p_reg->PSELTXD;
#endif
}

NRF_STATIC_INLINE uint32_t nrf_uart_rx_pin_get(NRF_UART_Type const * p_reg)
{
#if defined(UART_PSEL_RXD_CONNECT_Pos)
    return p_reg->PSEL.RXD;
#else
    return p_reg->PSELRXD;
#endif
}

NRF_STATIC_INLINE uint32_t nrf_uart_rts_pin_get(NRF_UART_Type const * p_reg)
{
#if defined(UART_PSEL_RTS_CONNECT_Pos)
    return p_reg->PSEL.RTS;
#else
    return p_reg->PSELRTS;
#endif
}

NRF_STATIC_INLINE uint32_t nrf_uart_cts_pin_get(NRF_UART_Type const * p_reg)
{
#if defined(UART_PSEL_RTS_CONNECT_Pos)
    return p_reg->PSEL.CTS;
#else
    return p_reg->PSELCTS;
#endif
}

NRF_STATIC_INLINE void nrf_uart_hwfc_pins_set(NRF_UART_Type * p_reg,
                                              uint32_t        pselrts,
                                              uint32_t        pselcts)
{
#if defined(UART_PSEL_RTS_CONNECT_Pos)
    p_reg->PSEL.RTS = pselrts;
#else
    p_reg->PSELRTS = pselrts;
#endif

#if defined(UART_PSEL_RTS_CONNECT_Pos)
    p_reg->PSEL.CTS = pselcts;
#else
    p_reg->PSELCTS = pselcts;
#endif
}

NRF_STATIC_INLINE void nrf_uart_hwfc_pins_disconnect(NRF_UART_Type * p_reg)
{
    nrf_uart_hwfc_pins_set(p_reg, NRF_UART_PSEL_DISCONNECTED, NRF_UART_PSEL_DISCONNECTED);
}

NRF_STATIC_INLINE uint8_t nrf_uart_rxd_get(NRF_UART_Type const * p_reg)
{
    return p_reg->RXD;
}

NRF_STATIC_INLINE void nrf_uart_txd_set(NRF_UART_Type * p_reg, uint8_t txd)
{
    p_reg->TXD = txd;
}

NRF_STATIC_INLINE void nrf_uart_task_trigger(NRF_UART_Type * p_reg, nrf_uart_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

NRF_STATIC_INLINE uint32_t nrf_uart_task_address_get(NRF_UART_Type const * p_reg,
                                                     nrf_uart_task_t       task)
{
    return (uint32_t)p_reg + (uint32_t)task;
}

NRF_STATIC_INLINE void nrf_uart_configure(NRF_UART_Type           * p_reg,
                                          nrf_uart_config_t const * p_cfg)
{
    p_reg->CONFIG = (uint32_t)p_cfg->parity
#if defined(UART_CONFIG_STOP_Msk)
                    | (uint32_t)p_cfg->stop
#endif
#if defined(UART_CONFIG_PARITYTYPE_Msk)
                    | (uint32_t)p_cfg->paritytype
#endif
                    | (uint32_t)p_cfg->hwfc;
}

NRF_STATIC_INLINE void nrf_uart_baudrate_set(NRF_UART_Type * p_reg, nrf_uart_baudrate_t baudrate)
{
    p_reg->BAUDRATE = baudrate;
}
#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_UART_H__
