/*
 * Copyright (c) 2016 - 2020, Nordic Semiconductor ASA
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

#ifndef NRF_QSPI_H__
#define NRF_QSPI_H__

#include <nrfx.h>
#include <nrf_erratas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_qspi_hal QSPI HAL
 * @{
 * @ingroup nrf_qspi
 * @brief   Hardware access layer for managing the QSPI peripheral.
 */

#if defined(QSPI_XIPEN_XIPEN_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether XIP can be explicitly enabled or disabled via XIPEN register. */
#define NRF_QSPI_HAS_XIPEN 1
#else
#define NRF_QSPI_HAS_XIPEN 0
#endif

#if defined(QSPI_XIP_ENC_ENABLE_ENABLE_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether encryption for XIP is present. */
#define NRF_QSPI_HAS_XIP_ENC 1
#else
#define NRF_QSPI_HAS_XIP_ENC 0
#endif

#if defined(QSPI_DMA_ENC_ENABLE_ENABLE_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether encryption for EasyDMA is present. */
#define NRF_QSPI_HAS_DMA_ENC 1
#else
#define NRF_QSPI_HAS_DMA_ENC 0
#endif

#if defined(QSPI_IFCONFIG1_SPIMODE_MODE3) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether support for QSPI mode 1 is present. */
#define NRF_QSPI_HAS_MODE_1 1
#else
#define NRF_QSPI_HAS_MODE_1 0
#endif

#if defined(NRF53_SERIES) || defined(__NRFX_DOXYGEN__)
/** @brief Value representing QSPI base clock frequency. */
#define NRF_QSPI_BASE_CLOCK_FREQ 96000000uL
#else
#define NRF_QSPI_BASE_CLOCK_FREQ 32000000uL
#endif

/**
 * @brief This value can be used as a parameter for the @ref nrf_qspi_pins_set
 *        function to specify that a given QSPI signal (SCK, CSN, IO0, IO1, IO2, or IO3)
 *        will not be connected to a physical pin.
 */
#define NRF_QSPI_PIN_NOT_CONNECTED 0xFF

/** @brief Macro for setting proper values to pin registers. */
#define NRF_QSPI_PIN_VAL(pin) (pin) == NRF_QSPI_PIN_NOT_CONNECTED ? 0xFFFFFFFF : (pin)


/** @brief QSPI tasks. */
typedef enum
{
    NRF_QSPI_TASK_ACTIVATE   = offsetof(NRF_QSPI_Type, TASKS_ACTIVATE),   /**< Activate the QSPI interface. */
    NRF_QSPI_TASK_READSTART  = offsetof(NRF_QSPI_Type, TASKS_READSTART),  /**< Start transfer from external flash memory to internal RAM. */
    NRF_QSPI_TASK_WRITESTART = offsetof(NRF_QSPI_Type, TASKS_WRITESTART), /**< Start transfer from internal RAM to external flash memory. */
    NRF_QSPI_TASK_ERASESTART = offsetof(NRF_QSPI_Type, TASKS_ERASESTART), /**< Start external flash memory erase operation. */
    NRF_QSPI_TASK_DEACTIVATE = offsetof(NRF_QSPI_Type, TASKS_DEACTIVATE), /**< Deactivate the QSPI interface. */
} nrf_qspi_task_t;

/** @brief QSPI events. */
typedef enum
{
    NRF_QSPI_EVENT_READY = offsetof(NRF_QSPI_Type, EVENTS_READY) /**< QSPI peripheral is ready after it executes any task. */
} nrf_qspi_event_t;

/** @brief QSPI interrupts. */
typedef enum
{
    NRF_QSPI_INT_READY_MASK = QSPI_INTENSET_READY_Msk /**< Interrupt on READY event. */
} nrf_qspi_int_mask_t;

/** @brief QSPI base clock frequency divider values. */
typedef enum
{
    NRF_QSPI_FREQ_DIV1,  /**< Divide by 1. */
    NRF_QSPI_FREQ_DIV2,  /**< Divide by 2. */
    NRF_QSPI_FREQ_DIV3,  /**< Divide by 3. */
    NRF_QSPI_FREQ_DIV4,  /**< Divide by 4. */
    NRF_QSPI_FREQ_DIV5,  /**< Divide by 5. */
    NRF_QSPI_FREQ_DIV6,  /**< Divide by 6. */
    NRF_QSPI_FREQ_DIV7,  /**< Divide by 7. */
    NRF_QSPI_FREQ_DIV8,  /**< Divide by 8. */
    NRF_QSPI_FREQ_DIV9,  /**< Divide by 9. */
    NRF_QSPI_FREQ_DIV10, /**< Divide by 10. */
    NRF_QSPI_FREQ_DIV11, /**< Divide by 11. */
    NRF_QSPI_FREQ_DIV12, /**< Divide by 12. */
    NRF_QSPI_FREQ_DIV13, /**< Divide by 13. */
    NRF_QSPI_FREQ_DIV14, /**< Divide by 14. */
    NRF_QSPI_FREQ_DIV15, /**< Divide by 15. */
    NRF_QSPI_FREQ_DIV16, /**< Divide by 16. */
} nrf_qspi_frequency_t;

