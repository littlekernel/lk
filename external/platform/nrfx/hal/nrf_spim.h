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

#ifndef NRF_SPIM_H__
#define NRF_SPIM_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_spim_hal SPIM HAL
 * @{
 * @ingroup nrf_spim
 * @brief   Hardware access layer for managing the SPIM peripheral.
 */

/**
 * @brief This value can be used as a parameter for the @ref nrf_spim_pins_set
 *        function to specify that a given SPI signal (SCK, MOSI, or MISO)
 *        shall not be connected to a physical pin.
 */
#define NRF_SPIM_PIN_NOT_CONNECTED  0xFFFFFFFF

/** @brief Macro for checking if the hardware chip select function is available. */
#define NRF_SPIM_HW_CSN_PRESENT                        \
    (NRFX_CHECK(SPIM0_FEATURE_HARDWARE_CSN_PRESENT) || \
     NRFX_CHECK(SPIM1_FEATURE_HARDWARE_CSN_PRESENT) || \
     NRFX_CHECK(SPIM2_FEATURE_HARDWARE_CSN_PRESENT) || \
     NRFX_CHECK(SPIM3_FEATURE_HARDWARE_CSN_PRESENT) || \
     NRFX_CHECK(SPIM4_FEATURE_HARDWARE_CSN_PRESENT))

/** @brief Macro for checking if the DCX pin control is available. */
#define NRF_SPIM_DCX_PRESENT                  \
    (NRFX_CHECK(SPIM0_FEATURE_DCX_PRESENT) || \
     NRFX_CHECK(SPIM1_FEATURE_DCX_PRESENT) || \
     NRFX_CHECK(SPIM2_FEATURE_DCX_PRESENT) || \
     NRFX_CHECK(SPIM3_FEATURE_DCX_PRESENT) || \
     NRFX_CHECK(SPIM4_FEATURE_DCX_PRESENT))

/** @brief Macro for checking if the RXDELAY function is available. */
#define NRF_SPIM_RXDELAY_PRESENT                  \
    (NRFX_CHECK(SPIM0_FEATURE_RXDELAY_PRESENT) || \
     NRFX_CHECK(SPIM1_FEATURE_RXDELAY_PRESENT) || \
     NRFX_CHECK(SPIM2_FEATURE_RXDELAY_PRESENT) || \
     NRFX_CHECK(SPIM3_FEATURE_RXDELAY_PRESENT) || \
     NRFX_CHECK(SPIM4_FEATURE_RXDELAY_PRESENT))

#if defined(NRF_SPIM_DCX_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief This value specified in the DCX line configuration causes this line
 *        to be set low during whole transmission (all transmitted bytes are
 *        marked as command bytes). Any lower value causes the DCX line to be
 *        switched from low to high after this number of bytes is transmitted
 *        (all remaining bytes are marked as data bytes).
 */
#define NRF_SPIM_DCX_CNT_ALL_CMD 0xF
#endif

/** @brief SPIM tasks. */
typedef enum
{
    NRF_SPIM_TASK_START   = offsetof(NRF_SPIM_Type, TASKS_START),   ///< Start SPI transaction.
    NRF_SPIM_TASK_STOP    = offsetof(NRF_SPIM_Type, TASKS_STOP),    ///< Stop SPI transaction.
    NRF_SPIM_TASK_SUSPEND = offsetof(NRF_SPIM_Type, TASKS_SUSPEND), ///< Suspend SPI transaction.
    NRF_SPIM_TASK_RESUME  = offsetof(NRF_SPIM_Type, TASKS_RESUME)   ///< Resume SPI transaction.
} nrf_spim_task_t;

/** @brief SPIM events. */
typedef enum
{
    NRF_SPIM_EVENT_STOPPED = offsetof(NRF_SPIM_Type, EVENTS_STOPPED), ///< SPI transaction has stopped.
    NRF_SPIM_EVENT_ENDRX   = offsetof(NRF_SPIM_Type, EVENTS_ENDRX),   ///< End of RXD buffer reached.
    NRF_SPIM_EVENT_END     = offsetof(NRF_SPIM_Type, EVENTS_END),     ///< End of RXD buffer and TXD buffer reached.
    NRF_SPIM_EVENT_ENDTX   = offsetof(NRF_SPIM_Type, EVENTS_ENDTX),   ///< End of TXD buffer reached.
    NRF_SPIM_EVENT_STARTED = offsetof(NRF_SPIM_Type, EVENTS_STARTED)  ///< Transaction started.
} nrf_spim_event_t;

/**
 * @brief SPIM shortcuts.
 */
typedef enum
{
    NRF_SPIM_SHORT_END_START_MASK = SPIM_SHORTS_END_START_Msk, ///< Shortcut between END event and START task.
    NRF_SPIM_ALL_SHORTS_MASK      = SPIM_SHORTS_END_START_Msk  ///< All SPIM shortcuts.
} nrf_spim_short_mask_t;

/** @brief SPIM interrupts. */
typedef enum
{
    NRF_SPIM_INT_STOPPED_MASK = SPIM_INTENSET_STOPPED_Msk,  ///< Interrupt on STOPPED event.
    NRF_SPIM_INT_ENDRX_MASK   = SPIM_INTENSET_ENDRX_Msk,    ///< Interrupt on ENDRX event.
    NRF_SPIM_INT_END_MASK     = SPIM_INTENSET_END_Msk,      ///< Interrupt on END event.
    NRF_SPIM_INT_ENDTX_MASK   = SPIM_INTENSET_ENDTX_Msk,    ///< Interrupt on ENDTX event.
    NRF_SPIM_INT_STARTED_MASK = SPIM_INTENSET_STARTED_Msk,  ///< Interrupt on STARTED event.
    NRF_SPIM_ALL_INTS_MASK    = SPIM_INTENSET_STOPPED_Msk |
                                SPIM_INTENSET_ENDRX_Msk   |
                                SPIM_INTENSET_END_Msk     |
                                SPIM_INTENSET_ENDTX_Msk   |
                                SPIM_INTENSET_STARTED_Msk   ///< All SPIM interrupts.
} nrf_spim_int_mask_t;

/** @brief SPI master data rates. */
typedef enum
{
    NRF_SPIM_FREQ_125K = SPIM_FREQUENCY_FREQUENCY_K125,    ///< 125 kbps.
    NRF_SPIM_FREQ_250K = SPIM_FREQUENCY_FREQUENCY_K250,    ///< 250 kbps.
    NRF_SPIM_FREQ_500K = SPIM_FREQUENCY_FREQUENCY_K500,    ///< 500 kbps.
    NRF_SPIM_FREQ_1M   = SPIM_FREQUENCY_FREQUENCY_M1,      ///< 1 Mbps.
    NRF_SPIM_FREQ_2M   = SPIM_FREQUENCY_FREQUENCY_M2,      ///< 2 Mbps.
    NRF_SPIM_FREQ_4M   = SPIM_FREQUENCY_FREQUENCY_M4,      ///< 4 Mbps.
    // [conversion to 'int' needed to prevent compilers from complaining
    //  that the provided value (0x80000000UL) is out of range of "int"]
    NRF_SPIM_FREQ_8M   = (int)SPIM_FREQUENCY_FREQUENCY_M8, ///< 8 Mbps.
#if defined(SPIM_FREQUENCY_FREQUENCY_M16) || defined(__NRFX_DOXYGEN__)
    NRF_SPIM_FREQ_16M  = SPIM_FREQUENCY_FREQUENCY_M16,     ///< 16 Mbps.
#endif
#if defined(SPIM_FREQUENCY_FREQUENCY_M32) || defined(__NRFX_DOXYGEN__)
    NRF_SPIM_FREQ_32M  = SPIM_FREQUENCY_FREQUENCY_M32      ///< 32 Mbps.
#endif
} nrf_spim_frequency_t;

/** @brief SPI modes. */
typedef enum
{
    NRF_SPIM_MODE_0, ///< SCK active high, sample on leading edge of clock.
    NRF_SPIM_MODE_1, ///< SCK active high, sample on trailing edge of clock.
    NRF_SPIM_MODE_2, ///< SCK active low, sample on leading edge of clock.
    NRF_SPIM_MODE_3  ///< SCK active low, sample on trailing edge of clock.
} nrf_spim_mode_t;

/** @brief SPI bit orders. */
typedef enum
{
    NRF_SPIM_BIT_ORDER_MSB_FIRST = SPIM_CONFIG_ORDER_MsbFirst, ///< Most significant bit shifted out first.
    NRF_SPIM_BIT_ORDER_LSB_FIRST = SPIM_CONFIG_ORDER_LsbFirst  ///< Least significant bit shifted out first.
} nrf_spim_bit_order_t;

#if (NRF_SPIM_HW_CSN_PRESENT) || defined(__NRFX_DOXYGEN__)
/** @brief SPI CSN pin polarity. */
typedef enum
{
    NRF_SPIM_CSN_POL_LOW  = SPIM_CSNPOL_CSNPOL_LOW, ///< Active low (idle state high).
    NRF_SPIM_CSN_POL_HIGH = SPIM_CSNPOL_CSNPOL_HIGH ///< Active high (idle state low).
} nrf_spim_csn_pol_t;
#endif // (NRF_SPIM_HW_CSN_PRESENT) || defined(__NRFX_DOXYGEN__)


/**
 * @brief Function for activating the specified SPIM task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task to be activated.
 */
NRF_STATIC_INLINE void nrf_spim_task_trigger(NRF_SPIM_Type * p_reg,
                                             nrf_spim_task_t task);

/**
 * @brief Function for getting the address of the specified SPIM task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  The specified task.
 *
 * @return Address of the specified task register.
 */
NRF_STATIC_INLINE uint32_t nrf_spim_task_address_get(NRF_SPIM_Type const * p_reg,
                                                     nrf_spim_task_t       task);

/**
 * @brief Function for clearing the specified SPIM event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be cleared.
 */
NRF_STATIC_INLINE void nrf_spim_event_clear(NRF_SPIM_Type *  p_reg,
                                            nrf_spim_event_t event);

/**
 * @brief Function for retrieving the state of the SPIM event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_spim_event_check(NRF_SPIM_Type const * p_reg,
                                            nrf_spim_event_t      event);

/**
 * @brief Function for getting the address of the specified SPIM event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event The specified event.
 *
 * @return Address of the specified event register.
 */
NRF_STATIC_INLINE uint32_t nrf_spim_event_address_get(NRF_SPIM_Type const * p_reg,
                                                      nrf_spim_event_t      event);

/**
 * @brief Function for enabling the specified shortcuts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Shortcuts to be enabled.
 */
NRF_STATIC_INLINE void nrf_spim_shorts_enable(NRF_SPIM_Type * p_reg,
                                              uint32_t        mask);

/**
 * @brief Function for disabling the specified shortcuts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Shortcuts to be disabled.
 */
NRF_STATIC_INLINE void nrf_spim_shorts_disable(NRF_SPIM_Type * p_reg,
                                               uint32_t        mask);

/**
 * @brief Function for getting the shortcut setting.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Current shortcut configuration.
 */
NRF_STATIC_INLINE uint32_t nrf_spim_shorts_get(NRF_SPIM_Type const * p_reg);

/**
 * @brief Function for enabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
NRF_STATIC_INLINE void nrf_spim_int_enable(NRF_SPIM_Type * p_reg,
                                           uint32_t        mask);

/**
 * @brief Function for disabling the specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
NRF_STATIC_INLINE void nrf_spim_int_disable(NRF_SPIM_Type * p_reg,
                                            uint32_t        mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_spim_int_enable_check(NRF_SPIM_Type const * p_reg, uint32_t mask);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        SPIM task.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
NRF_STATIC_INLINE void nrf_spim_subscribe_set(NRF_SPIM_Type * p_reg,
                                              nrf_spim_task_t task,
                                              uint8_t         channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        SPIM task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_spim_subscribe_clear(NRF_SPIM_Type * p_reg,
                                                nrf_spim_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        SPIM event.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
NRF_STATIC_INLINE void nrf_spim_publish_set(NRF_SPIM_Type *  p_reg,
                                            nrf_spim_event_t event,
                                            uint8_t          channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        SPIM event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event for which to clear the configuration.
 */
NRF_STATIC_INLINE void nrf_spim_publish_clear(NRF_SPIM_Type *  p_reg,
                                              nrf_spim_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for enabling the SPIM peripheral.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spim_enable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for disabling the SPIM peripheral.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spim_disable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for configuring SPIM pins.
 *
 * If a given signal is not needed, pass the @ref NRF_SPIM_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] sck_pin  SCK pin number.
 * @param[in] mosi_pin MOSI pin number.
 * @param[in] miso_pin MISO pin number.
 */
NRF_STATIC_INLINE void nrf_spim_pins_set(NRF_SPIM_Type * p_reg,
                                         uint32_t        sck_pin,
                                         uint32_t        mosi_pin,
                                         uint32_t        miso_pin);

/**
 * @brief Function for getting the SCK pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return SCK pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_spim_sck_pin_get(NRF_SPIM_Type const * p_reg);

/**
 * @brief Function for getting the MOSI pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return MOSI pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_spim_mosi_pin_get(NRF_SPIM_Type const * p_reg);

/**
 * @brief Function for getting the MISO pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return MISO pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_spim_miso_pin_get(NRF_SPIM_Type const * p_reg);

#if (NRF_SPIM_HW_CSN_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for configuring the SPIM hardware CSN pin.
 *
 * If this signal is not needed, pass the @ref NRF_SPIM_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] pin      CSN pin number.
 * @param[in] polarity CSN pin polarity.
 * @param[in] duration Minimum duration between the edge of CSN and the edge of SCK
 *                     and minimum duration of CSN must stay unselected between transactions.
 *                     The value is specified in number of 64 MHz clock cycles (15.625 ns).
 */
NRF_STATIC_INLINE void nrf_spim_csn_configure(NRF_SPIM_Type *    p_reg,
                                              uint32_t           pin,
                                              nrf_spim_csn_pol_t polarity,
                                              uint32_t           duration);
#endif // (NRF_SPIM_HW_CSN_PRESENT) || defined(__NRFX_DOXYGEN__)

#if NRF_SPIM_DCX_PRESENT || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for configuring the SPIM DCX pin.
 *
 * If this signal is not needed, pass the @ref NRF_SPIM_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] dcx_pin DCX pin number.
 */