#if defined(NRF52_SERIES)
/** Symbols translation for backward compatibility. */
#define NRF_QSPI_FREQ_32MDIV1  NRF_QSPI_FREQ_DIV1
#define NRF_QSPI_FREQ_32MDIV2  NRF_QSPI_FREQ_DIV2
#define NRF_QSPI_FREQ_32MDIV3  NRF_QSPI_FREQ_DIV3
#define NRF_QSPI_FREQ_32MDIV4  NRF_QSPI_FREQ_DIV4
#define NRF_QSPI_FREQ_32MDIV5  NRF_QSPI_FREQ_DIV5
#define NRF_QSPI_FREQ_32MDIV6  NRF_QSPI_FREQ_DIV6
#define NRF_QSPI_FREQ_32MDIV7  NRF_QSPI_FREQ_DIV7
#define NRF_QSPI_FREQ_32MDIV8  NRF_QSPI_FREQ_DIV8
#define NRF_QSPI_FREQ_32MDIV9  NRF_QSPI_FREQ_DIV9
#define NRF_QSPI_FREQ_32MDIV10 NRF_QSPI_FREQ_DIV10
#define NRF_QSPI_FREQ_32MDIV11 NRF_QSPI_FREQ_DIV11
#define NRF_QSPI_FREQ_32MDIV12 NRF_QSPI_FREQ_DIV12
#define NRF_QSPI_FREQ_32MDIV13 NRF_QSPI_FREQ_DIV13
#define NRF_QSPI_FREQ_32MDIV14 NRF_QSPI_FREQ_DIV14
#define NRF_QSPI_FREQ_32MDIV15 NRF_QSPI_FREQ_DIV15
#define NRF_QSPI_FREQ_32MDIV16 NRF_QSPI_FREQ_DIV16
#endif

/** @brief Interface configuration for a read operation. */
typedef enum
{
    NRF_QSPI_READOC_FASTREAD = QSPI_IFCONFIG0_READOC_FASTREAD, /**< Single data line SPI. FAST_READ (opcode 0x0B). */
    NRF_QSPI_READOC_READ2O   = QSPI_IFCONFIG0_READOC_READ2O,   /**< Dual data line SPI. READ2O (opcode 0x3B). */
    NRF_QSPI_READOC_READ2IO  = QSPI_IFCONFIG0_READOC_READ2IO,  /**< Dual data line SPI. READ2IO (opcode 0xBB). */
    NRF_QSPI_READOC_READ4O   = QSPI_IFCONFIG0_READOC_READ4O,   /**< Quad data line SPI. READ4O (opcode 0x6B). */
    NRF_QSPI_READOC_READ4IO  = QSPI_IFCONFIG0_READOC_READ4IO   /**< Quad data line SPI. READ4IO (opcode 0xEB). */
} nrf_qspi_readoc_t;

/** @brief Interface configuration for a write operation. */
typedef enum
{
    NRF_QSPI_WRITEOC_PP    = QSPI_IFCONFIG0_WRITEOC_PP,    /**< Single data line SPI. PP (opcode 0x02). */
    NRF_QSPI_WRITEOC_PP2O  = QSPI_IFCONFIG0_WRITEOC_PP2O,  /**< Dual data line SPI. PP2O (opcode 0xA2). */
    NRF_QSPI_WRITEOC_PP4O  = QSPI_IFCONFIG0_WRITEOC_PP4O,  /**< Quad data line SPI. PP4O (opcode 0x32). */
    NRF_QSPI_WRITEOC_PP4IO = QSPI_IFCONFIG0_WRITEOC_PP4IO, /**< Quad data line SPI. READ4O (opcode 0x38). */
} nrf_qspi_writeoc_t;

/** @brief Interface configuration for addressing mode. */
typedef enum
{
    NRF_QSPI_ADDRMODE_24BIT = QSPI_IFCONFIG0_ADDRMODE_24BIT, /**< 24-bit addressing. */
    NRF_QSPI_ADDRMODE_32BIT = QSPI_IFCONFIG0_ADDRMODE_32BIT  /**< 32-bit addressing. */
} nrf_qspi_addrmode_t;

/** @brief QSPI SPI mode. Polarization and phase configuration. */
typedef enum
{
    NRF_QSPI_MODE_0 = QSPI_IFCONFIG1_SPIMODE_MODE0, /**< Mode 0 (CPOL=0, CPHA=0). */
#if NRF_QSPI_HAS_MODE_1
    NRF_QSPI_MODE_1 = QSPI_IFCONFIG1_SPIMODE_MODE3  /**< Mode 1 (CPOL=1, CPHA=1). */
#endif
} nrf_qspi_spi_mode_t;

/** @brief Addressing configuration mode. */
typedef enum
{
    NRF_QSPI_ADDRCONF_MODE_NOINSTR = QSPI_ADDRCONF_MODE_NoInstr, /**< Do not send any instruction. */
    NRF_QSPI_ADDRCONF_MODE_OPCODE  = QSPI_ADDRCONF_MODE_Opcode,  /**< Send opcode. */
    NRF_QSPI_ADDRCONF_MODE_OPBYTE0 = QSPI_ADDRCONF_MODE_OpByte0, /**< Send opcode, byte0. */
    NRF_QSPI_ADDRCONF_MODE_ALL     = QSPI_ADDRCONF_MODE_All      /**< Send opcode, byte0, byte1. */
} nrf_qspi_addrconfig_mode_t;

/** @brief Erasing data length. */
typedef enum
{
    NRF_QSPI_ERASE_LEN_4KB  = QSPI_ERASE_LEN_LEN_4KB,  /**< Erase 4 kB block (flash command 0x20). */
    NRF_QSPI_ERASE_LEN_64KB = QSPI_ERASE_LEN_LEN_64KB, /**< Erase 64 kB block (flash command 0xD8). */
    NRF_QSPI_ERASE_LEN_ALL  = QSPI_ERASE_LEN_LEN_All   /**< Erase all (flash command 0xC7). */
} nrf_qspi_erase_len_t;

/** @brief Custom instruction length. */
typedef enum
{
    NRF_QSPI_CINSTR_LEN_1B = QSPI_CINSTRCONF_LENGTH_1B, /**< Send opcode only. */
    NRF_QSPI_CINSTR_LEN_2B = QSPI_CINSTRCONF_LENGTH_2B, /**< Send opcode, CINSTRDAT0.BYTE0. */
    NRF_QSPI_CINSTR_LEN_3B = QSPI_CINSTRCONF_LENGTH_3B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT0.BYTE1. */
    NRF_QSPI_CINSTR_LEN_4B = QSPI_CINSTRCONF_LENGTH_4B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT0.BYTE2. */
    NRF_QSPI_CINSTR_LEN_5B = QSPI_CINSTRCONF_LENGTH_5B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT0.BYTE3. */
    NRF_QSPI_CINSTR_LEN_6B = QSPI_CINSTRCONF_LENGTH_6B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT1.BYTE4. */
    NRF_QSPI_CINSTR_LEN_7B = QSPI_CINSTRCONF_LENGTH_7B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT1.BYTE5. */
    NRF_QSPI_CINSTR_LEN_8B = QSPI_CINSTRCONF_LENGTH_8B, /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT1.BYTE6. */
    NRF_QSPI_CINSTR_LEN_9B = QSPI_CINSTRCONF_LENGTH_9B  /**< Send opcode, CINSTRDAT0.BYTE0 -> CINSTRDAT1.BYTE7. */
} nrf_qspi_cinstr_len_t;

/** @brief Pin configuration. */
typedef struct
{
    uint8_t sck_pin; /**< SCK pin number. */
    uint8_t csn_pin; /**< Chip select pin number. */
    uint8_t io0_pin; /**< IO0/MOSI pin number. */
    uint8_t io1_pin; /**< IO1/MISO pin number. */
    uint8_t io2_pin; /**< IO2 pin number (optional).
                      *   Set to @ref NRF_QSPI_PIN_NOT_CONNECTED if this signal is not needed.
                      */
    uint8_t io3_pin; /**< IO3 pin number (optional).
                      *   Set to @ref NRF_QSPI_PIN_NOT_CONNECTED if this signal is not needed.
                      */
} nrf_qspi_pins_t;

/** @brief Custom instruction configuration. */
typedef struct
{
    uint8_t               opcode;    /**< Opcode used in custom instruction transmission. */
    nrf_qspi_cinstr_len_t length;    /**< Length of the custom instruction data. */
    bool                  io2_level; /**< I/O line level during transmission. */
    bool                  io3_level; /**< I/O line level during transmission. */
    bool                  wipwait;   /**< Wait if a Wait in Progress bit is set in the memory status byte. */
    bool                  wren;      /**< Send write enable before instruction. */
} nrf_qspi_cinstr_conf_t;

/** @brief Addressing mode register configuration. See @ref nrf_qspi_addrconfig_set */
typedef struct
{
    uint8_t                    opcode;  /**< Opcode used to enter the proper addressing mode. */
    uint8_t                    byte0;   /**< Byte following the opcode. */
    uint8_t                    byte1;   /**< Byte following byte0. */
    nrf_qspi_addrconfig_mode_t mode;    /**< Extended addresing mode. */
    bool                       wipwait; /**< Enable or disable waiting for complete operation execution. */
    bool                       wren;    /**< Send write enable before instruction. */
} nrf_qspi_addrconfig_conf_t;

/** @brief Structure with QSPI protocol interface configuration. */
typedef struct
{
    nrf_qspi_readoc_t   readoc;    /**< Read operation code. */
    nrf_qspi_writeoc_t  writeoc;   /**< Write operation code. */
    nrf_qspi_addrmode_t addrmode;  /**< Addresing mode (24-bit or 32-bit). */
    bool                dpmconfig; /**< Enable the Deep Power-down Mode (DPM) feature. */
} nrf_qspi_prot_conf_t;

/** @brief QSPI physical interface configuration. */
typedef struct
{
    uint8_t              sck_delay; /**< tSHSL, tWHSL, and tSHWL in number of 16 MHz periods (62.5ns). */
    bool                 dpmen;     /**< Enable the DPM feature. */
    nrf_qspi_spi_mode_t  spi_mode;  /**< SPI phase and polarization. */
    nrf_qspi_frequency_t sck_freq;  /**< SCK frequency given as QSPI base clock frequency divider.
                                     *   To calculate @p sck_freq value corresponding to chosen frequency,
                                     *   use the following equation:
                                     *
                                     *   sck_freq = (NRF_QSPI_BASE_CLOCK_FREQ / frequency) - 1
                                     *
                                     *   @note Achievable frequencies are determined by available
                                     *         divider values and QSPI base clock frequency.
                                     */
} nrf_qspi_phy_conf_t;


#if NRF_QSPI_HAS_XIP_ENC || NRF_QSPI_HAS_DMA_ENC
/** @brief QSPI encryption settings for XIP and DMA transfers. */
typedef struct
{
    uint32_t key[4];   /**< AES 128-bit key, stored on 4 32-bit words. */
    uint32_t nonce[3]; /**< AES 96-bit nonce, stored on 3 32-bit words. */
} nrf_qspi_encryption_t;
#endif

/**
 * @brief Function for activating the specified QSPI task.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  Task to be activated.
 */
NRF_STATIC_INLINE void nrf_qspi_task_trigger(NRF_QSPI_Type * p_reg, nrf_qspi_task_t task);

/**
 * @brief Function for getting the address of the specified QSPI task register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] task  QSPI task.
 *
 * @return Address of the specified task register.
 */
NRF_STATIC_INLINE uint32_t nrf_qspi_task_address_get(NRF_QSPI_Type const * p_reg,
                                                     nrf_qspi_task_t       task);