NRF_STATIC_INLINE void nrf_spim_dcx_pin_set(NRF_SPIM_Type * p_reg,
                                            uint32_t        dcx_pin);

/**
 * @brief Function for getting the DCX pin selection.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return DCX pin selection.
 */
NRF_STATIC_INLINE uint32_t nrf_spim_dcx_pin_get(NRF_SPIM_Type const * p_reg);

/**
 * @brief Function for configuring the number of command bytes.
 *
 * Maximum value available for dividing the transmitted bytes into command
 * bytes and data bytes is @ref NRF_SPIM_DCX_CNT_ALL_CMD - 1.
 * The @ref NRF_SPIM_DCX_CNT_ALL_CMD value passed as the @c count parameter
 * causes all transmitted bytes to be marked as command bytes.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] count Number of command bytes preceding the data bytes.
 */
NRF_STATIC_INLINE void nrf_spim_dcx_cnt_set(NRF_SPIM_Type * p_reg,
                                            uint32_t        count);
#endif // NRF_SPIM_DCX_PRESENT || defined(__NRFX_DOXYGEN__)

#if NRF_SPIM_RXDELAY_PRESENT || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for configuring the extended SPIM interface.
 *
 * @param[in] p_reg   Pointer to the structure of registers of the peripheral.
 * @param[in] rxdelay Sample delay for input serial data on MISO,
 *                    specified in 64 MHz clock cycles (15.625 ns) from the sampling edge of SCK.
 */
NRF_STATIC_INLINE void nrf_spim_iftiming_set(NRF_SPIM_Type * p_reg,
                                             uint32_t        rxdelay);
#endif // NRF_SPIM_RXDELAY_PRESENT || defined(__NRFX_DOXYGEN__)

#if defined(SPIM_STALLSTAT_RX_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for clearing stall status for RX EasyDMA RAM accesses.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spim_stallstat_rx_clear(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for getting stall status for RX EasyDMA RAM accesses.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Stall status of RX EasyDMA RAM accesses.
 */
NRF_STATIC_INLINE bool nrf_spim_stallstat_rx_get(NRF_SPIM_Type const * p_reg);
#endif // defined(SPIM_STALLSTAT_RX_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(SPIM_STALLSTAT_TX_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for clearing stall status for TX EasyDMA RAM accesses.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spim_stallstat_tx_clear(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for getting stall status for TX EasyDMA RAM accesses.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Stall status of TX EasyDMA RAM accesses.
 */
NRF_STATIC_INLINE bool nrf_spim_stallstat_tx_get(NRF_SPIM_Type const * p_reg);
#endif // defined(SPIM_STALLSTAT_TX_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for setting the SPI master data rate.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] frequency SPI frequency.
 */
NRF_STATIC_INLINE void nrf_spim_frequency_set(NRF_SPIM_Type *      p_reg,
                                              nrf_spim_frequency_t frequency);