/**
 * @brief Function for clearing the specified QSPI event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be cleared.
 */
NRF_STATIC_INLINE void nrf_qspi_event_clear(NRF_QSPI_Type * p_reg, nrf_qspi_event_t event);

/**
 * @brief Function for retrieving the state of the QSPI event.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
NRF_STATIC_INLINE bool nrf_qspi_event_check(NRF_QSPI_Type const * p_reg, nrf_qspi_event_t event);

/**
 * @brief Function for getting the address of the specified QSPI event register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] event The specified event.
 *
 * @return Address of the specified event register.
 */
NRF_STATIC_INLINE uint32_t nrf_qspi_event_address_get(NRF_QSPI_Type const * p_reg,
                                                      nrf_qspi_event_t      event);

/**
 * @brief Function for enabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be enabled.
 */
NRF_STATIC_INLINE void nrf_qspi_int_enable(NRF_QSPI_Type * p_reg, uint32_t mask);

/**
 * @brief Function for disabling specified interrupts.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be disabled.
 */
NRF_STATIC_INLINE void nrf_qspi_int_disable(NRF_QSPI_Type * p_reg, uint32_t mask);

/**
 * @brief Function for checking if the specified interrupts are enabled.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] mask  Mask of interrupts to be checked.
 *
 * @return Mask of enabled interrupts.
 */
NRF_STATIC_INLINE uint32_t nrf_qspi_int_enable_check(NRF_QSPI_Type const * p_reg, uint32_t mask);

/**
 * @brief Function for enabling the QSPI peripheral.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_qspi_enable(NRF_QSPI_Type * p_reg);

/**
 * @brief Function for disabling the QSPI peripheral.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 */
NRF_STATIC_INLINE void nrf_qspi_disable(NRF_QSPI_Type * p_reg);

/**
 * @brief Function for configuring QSPI pins.
 *
 * If a given signal is not needed, pass the @ref NRF_QSPI_PIN_NOT_CONNECTED
 * value instead of its pin number.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] p_pins Pointer to the pins configuration structure. See @ref nrf_qspi_pins_t.
 */
NRF_STATIC_INLINE void nrf_qspi_pins_set(NRF_QSPI_Type *         p_reg,
                                         nrf_qspi_pins_t const * p_pins);

/**
 * @brief Function for setting the QSPI XIPOFFSET register.
 *
 * @param[in] p_reg      Pointer to the structure of registers of the peripheral.
 * @param[in] xip_offset Address offset in the external memory for Execute in Place operation.
 */
NRF_STATIC_INLINE void nrf_qspi_xip_offset_set(NRF_QSPI_Type * p_reg,
                                               uint32_t        xip_offset);

/**
 * @brief Function for setting the QSPI IFCONFIG0 register.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_config Pointer to the QSPI protocol interface configuration structure.
 *                     See @ref nrf_qspi_prot_conf_t.
 */
NRF_STATIC_INLINE void nrf_qspi_ifconfig0_set(NRF_QSPI_Type *              p_reg,
                                              nrf_qspi_prot_conf_t const * p_config);

/**
 * @brief Function for setting the QSPI IFCONFIG1 register.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_config Pointer to the QSPI physical interface configuration structure.
 *                     See @ref nrf_qspi_phy_conf_t.
 */
NRF_STATIC_INLINE void nrf_qspi_ifconfig1_set(NRF_QSPI_Type *             p_reg,
                                              nrf_qspi_phy_conf_t const * p_config);

/**
 * @brief Function for setting the QSPI ADDRCONF register.
 *
 * This function must be executed before sending task NRF_QSPI_TASK_ACTIVATE. Data stored in the structure
 * is sent during the start of the peripheral. Remember that the reset instruction can set
 * addressing mode to default in the memory device. If memory reset is necessary before configuring
 * the addressing mode, use custom instruction feature instead of this function.
 * Case with reset: Enable the peripheral without setting ADDRCONF register, send reset instructions
 * using a custom instruction feature (reset enable and then reset), set proper addressing mode
 * using the custom instruction feature.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_config Pointer to the addressing mode configuration structure.
 *                     See @ref nrf_qspi_addrconfig_conf_t.
*/
NRF_STATIC_INLINE void nrf_qspi_addrconfig_set(NRF_QSPI_Type *                    p_reg,
                                               nrf_qspi_addrconfig_conf_t const * p_config);

/**
 * @brief Function for setting write data into the peripheral register (without starting the process).
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] p_buffer  Pointer to the writing buffer.
 * @param[in] length    Lenght of the writing data.
 * @param[in] dest_addr Address in memory to write to.
 */
NRF_STATIC_INLINE void nrf_qspi_write_buffer_set(NRF_QSPI_Type * p_reg,
                                                 void const *    p_buffer,
                                                 uint32_t        length,
                                                 uint32_t        dest_addr);

/**
 * @brief Function for setting read data into the peripheral register (without starting the process).
 *
 * @param[in]  p_reg    Pointer to the structure of registers of the peripheral.
 * @param[out] p_buffer Pointer to the reading buffer.
 * @param[in]  length   Length of the read data.
 * @param[in]  src_addr Address in memory to read from.
 */
NRF_STATIC_INLINE void nrf_qspi_read_buffer_set(NRF_QSPI_Type * p_reg,
                                                void *          p_buffer,
                                                uint32_t        length,
                                                uint32_t        src_addr);

/**
 * @brief Function for setting erase data into the peripheral register (without starting the process).
 *
 * @param[in] p_reg      Pointer to the structure of registers of the peripheral.
 * @param[in] erase_addr Start address to erase. Address must have padding set to 4 bytes.
 * @param[in] len        Size of erasing area.
 */