/**
 * @brief Function for setting the transmit buffer.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_buffer Pointer to the buffer with data to send.
 * @param[in] length   Maximum number of data bytes to transmit.
 */
NRF_STATIC_INLINE void nrf_spim_tx_buffer_set(NRF_SPIM_Type * p_reg,
                                              uint8_t const * p_buffer,
                                              size_t          length);

/**
 * @brief Function for setting the receive buffer.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_buffer Pointer to the buffer for received data.
 * @param[in] length   Maximum number of data bytes to receive.
 */
NRF_STATIC_INLINE void nrf_spim_rx_buffer_set(NRF_SPIM_Type * p_reg,
                                              uint8_t *       p_buffer,
                                              size_t          length);

/**
 * @brief Function for setting the SPI configuration.
 *
 * @param[in] p_reg         Pointer to the structure of registers of the peripheral.
 * @param[in] spi_mode      SPI mode.
 * @param[in] spi_bit_order SPI bit order.
 */
NRF_STATIC_INLINE void nrf_spim_configure(NRF_SPIM_Type *      p_reg,
                                          nrf_spim_mode_t      spi_mode,
                                          nrf_spim_bit_order_t spi_bit_order);

/**
 * @brief Function for setting the over-read character.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] orc   Over-read character that is clocked out in case of
 *                  an over-read of the TXD buffer.
 */