NRF_STATIC_INLINE void nrf_qspi_erase_ptr_set(NRF_QSPI_Type *      p_reg,
                                              uint32_t             erase_addr,
                                              nrf_qspi_erase_len_t len);

/**
 * @brief Function for getting the peripheral status register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Peripheral status register.
 */
NRF_STATIC_INLINE uint32_t nrf_qspi_status_reg_get(NRF_QSPI_Type const * p_reg);

/**
 * @brief Function for getting the device status register stored in the peripheral status register.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Device status register (lower byte).
 */
NRF_STATIC_INLINE uint8_t nrf_qspi_sreg_get(NRF_QSPI_Type const * p_reg);

/**
 * @brief Function for checking if the peripheral is busy or not.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 *
 * @retval true  The QSPI is busy.
 * @retval false The QSPI is ready.
 */
NRF_STATIC_INLINE bool nrf_qspi_busy_check(NRF_QSPI_Type const * p_reg);

/**
 * @brief Function for setting registers sending with custom instruction transmission.
 *
 * This function can be ommited when using NRF_QSPI_CINSTR_LEN_1B as the length argument
 * (sending only opcode without data).
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] length    Length of the custom instruction data.
 * @param[in] p_tx_data Pointer to the data to send with the custom instruction.
 */
NRF_STATIC_INLINE void nrf_qspi_cinstrdata_set(NRF_QSPI_Type *       p_reg,
                                               nrf_qspi_cinstr_len_t length,
                                               void const *          p_tx_data);

/**
 * @brief Function for getting data from register after custom instruction transmission.
 *
 * @param[in] p_reg     Pointer to the structure of registers of the peripheral.
 * @param[in] length    Length of the custom instruction data.
 * @param[in] p_rx_data Pointer to the reading buffer.
 */
NRF_STATIC_INLINE void nrf_qspi_cinstrdata_get(NRF_QSPI_Type const * p_reg,
                                               nrf_qspi_cinstr_len_t length,
                                               void *                p_rx_data);

/**
 * @brief Function for sending custom instruction to external memory.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_config Pointer to the custom instruction configuration structure.
 *                     See @ref nrf_qspi_cinstr_conf_t.
 */
NRF_STATIC_INLINE void nrf_qspi_cinstr_transfer_start(NRF_QSPI_Type *                p_reg,
                                                      nrf_qspi_cinstr_conf_t const * p_config);

/**
 * @brief Function for starting a custom instruction long transfer.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] p_config Pointer to the custom instruction configuration structure.
 *                     See @ref nrf_qspi_cinstr_conf_t.
 */
NRF_STATIC_INLINE void nrf_qspi_cinstr_long_transfer_start(NRF_QSPI_Type *                p_reg,
                                                           nrf_qspi_cinstr_conf_t const * p_config);

/**
 * @brief Function for checking whether a custom instruction long transfer is ongoing.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 *
 * @retval true  Custom instruction long transfer is ongoing.
 * @retval false Custom instruction long transfer is not ongoing.
 */
NRF_STATIC_INLINE bool nrf_qspi_cinstr_long_transfer_is_ongoing(NRF_QSPI_Type const * p_reg);

/**
 * @brief Function for continuing a custom instruction long transfer.
 *
 * @param[in] p_reg    Pointer to the structure of registers of the peripheral.
 * @param[in] length   Length of the custom instruction data.
 * @param[in] finalize True if the custom instruction long transfer is to be finalized.
 *                     False if the custom instruction long transfer is to be continued.
 */
NRF_STATIC_INLINE void nrf_qspi_cinstr_long_transfer_continue(NRF_QSPI_Type *       p_reg,
                                                              nrf_qspi_cinstr_len_t length,
                                                              bool                  finalize);

#if NRF_QSPI_HAS_XIPEN
/**
 * @brief Function for enabling or disabling Execute in Place (XIP) operation.
 *
 * @note XIP can be enabled after reset. See Product Specification.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if XIP is to be enabled, false otherwise.
 */
NRF_STATIC_INLINE void nrf_qspi_xip_set(NRF_QSPI_Type * p_reg, bool enable);
#endif

#if NRF_QSPI_HAS_XIP_ENC
/**
 * @brief Function for configuring the XIP encryption.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] p_cfg Pointer to encryption configuration structure.
 */
NRF_STATIC_INLINE void nrf_qspi_xip_encryption_configure(NRF_QSPI_Type *               p_reg,
                                                         nrf_qspi_encryption_t const * p_cfg);

/**
 * @brief Function for enabling or disabling the XIP encryption.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if XIP encryption is to be enabled, false otherwise.
 */
NRF_STATIC_INLINE void nrf_qspi_xip_encryption_set(NRF_QSPI_Type * p_reg, bool enable);
#endif

#if NRF_QSPI_HAS_DMA_ENC
/**
 * @brief Function for configuring the EasyDMA encryption.
 *
 * @param[in] p_reg Pointer to the structure of registers of the peripheral.
 * @param[in] p_cfg Pointer to encryption configuration structure.
 */
NRF_STATIC_INLINE void nrf_qspi_dma_encryption_configure(NRF_QSPI_Type *               p_reg,
                                                         nrf_qspi_encryption_t const * p_cfg);

/**
 * @brief Function for enabling or disabling the EasyDMA encryption.
 *
 * @param[in] p_reg  Pointer to the structure of registers of the peripheral.
 * @param[in] enable True if EasyDMA encryption is to be enabled, false otherwise.
 */
NRF_STATIC_INLINE void nrf_qspi_dma_encryption_set(NRF_QSPI_Type * p_reg, bool enable);
#endif

#ifndef NRF_DECLARE_ONLY

NRF_STATIC_INLINE void nrf_qspi_task_trigger(NRF_QSPI_Type * p_reg, nrf_qspi_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)task)) = 0x1UL;
}

NRF_STATIC_INLINE uint32_t nrf_qspi_task_address_get(NRF_QSPI_Type const * p_reg,
                                                     nrf_qspi_task_t       task)
{
    return ((uint32_t)p_reg + (uint32_t)task);
}

NRF_STATIC_INLINE void nrf_qspi_event_clear(NRF_QSPI_Type * p_reg, nrf_qspi_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event)) = 0x0UL;
}

NRF_STATIC_INLINE bool nrf_qspi_event_check(NRF_QSPI_Type const * p_reg, nrf_qspi_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE uint32_t nrf_qspi_event_address_get(NRF_QSPI_Type const * p_reg,
                                                      nrf_qspi_event_t      event)
{
    return (uint32_t)((uint8_t *)p_reg + (uint32_t)event);
}

NRF_STATIC_INLINE void nrf_qspi_int_enable(NRF_QSPI_Type * p_reg, uint32_t mask)
{
    p_reg->INTENSET = mask;
}

NRF_STATIC_INLINE void nrf_qspi_int_disable(NRF_QSPI_Type * p_reg, uint32_t mask)
{
    p_reg->INTENCLR = mask;
}

NRF_STATIC_INLINE uint32_t nrf_qspi_int_enable_check(NRF_QSPI_Type const * p_reg, uint32_t mask)
{
    return p_reg->INTENSET & mask;
}

NRF_STATIC_INLINE void nrf_qspi_enable(NRF_QSPI_Type * p_reg)
{
    p_reg->ENABLE = (QSPI_ENABLE_ENABLE_Enabled << QSPI_ENABLE_ENABLE_Pos);
}

NRF_STATIC_INLINE void nrf_qspi_disable(NRF_QSPI_Type * p_reg)
{
    if (nrf52_errata_122())
    {
        // Workaround for anomaly 122: "QSPI: QSPI uses current after being disabled".
        *(volatile uint32_t *)0x40029054ul = 1ul;
    }
    p_reg->ENABLE = (QSPI_ENABLE_ENABLE_Disabled << QSPI_ENABLE_ENABLE_Pos);
}

NRF_STATIC_INLINE void nrf_qspi_pins_set(NRF_QSPI_Type * p_reg, nrf_qspi_pins_t const * p_pins)
{
    p_reg->PSEL.SCK = NRF_QSPI_PIN_VAL(p_pins->sck_pin);
    p_reg->PSEL.CSN = NRF_QSPI_PIN_VAL(p_pins->csn_pin);
    p_reg->PSEL.IO0 = NRF_QSPI_PIN_VAL(p_pins->io0_pin);
    p_reg->PSEL.IO1 = NRF_QSPI_PIN_VAL(p_pins->io1_pin);
    p_reg->PSEL.IO2 = NRF_QSPI_PIN_VAL(p_pins->io2_pin);
    p_reg->PSEL.IO3 = NRF_QSPI_PIN_VAL(p_pins->io3_pin);
}

NRF_STATIC_INLINE void nrf_qspi_xip_offset_set(NRF_QSPI_Type * p_reg,
                                               uint32_t        xip_offset)
{
    p_reg->XIPOFFSET = xip_offset;
}

NRF_STATIC_INLINE void nrf_qspi_ifconfig0_set(NRF_QSPI_Type *              p_reg,
                                              nrf_qspi_prot_conf_t const * p_config)
{
    uint32_t config = p_config->readoc;
    config |= ((uint32_t)p_config->writeoc)    << QSPI_IFCONFIG0_WRITEOC_Pos;
    config |= ((uint32_t)p_config->addrmode)   << QSPI_IFCONFIG0_ADDRMODE_Pos;
    config |= (p_config->dpmconfig ? 1U : 0U ) << QSPI_IFCONFIG0_DPMENABLE_Pos;

    p_reg->IFCONFIG0 = config;
}

NRF_STATIC_INLINE void nrf_qspi_ifconfig1_set(NRF_QSPI_Type *             p_reg,
                                              nrf_qspi_phy_conf_t const * p_config)
{
    // IFCONFIG1 mask for reserved fields in the register.
    uint32_t config = p_reg->IFCONFIG1 & 0x00FFFF00;
    config |= p_config->sck_delay;
    config |= (p_config->dpmen ? 1U : 0U)      << QSPI_IFCONFIG1_DPMEN_Pos;
    config |= ((uint32_t)(p_config->spi_mode)) << QSPI_IFCONFIG1_SPIMODE_Pos;
    config |= ((uint32_t)(p_config->sck_freq)) << QSPI_IFCONFIG1_SCKFREQ_Pos;

    p_reg->IFCONFIG1 = config;
}

NRF_STATIC_INLINE void nrf_qspi_addrconfig_set(NRF_QSPI_Type *                    p_reg,
                                               nrf_qspi_addrconfig_conf_t const * p_config)
{
    uint32_t config = p_config->opcode;
    config |= ((uint32_t)p_config->byte0)   << QSPI_ADDRCONF_BYTE0_Pos;
    config |= ((uint32_t)p_config->byte1)   << QSPI_ADDRCONF_BYTE1_Pos;
    config |= ((uint32_t)(p_config->mode))  << QSPI_ADDRCONF_MODE_Pos;
    config |= (p_config->wipwait ? 1U : 0U) << QSPI_ADDRCONF_WIPWAIT_Pos;
    config |= (p_config->wren    ? 1U : 0U) << QSPI_ADDRCONF_WREN_Pos;

    p_reg->ADDRCONF = config;
}

NRF_STATIC_INLINE void nrf_qspi_write_buffer_set(NRF_QSPI_Type * p_reg,
                                                 void const    * p_buffer,
                                                 uint32_t        length,
                                                 uint32_t        dest_addr)
{
    p_reg->WRITE.DST = dest_addr;
    p_reg->WRITE.SRC = (uint32_t) p_buffer;
    p_reg->WRITE.CNT = length;
}

NRF_STATIC_INLINE void nrf_qspi_read_buffer_set(NRF_QSPI_Type * p_reg,
                                                void          * p_buffer,
                                                uint32_t        length,
                                                uint32_t        src_addr)
{
    p_reg->READ.SRC = src_addr;
    p_reg->READ.DST = (uint32_t) p_buffer;
    p_reg->READ.CNT = length;
}

NRF_STATIC_INLINE void nrf_qspi_erase_ptr_set(NRF_QSPI_Type *      p_reg,
                                              uint32_t             erase_addr,
                                              nrf_qspi_erase_len_t len)
{
    p_reg->ERASE.PTR = erase_addr;
    p_reg->ERASE.LEN = len;
}

NRF_STATIC_INLINE uint32_t nrf_qspi_status_reg_get(NRF_QSPI_Type const * p_reg)
{
    return p_reg->STATUS;
}

NRF_STATIC_INLINE uint8_t nrf_qspi_sreg_get(NRF_QSPI_Type const * p_reg)
{
    return (uint8_t)(p_reg->STATUS & QSPI_STATUS_SREG_Msk) >> QSPI_STATUS_SREG_Pos;
}

NRF_STATIC_INLINE bool nrf_qspi_busy_check(NRF_QSPI_Type const * p_reg)
{
    return ((p_reg->STATUS & QSPI_STATUS_READY_Msk) >>
            QSPI_STATUS_READY_Pos) == QSPI_STATUS_READY_BUSY;
}

NRF_STATIC_INLINE void nrf_qspi_cinstrdata_set(NRF_QSPI_Type *       p_reg,
                                               nrf_qspi_cinstr_len_t length,
                                               void const *          p_tx_data)
{
    uint32_t reg = 0;
    uint8_t const *p_tx_data_8 = (uint8_t const *) p_tx_data;

    // Load custom instruction.
    switch (length)
    {
        case NRF_QSPI_CINSTR_LEN_9B:
            reg |= ((uint32_t)p_tx_data_8[7]) << QSPI_CINSTRDAT1_BYTE7_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_8B:
            reg |= ((uint32_t)p_tx_data_8[6]) << QSPI_CINSTRDAT1_BYTE6_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_7B:
            reg |= ((uint32_t)p_tx_data_8[5]) << QSPI_CINSTRDAT1_BYTE5_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_6B:
            reg |= ((uint32_t)p_tx_data_8[4]);
            p_reg->CINSTRDAT1 = reg;
            reg = 0;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_5B:
            reg |= ((uint32_t)p_tx_data_8[3]) << QSPI_CINSTRDAT0_BYTE3_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_4B:
            reg |= ((uint32_t)p_tx_data_8[2]) << QSPI_CINSTRDAT0_BYTE2_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_3B:
            reg |= ((uint32_t)p_tx_data_8[1]) << QSPI_CINSTRDAT0_BYTE1_Pos;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_2B:
            reg |= ((uint32_t)p_tx_data_8[0]);
            p_reg->CINSTRDAT0 = reg;
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_1B:
            /* Send only opcode. Case to avoid compiler warnings. */
            break;
        default:
            break;
    }
}

NRF_STATIC_INLINE void nrf_qspi_cinstrdata_get(NRF_QSPI_Type const * p_reg,
                                               nrf_qspi_cinstr_len_t length,
                                               void *                p_rx_data)
{
    uint8_t *p_rx_data_8 = (uint8_t *) p_rx_data;

    uint32_t reg1 = p_reg->CINSTRDAT1;
    uint32_t reg0 = p_reg->CINSTRDAT0;
    switch (length)
    {
        case NRF_QSPI_CINSTR_LEN_9B:
            p_rx_data_8[7] = (uint8_t)(reg1 >> QSPI_CINSTRDAT1_BYTE7_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_8B:
            p_rx_data_8[6] = (uint8_t)(reg1 >> QSPI_CINSTRDAT1_BYTE6_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_7B:
            p_rx_data_8[5] = (uint8_t)(reg1 >> QSPI_CINSTRDAT1_BYTE5_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_6B:
            p_rx_data_8[4] = (uint8_t)(reg1);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_5B:
            p_rx_data_8[3] = (uint8_t)(reg0 >> QSPI_CINSTRDAT0_BYTE3_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_4B:
            p_rx_data_8[2] = (uint8_t)(reg0 >> QSPI_CINSTRDAT0_BYTE2_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_3B:
            p_rx_data_8[1] = (uint8_t)(reg0 >> QSPI_CINSTRDAT0_BYTE1_Pos);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_2B:
            p_rx_data_8[0] = (uint8_t)(reg0);
            /* fall-through */
        case NRF_QSPI_CINSTR_LEN_1B:
            /* Send only opcode. Case to avoid compiler warnings. */
            break;
        default:
            break;
    }
}

NRF_STATIC_INLINE void nrf_qspi_cinstr_transfer_start(NRF_QSPI_Type *                p_reg,
                                                      nrf_qspi_cinstr_conf_t const * p_config)
{
    p_reg->CINSTRCONF = (((uint32_t)p_config->opcode    << QSPI_CINSTRCONF_OPCODE_Pos) |
                         ((uint32_t)p_config->length    << QSPI_CINSTRCONF_LENGTH_Pos) |
                         ((uint32_t)p_config->io2_level << QSPI_CINSTRCONF_LIO2_Pos) |
                         ((uint32_t)p_config->io3_level << QSPI_CINSTRCONF_LIO3_Pos) |
                         ((uint32_t)p_config->wipwait   << QSPI_CINSTRCONF_WIPWAIT_Pos) |
                         ((uint32_t)p_config->wren      << QSPI_CINSTRCONF_WREN_Pos));
}

NRF_STATIC_INLINE void nrf_qspi_cinstr_long_transfer_start(NRF_QSPI_Type *                p_reg,
                                                           nrf_qspi_cinstr_conf_t const * p_config)
{
    p_reg->CINSTRCONF = (((uint32_t)p_config->opcode    << QSPI_CINSTRCONF_OPCODE_Pos) |
                         ((uint32_t)p_config->length    << QSPI_CINSTRCONF_LENGTH_Pos) |
                         ((uint32_t)p_config->io2_level << QSPI_CINSTRCONF_LIO2_Pos) |
                         ((uint32_t)p_config->io3_level << QSPI_CINSTRCONF_LIO3_Pos) |
                         ((uint32_t)p_config->wipwait   << QSPI_CINSTRCONF_WIPWAIT_Pos) |
                         ((uint32_t)p_config->wren      << QSPI_CINSTRCONF_WREN_Pos) |
                         (QSPI_CINSTRCONF_LFEN_Msk));
}

NRF_STATIC_INLINE bool nrf_qspi_cinstr_long_transfer_is_ongoing(NRF_QSPI_Type const * p_reg)
{
    return (bool)((p_reg->CINSTRCONF & (QSPI_CINSTRCONF_LFEN_Msk | QSPI_CINSTRCONF_LFSTOP_Msk))
                   == QSPI_CINSTRCONF_LFEN_Msk);
}

NRF_STATIC_INLINE void nrf_qspi_cinstr_long_transfer_continue(NRF_QSPI_Type *       p_reg,
                                                              nrf_qspi_cinstr_len_t length,
                                                              bool                  finalize)
{
    uint32_t mask = (((uint32_t)length << QSPI_CINSTRCONF_LENGTH_Pos) | (QSPI_CINSTRCONF_LFEN_Msk));
    mask |= (finalize ? QSPI_CINSTRCONF_LFSTOP_Msk : 0);

    p_reg->CINSTRCONF = mask;
}

#if NRF_QSPI_HAS_XIPEN
NRF_STATIC_INLINE void nrf_qspi_xip_set(NRF_QSPI_Type * p_reg, bool enable)
{
    p_reg->XIPEN = (enable ? QSPI_XIPEN_XIPEN_Enable << QSPI_XIPEN_XIPEN_Pos
                           : QSPI_XIPEN_XIPEN_Disable << QSPI_XIPEN_XIPEN_Pos);
}
#endif

#if NRF_QSPI_HAS_XIP_ENC
NRF_STATIC_INLINE void nrf_qspi_xip_encryption_configure(NRF_QSPI_Type *               p_reg,
                                                         nrf_qspi_encryption_t const * p_cfg)
{
    p_reg->XIP_ENC.KEY0 = p_cfg->key[0];
    p_reg->XIP_ENC.KEY1 = p_cfg->key[1];
    p_reg->XIP_ENC.KEY2 = p_cfg->key[2];
    p_reg->XIP_ENC.KEY3 = p_cfg->key[3];
    p_reg->XIP_ENC.NONCE0 = p_cfg->nonce[0];
    p_reg->XIP_ENC.NONCE1 = p_cfg->nonce[1];
    p_reg->XIP_ENC.NONCE2 = p_cfg->nonce[2];
}

NRF_STATIC_INLINE void nrf_qspi_xip_encryption_set(NRF_QSPI_Type * p_reg, bool enable)
{
    p_reg->XIP_ENC.ENABLE =
        (enable ? QSPI_XIP_ENC_ENABLE_ENABLE_Enabled << QSPI_XIP_ENC_ENABLE_ENABLE_Pos
                : QSPI_XIP_ENC_ENABLE_ENABLE_Disabled << QSPI_XIP_ENC_ENABLE_ENABLE_Pos);
}
#endif

#if NRF_QSPI_HAS_DMA_ENC
NRF_STATIC_INLINE void nrf_qspi_dma_encryption_configure(NRF_QSPI_Type *               p_reg,
                                                         nrf_qspi_encryption_t const * p_cfg)
{
    p_reg->DMA_ENC.KEY0 = p_cfg->key[0];
    p_reg->DMA_ENC.KEY1 = p_cfg->key[1];
    p_reg->DMA_ENC.KEY2 = p_cfg->key[2];
    p_reg->DMA_ENC.KEY3 = p_cfg->key[3];
    p_reg->DMA_ENC.NONCE0 = p_cfg->nonce[0];
    p_reg->DMA_ENC.NONCE1 = p_cfg->nonce[1];
    p_reg->DMA_ENC.NONCE2 = p_cfg->nonce[2];
}

NRF_STATIC_INLINE void nrf_qspi_dma_encryption_set(NRF_QSPI_Type * p_reg, bool enable)
{
    p_reg->DMA_ENC.ENABLE =
        (enable ? QSPI_DMA_ENC_ENABLE_ENABLE_Enabled << QSPI_DMA_ENC_ENABLE_ENABLE_Pos
                : QSPI_DMA_ENC_ENABLE_ENABLE_Disabled << QSPI_DMA_ENC_ENABLE_ENABLE_Pos);
}
#endif
#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_QSPI_H__