NRF_STATIC_INLINE void nrf_spim_orc_set(NRF_SPIM_Type * p_reg,
                                        uint8_t         orc);

/**
 * @brief Function for enabling the TX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spim_tx_list_enable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for disabling the TX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spim_tx_list_disable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for enabling the RX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spim_rx_list_enable(NRF_SPIM_Type * p_reg);

/**
 * @brief Function for disabling the RX list feature.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_spim_rx_list_disable(NRF_SPIM_Type * p_reg);


#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_spim_task_trigger(NRF_SPIM_Type * p_reg,
                                             nrf_spim_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

NRF_STATIC_INLINE uint32_t nrf_spim_task_address_get(NRF_SPIM_Type const * p_reg,
                                                     nrf_spim_task_t       task)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)task);
}

NRF_STATIC_INLINE void nrf_spim_event_clear(NRF_SPIM_Type *  p_reg,
                                            nrf_spim_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
    nrf_event_readback((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE bool nrf_spim_event_check(NRF_SPIM_Type const * p_reg,
                                            nrf_spim_event_t      event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE uint32_t nrf_spim_event_address_get(NRF_SPIM_Type const * p_reg,
                                                      nrf_spim_event_t      event)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_spim_shorts_enable(NRF_SPIM_Type * p_reg,
                                              uint32_t        mask)
{
    p_reg->SHORTS |= mask;
}

NRF_STATIC_INLINE void nrf_spim_shorts_disable(NRF_SPIM_Type * p_reg,
                                               uint32_t        mask)
{
    p_reg->SHORTS &= ~(mask);
}

NRF_STATIC_INLINE uint32_t nrf_spim_shorts_get(NRF_SPIM_Type const * p_reg)
{
    return p_reg->SHORTS;
}

NRF_STATIC_INLINE void nrf_spim_int_enable(NRF_SPIM_Type * p_reg,
                                           uint32_t        mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE void nrf_spim_int_disable(NRF_SPIM_Type * p_reg,
                                            uint32_t        mask)
{
    p_reg->INTENCLR = mask;
}

NRF_STATIC_INLINE uint32_t nrf_spim_int_enable_check(NRF_SPIM_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

#if defined(DPPI_PRESENT)
NRF_STATIC_INLINE void nrf_spim_subscribe_set(NRF_SPIM_Type * p_reg,
                                              nrf_spim_task_t task,
                                              uint8_t         channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | SPIM_SUBSCRIBE_START_EN_Msk);
}

NRF_STATIC_INLINE void nrf_spim_subscribe_clear(NRF_SPIM_Type * p_reg,
                                                nrf_spim_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) task + 0x80uL)) = 0;
}

NRF_STATIC_INLINE void nrf_spim_publish_set(NRF_SPIM_Type *  p_reg,
                                            nrf_spim_event_t event,
                                            uint8_t          channel)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | SPIM_PUBLISH_STARTED_EN_Msk);
}

NRF_STATIC_INLINE void nrf_spim_publish_clear(NRF_SPIM_Type *  p_reg,
                                              nrf_spim_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) p_reg + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

NRF_STATIC_INLINE void nrf_spim_enable(NRF_SPIM_Type * p_reg)
{
    p_reg->ENABLE = (SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos);
}

NRF_STATIC_INLINE void nrf_spim_disable(NRF_SPIM_Type * p_reg)
{
    p_reg->ENABLE = (SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos);
}

NRF_STATIC_INLINE void nrf_spim_pins_set(NRF_SPIM_Type * p_reg,
                                         uint32_t        sck_pin,
                                         uint32_t        mosi_pin,
                                         uint32_t        miso_pin)
{
    p_reg->PSEL.SCK  = sck_pin;
    p_reg->PSEL.MOSI = mosi_pin;
    p_reg->PSEL.MISO = miso_pin;
}

NRF_STATIC_INLINE uint32_t nrf_spim_sck_pin_get(NRF_SPIM_Type const * p_reg)
{
    return p_reg->PSEL.SCK;
}

NRF_STATIC_INLINE uint32_t nrf_spim_mosi_pin_get(NRF_SPIM_Type const * p_reg)
{
    return p_reg->PSEL.MOSI;
}

NRF_STATIC_INLINE uint32_t nrf_spim_miso_pin_get(NRF_SPIM_Type const * p_reg)
{
    return p_reg->PSEL.MISO;
}

#if NRF_SPIM_HW_CSN_PRESENT
NRF_STATIC_INLINE void nrf_spim_csn_configure(NRF_SPIM_Type *    p_reg,
                                              uint32_t           pin,
                                              nrf_spim_csn_pol_t polarity,
                                              uint32_t           duration)
{
    p_reg->PSEL.CSN = pin;
    p_reg->CSNPOL = polarity;
    p_reg->IFTIMING.CSNDUR = duration;
}
#endif // NRF_SPIM_HW_CSN_PRESENT

#if NRF_SPIM_DCX_PRESENT
NRF_STATIC_INLINE void nrf_spim_dcx_pin_set(NRF_SPIM_Type * p_reg,
                                            uint32_t        dcx_pin)
{
    p_reg->PSELDCX = dcx_pin;
}

NRF_STATIC_INLINE uint32_t nrf_spim_dcx_pin_get(NRF_SPIM_Type const * p_reg)
{
    return p_reg->PSELDCX;
}

NRF_STATIC_INLINE void nrf_spim_dcx_cnt_set(NRF_SPIM_Type * p_reg,
                                            uint32_t        dcx_cnt)
{
    p_reg->DCXCNT = dcx_cnt;
}
#endif // NRF_SPIM_DCX_PRESENT

#if NRF_SPIM_RXDELAY_PRESENT
NRF_STATIC_INLINE void nrf_spim_iftiming_set(NRF_SPIM_Type * p_reg,
                                             uint32_t        rxdelay)
{
    p_reg->IFTIMING.RXDELAY = rxdelay;
}
#endif // NRF_SPIM_RXDELAY_PRESENT

#if defined(SPIM_STALLSTAT_RX_Msk)
NRF_STATIC_INLINE void nrf_spim_stallstat_rx_clear(NRF_SPIM_Type * p_reg)
{
    p_reg->STALLSTAT &= ~(SPIM_STALLSTAT_RX_Msk);
}

NRF_STATIC_INLINE bool nrf_spim_stallstat_rx_get(NRF_SPIM_Type const * p_reg)
{
    return (p_reg->STALLSTAT & SPIM_STALLSTAT_RX_Msk) != 0;
}
#endif // defined(SPIM_STALLSTAT_RX_Msk)

#if defined(SPIM_STALLSTAT_TX_Msk)
NRF_STATIC_INLINE void nrf_spim_stallstat_tx_clear(NRF_SPIM_Type * p_reg)
{
    p_reg->STALLSTAT &= ~(SPIM_STALLSTAT_TX_Msk);
}

NRF_STATIC_INLINE bool nrf_spim_stallstat_tx_get(NRF_SPIM_Type const * p_reg)
{
    return (p_reg->STALLSTAT & SPIM_STALLSTAT_TX_Msk) != 0;
}
#endif // defined(SPIM_STALLSTAT_TX_Msk)

NRF_STATIC_INLINE void nrf_spim_frequency_set(NRF_SPIM_Type *      p_reg,
                                              nrf_spim_frequency_t frequency)
{
    p_reg->FREQUENCY = (uint32_t)frequency;
}

NRF_STATIC_INLINE void nrf_spim_tx_buffer_set(NRF_SPIM_Type * p_reg,
                                              uint8_t const * p_buffer,
                                              size_t          length)
{
    p_reg->TXD.PTR    = (uint32_t)p_buffer;
    p_reg->TXD.MAXCNT = length;
}

NRF_STATIC_INLINE void nrf_spim_rx_buffer_set(NRF_SPIM_Type * p_reg,
                                              uint8_t * p_buffer,
                                              size_t    length)
{
    p_reg->RXD.PTR    = (uint32_t)p_buffer;
    p_reg->RXD.MAXCNT = length;
}

NRF_STATIC_INLINE void nrf_spim_configure(NRF_SPIM_Type *      p_reg,
                                          nrf_spim_mode_t      spi_mode,
                                          nrf_spim_bit_order_t spi_bit_order)
{
    uint32_t config = (spi_bit_order == NRF_SPIM_BIT_ORDER_MSB_FIRST ?
        SPIM_CONFIG_ORDER_MsbFirst : SPIM_CONFIG_ORDER_LsbFirst);
    switch (spi_mode)
    {
    default:
    case NRF_SPIM_MODE_0:
        config |= (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Leading    << SPIM_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIM_MODE_1:
        config |= (SPIM_CONFIG_CPOL_ActiveHigh << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Trailing   << SPIM_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIM_MODE_2:
        config |= (SPIM_CONFIG_CPOL_ActiveLow  << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Leading    << SPIM_CONFIG_CPHA_Pos);
        break;

    case NRF_SPIM_MODE_3:
        config |= (SPIM_CONFIG_CPOL_ActiveLow  << SPIM_CONFIG_CPOL_Pos) |
                  (SPIM_CONFIG_CPHA_Trailing   << SPIM_CONFIG_CPHA_Pos);
        break;
    }
    p_reg->CONFIG = config;
}

NRF_STATIC_INLINE void nrf_spim_orc_set(NRF_SPIM_Type * p_reg,
                                        uint8_t         orc)
{
    p_reg->ORC = orc;
}


NRF_STATIC_INLINE void nrf_spim_tx_list_enable(NRF_SPIM_Type * p_reg)
{
    p_reg->TXD.LIST = SPIM_TXD_LIST_LIST_ArrayList << SPIM_TXD_LIST_LIST_Pos;
}

NRF_STATIC_INLINE void nrf_spim_tx_list_disable(NRF_SPIM_Type * p_reg)
{
    p_reg->TXD.LIST = SPIM_TXD_LIST_LIST_Disabled << SPIM_TXD_LIST_LIST_Pos;
}

NRF_STATIC_INLINE void nrf_spim_rx_list_enable(NRF_SPIM_Type * p_reg)
{
    p_reg->RXD.LIST = SPIM_RXD_LIST_LIST_ArrayList << SPIM_RXD_LIST_LIST_Pos;
}

NRF_STATIC_INLINE void nrf_spim_rx_list_disable(NRF_SPIM_Type * p_reg)
{
    p_reg->RXD.LIST = SPIM_RXD_LIST_LIST_Disabled << SPIM_RXD_LIST_LIST_Pos;
}

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_SPIM_H__
