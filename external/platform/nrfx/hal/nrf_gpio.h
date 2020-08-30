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

#ifndef NRF_GPIO_H__
#define NRF_GPIO_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NRF_P0
#define NRF_P0 NRF_GPIO
#endif

#if (GPIO_COUNT == 1)
#define NUMBER_OF_PINS (P0_PIN_NUM)
#define GPIO_REG_LIST  {NRF_P0}
#elif (GPIO_COUNT == 2)
#define NUMBER_OF_PINS (P0_PIN_NUM + P1_PIN_NUM)
#define GPIO_REG_LIST  {NRF_P0, NRF_P1}
#else
#error "Not supported."
#endif

#if defined(NRF52820_XXAA)
#include <nrf_erratas.h>
#endif

/**
 * @defgroup nrf_gpio_hal GPIO HAL
 * @{
 * @ingroup nrf_gpio
 * @brief   Hardware access layer for managing the GPIO peripheral.
 */

#if defined(GPIO_LATCH_PIN0_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether the functionality of latching GPIO state change is present. */
#define NRF_GPIO_LATCH_PRESENT
#endif

/** @brief Macro for mapping port and pin numbers to values understandable for nrf_gpio functions. */
#define NRF_GPIO_PIN_MAP(port, pin) (((port) << 5) | ((pin) & 0x1F))


/** @brief Pin direction definitions. */
typedef enum
{
    NRF_GPIO_PIN_DIR_INPUT  = GPIO_PIN_CNF_DIR_Input, ///< Input.
    NRF_GPIO_PIN_DIR_OUTPUT = GPIO_PIN_CNF_DIR_Output ///< Output.
} nrf_gpio_pin_dir_t;

/** @brief Connection of input buffer. */
typedef enum
{
    NRF_GPIO_PIN_INPUT_CONNECT    = GPIO_PIN_CNF_INPUT_Connect,   ///< Connect input buffer.
    NRF_GPIO_PIN_INPUT_DISCONNECT = GPIO_PIN_CNF_INPUT_Disconnect ///< Disconnect input buffer.
} nrf_gpio_pin_input_t;

/**
 * @brief Enumerator used for selecting the pin to be pulled down or up at the time of pin
 * configuration.
 */
typedef enum
{
    NRF_GPIO_PIN_NOPULL   = GPIO_PIN_CNF_PULL_Disabled, ///<  Pin pull-up resistor disabled.
    NRF_GPIO_PIN_PULLDOWN = GPIO_PIN_CNF_PULL_Pulldown, ///<  Pin pull-down resistor enabled.
    NRF_GPIO_PIN_PULLUP   = GPIO_PIN_CNF_PULL_Pullup,   ///<  Pin pull-up resistor enabled.
} nrf_gpio_pin_pull_t;

/** @brief Enumerator used for selecting output drive mode. */
typedef enum
{
    NRF_GPIO_PIN_S0S1 = GPIO_PIN_CNF_DRIVE_S0S1, ///< Standard '0', standard '1'.
    NRF_GPIO_PIN_H0S1 = GPIO_PIN_CNF_DRIVE_H0S1, ///< High drive '0', standard '1'.
    NRF_GPIO_PIN_S0H1 = GPIO_PIN_CNF_DRIVE_S0H1, ///< Standard '0', high drive '1'.
    NRF_GPIO_PIN_H0H1 = GPIO_PIN_CNF_DRIVE_H0H1, ///< High drive '0', high drive '1'.
    NRF_GPIO_PIN_D0S1 = GPIO_PIN_CNF_DRIVE_D0S1, ///< Disconnect '0' standard '1'.
    NRF_GPIO_PIN_D0H1 = GPIO_PIN_CNF_DRIVE_D0H1, ///< Disconnect '0', high drive '1'.
    NRF_GPIO_PIN_S0D1 = GPIO_PIN_CNF_DRIVE_S0D1, ///< Standard '0', disconnect '1'.
    NRF_GPIO_PIN_H0D1 = GPIO_PIN_CNF_DRIVE_H0D1, ///< High drive '0', disconnect '1'.
#if defined(GPIO_PIN_CNF_DRIVE_E0S1) || defined(__NRFX_DOXYGEN__)
    NRF_GPIO_PIN_E0S1 = GPIO_PIN_CNF_DRIVE_E0S1, ///< Extra high drive '0', standard '1'.
#endif
#if defined(GPIO_PIN_CNF_DRIVE_S0E1) || defined(__NRFX_DOXYGEN__)
    NRF_GPIO_PIN_S0E1 = GPIO_PIN_CNF_DRIVE_S0E1, ///< Standard '0', extra high drive '1'.
#endif
#if defined(GPIO_PIN_CNF_DRIVE_E0E1) || defined(__NRFX_DOXYGEN__)
    NRF_GPIO_PIN_E0E1 = GPIO_PIN_CNF_DRIVE_E0E1, ///< Extra high drive '0', extra high drive '1'.
#endif
#if defined(GPIO_PIN_CNF_DRIVE_E0H1) || defined(__NRFX_DOXYGEN__)
    NRF_GPIO_PIN_E0H1 = GPIO_PIN_CNF_DRIVE_E0H1, ///< Extra high drive '0', high drive '1'.
#endif
#if defined(GPIO_PIN_CNF_DRIVE_H0E1) || defined(__NRFX_DOXYGEN__)
    NRF_GPIO_PIN_H0E1 = GPIO_PIN_CNF_DRIVE_H0E1, ///< High drive '0', extra high drive '1'.
#endif
#if defined(GPIO_PIN_CNF_DRIVE_D0E1) || defined(__NRFX_DOXYGEN__)
    NRF_GPIO_PIN_D0E1 = GPIO_PIN_CNF_DRIVE_D0E1, ///< Disconnect '0', extra high drive '1'.
#endif
#if defined(GPIO_PIN_CNF_DRIVE_E0D1) || defined(__NRFX_DOXYGEN__)
    NRF_GPIO_PIN_E0D1 = GPIO_PIN_CNF_DRIVE_E0D1, ///< Extra high drive '0', disconnect '1'.
#endif
} nrf_gpio_pin_drive_t;

/** @brief Enumerator used for selecting the pin to sense high or low level on the pin input. */
typedef enum
{
    NRF_GPIO_PIN_NOSENSE    = GPIO_PIN_CNF_SENSE_Disabled, ///<  Pin sense level disabled.
    NRF_GPIO_PIN_SENSE_LOW  = GPIO_PIN_CNF_SENSE_Low,      ///<  Pin sense low level.
    NRF_GPIO_PIN_SENSE_HIGH = GPIO_PIN_CNF_SENSE_High,     ///<  Pin sense high level.
} nrf_gpio_pin_sense_t;

#if defined(GPIO_PIN_CNF_MCUSEL_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Enumerator used for selecting the MCU/Subsystem to control the specified pin. */
typedef enum
{
    NRF_GPIO_PIN_MCUSEL_APP        = GPIO_PIN_CNF_MCUSEL_AppMCU,     ///< Pin controlled by Application MCU.
    NRF_GPIO_PIN_MCUSEL_NETWORK    = GPIO_PIN_CNF_MCUSEL_NetworkMCU, ///< Pin controlled by Network MCU.
    NRF_GPIO_PIN_MCUSEL_PERIPHERAL = GPIO_PIN_CNF_MCUSEL_Peripheral, ///< Pin controlled by dedicated peripheral.
    NRF_GPIO_PIN_MCUSEL_TND        = GPIO_PIN_CNF_MCUSEL_TND,        ///< Pin controlled by Trace and Debug Subsystem.
} nrf_gpio_pin_mcusel_t;
#endif

/**
 * @brief Function for configuring the GPIO pin range as output pins with normal drive strength.
 *        This function can be used to configure pin range as simple output with gate driving GPIO_PIN_CNF_DRIVE_S0S1 (normal cases).
 *
 * @note For configuring only one pin as output, use @ref nrf_gpio_cfg_output.
 *       Sense capability on the pin is disabled and input is disconnected from the buffer as the pins are configured as output.
 *
 * @param pin_range_start  Specifies the start number (inclusive) in the range of pin numbers to be configured (allowed values 0-30).
 * @param pin_range_end    Specifies the end number (inclusive) in the range of pin numbers to be configured (allowed values 0-30).
 */
NRF_STATIC_INLINE void nrf_gpio_range_cfg_output(uint32_t pin_range_start, uint32_t pin_range_end);

/**
 * @brief Function for configuring the GPIO pin range as input pins with given initial value set, hiding inner details.
 *        This function can be used to configure pin range as simple input.
 *
 * @note  For configuring only one pin as input, use @ref nrf_gpio_cfg_input.
 *        Sense capability on the pin is disabled and input is connected to buffer so that the GPIO->IN register is readable.
 *
 * @param pin_range_start  Specifies the start number (inclusive) in the range of pin numbers to be configured (allowed values 0-30).
 * @param pin_range_end    Specifies the end number (inclusive) in the range of pin numbers to be configured (allowed values 0-30).
 * @param pull_config      State of the pin range pull resistor (no pull, pulled down, or pulled high).
 */
NRF_STATIC_INLINE void nrf_gpio_range_cfg_input(uint32_t            pin_range_start,
                                                uint32_t            pin_range_end,
                                                nrf_gpio_pin_pull_t pull_config);

/**
 * @brief Pin configuration function.
 *
 * The main pin configuration function.
 * This function allows to set any aspect in PIN_CNF register.
 *
 * @param pin_number Specifies the pin number.
 * @param dir        Pin direction.
 * @param input      Connect or disconnect the input buffer.
 * @param pull       Pull configuration.
 * @param drive      Drive configuration.
 * @param sense      Pin sensing mechanism.
 */
NRF_STATIC_INLINE void nrf_gpio_cfg(
    uint32_t             pin_number,
    nrf_gpio_pin_dir_t   dir,
    nrf_gpio_pin_input_t input,
    nrf_gpio_pin_pull_t  pull,
    nrf_gpio_pin_drive_t drive,
    nrf_gpio_pin_sense_t sense);

/**
 * @brief Function for configuring the given GPIO pin number as output, hiding inner details.
 *        This function can be used to configure a pin as simple output with gate driving GPIO_PIN_CNF_DRIVE_S0S1 (normal cases).
 *
 * @note  Sense capability on the pin is disabled and input is disconnected from the buffer as the pins are configured as output.
 *
 * @param pin_number Specifies the pin number.
 */
NRF_STATIC_INLINE void nrf_gpio_cfg_output(uint32_t pin_number);

/**
 * @brief Function for configuring the given GPIO pin number as input, hiding inner details.
 *        This function can be used to configure a pin as simple input.
 *
 * @note  Sense capability on the pin is disabled and input is connected to buffer so that the GPIO->IN register is readable.
 *
 * @param pin_number  Specifies the pin number.
 * @param pull_config State of the pin range pull resistor (no pull, pulled down, or pulled high).
 */
NRF_STATIC_INLINE void nrf_gpio_cfg_input(uint32_t pin_number, nrf_gpio_pin_pull_t pull_config);

/**
 * @brief Function for resetting pin configuration to its default state.
 *
 * @param pin_number Specifies the pin number.
 */
NRF_STATIC_INLINE void nrf_gpio_cfg_default(uint32_t pin_number);

/**
 * @brief Function for configuring the given GPIO pin number as a watcher. Only input is connected.
 *
 * @param pin_number Specifies the pin number.
 *
 */
NRF_STATIC_INLINE void nrf_gpio_cfg_watcher(uint32_t pin_number);

/**
 * @brief Function for disconnecting input for the given GPIO.
 *
 * @param pin_number Specifies the pin number.
 */
NRF_STATIC_INLINE void nrf_gpio_input_disconnect(uint32_t pin_number);

/**
 * @brief Function for configuring the given GPIO pin number as input, hiding inner details.
 *        This function can be used to configure pin range as simple input.
 *        Sense capability on the pin is configurable and input is connected to buffer so that the GPIO->IN register is readable.
 *
 * @param pin_number   Specifies the pin number.
 * @param pull_config  State of the pin pull resistor (no pull, pulled down, or pulled high).
 * @param sense_config Sense level of the pin (no sense, sense low, or sense high).
 */
NRF_STATIC_INLINE void nrf_gpio_cfg_sense_input(uint32_t             pin_number,
                                                nrf_gpio_pin_pull_t  pull_config,
                                                nrf_gpio_pin_sense_t sense_config);

/**
 * @brief Function for configuring sense level for the given GPIO.
 *
 * @param pin_number   Specifies the pin number.
 * @param sense_config Sense configuration.
 */
NRF_STATIC_INLINE void nrf_gpio_cfg_sense_set(uint32_t             pin_number,
                                              nrf_gpio_pin_sense_t sense_config);

/**
 * @brief Function for setting the direction for a GPIO pin.
 *
 * @param pin_number Specifies the pin number for which to set the direction.
 * @param direction  Specifies the direction.
 */
NRF_STATIC_INLINE void nrf_gpio_pin_dir_set(uint32_t pin_number, nrf_gpio_pin_dir_t direction);

/**
 * @brief Function for setting a GPIO pin.
 *
 * For this function to have any effect, the pin must be configured as an output.
 *
 * @param pin_number Specifies the pin number to be set.
 */
NRF_STATIC_INLINE void nrf_gpio_pin_set(uint32_t pin_number);

/**
 * @brief Function for clearing a GPIO pin.
 *
 * For this function to have any effect, the pin must be configured as an output.
 *
 * @param pin_number Specifies the pin number to clear.
 */
NRF_STATIC_INLINE void nrf_gpio_pin_clear(uint32_t pin_number);

/**
 * @brief Function for toggling a GPIO pin.
 *
 * For this function to have any effect, the pin must be configured as an output.
 *
 * @param pin_number Specifies the pin number to toggle.
 */
NRF_STATIC_INLINE void nrf_gpio_pin_toggle(uint32_t pin_number);

/**
 * @brief Function for writing a value to a GPIO pin.
 *
 * For this function to have any effect, the pin must be configured as an output.
 *
 * @param pin_number Specifies the pin number to write.
 * @param value      Specifies the value to be written to the pin.
 * @arg 0 Clears the pin.
 * @arg >=1 Sets the pin.
 */
NRF_STATIC_INLINE void nrf_gpio_pin_write(uint32_t pin_number, uint32_t value);

/**
 * @brief Function for reading the input level of a GPIO pin.
 *
 * If the value returned by this function is to be valid, the pin's input buffer must be connected.
 *
 * @param pin_number Specifies the pin number to read.
 *
 * @return 0 if the pin input level is low. Positive value if the pin is high.
 */
NRF_STATIC_INLINE uint32_t nrf_gpio_pin_read(uint32_t pin_number);

/**
 * @brief Function for reading the output level of a GPIO pin.
 *
 * @param pin_number Specifies the pin number to read.
 *
 * @return 0 if the pin output level is low. Positive value if pin output is high.
 */
NRF_STATIC_INLINE uint32_t nrf_gpio_pin_out_read(uint32_t pin_number);

/**
 * @brief Function for reading the sense configuration of a GPIO pin.
 *
 * @param pin_number Specifies the pin number to read.
 *
 * @return Sense configuration.
 */
NRF_STATIC_INLINE nrf_gpio_pin_sense_t nrf_gpio_pin_sense_get(uint32_t pin_number);

/**
 * @brief Function for reading the direction configuration of a GPIO pin.
 *
 * @param pin_number Specifies the pin number to read.
 *
 * @return Direction configuration.
 */
NRF_STATIC_INLINE nrf_gpio_pin_dir_t nrf_gpio_pin_dir_get(uint32_t pin_number);

/**
 * @brief Function for reading the status of GPIO pin input buffer.
 *
 * @param pin_number Pin number to be read.
 *
 * @retval Input buffer configuration.
 */
NRF_STATIC_INLINE nrf_gpio_pin_input_t nrf_gpio_pin_input_get(uint32_t pin_number);

/**
 * @brief Function for reading the pull configuration of a GPIO pin.
 *
 * @param pin_number Specifies the pin number to read.
 *
 * @retval Pull configuration.
 */
NRF_STATIC_INLINE nrf_gpio_pin_pull_t nrf_gpio_pin_pull_get(uint32_t pin_number);

/**
 * @brief Function for setting output direction on the selected pins on the given port.
 *
 * @param p_reg    Pointer to the structure of registers of the peripheral.
 * @param out_mask Mask specifying the pins to set as output.
 */
NRF_STATIC_INLINE void nrf_gpio_port_dir_output_set(NRF_GPIO_Type * p_reg, uint32_t out_mask);

/**
 * @brief Function for setting input direction on selected pins on a given port.
 *
 * @param p_reg   Pointer to the structure of registers of the peripheral.
 * @param in_mask Mask that specifies the pins to be set as input.
 */
NRF_STATIC_INLINE void nrf_gpio_port_dir_input_set(NRF_GPIO_Type * p_reg, uint32_t in_mask);

/**
 * @brief Function for writing the direction configuration of the GPIO pins in the given port.
 *
 * @param p_reg    Pointer to the structure of registers of the peripheral.
 * @param dir_mask Mask that specifies the direction of pins. Bit set means that the given pin is configured as output.
 */
NRF_STATIC_INLINE void nrf_gpio_port_dir_write(NRF_GPIO_Type * p_reg, uint32_t dir_mask);

/**
 * @brief Function for reading the direction configuration of a GPIO port.
 *
 * @param p_reg Pointer to the structure of registers of the peripheral.
 *
 * @return Pin configuration of the current direction settings. Bit set means that the given pin is configured as output.
 */
NRF_STATIC_INLINE uint32_t nrf_gpio_port_dir_read(NRF_GPIO_Type const * p_reg);

/**
 * @brief Function for reading the input signals of the GPIO pins on the given port.
 *
 * @param p_reg Pointer to the peripheral registers structure.
 *
 * @return Port input values.
 */
NRF_STATIC_INLINE uint32_t nrf_gpio_port_in_read(NRF_GPIO_Type const * p_reg);

/**
 * @brief Function for reading the output signals of the GPIO pins on the given port.
 *
 * @param p_reg Pointer to the peripheral registers structure.
 *
 * @return Port output values.
 */
NRF_STATIC_INLINE uint32_t nrf_gpio_port_out_read(NRF_GPIO_Type const * p_reg);

/**
 * @brief Function for writing the GPIO pins output on a given port.
 *
 * @param p_reg Pointer to the structure of registers of the peripheral.
 * @param value Output port mask.
 */
NRF_STATIC_INLINE void nrf_gpio_port_out_write(NRF_GPIO_Type * p_reg, uint32_t value);

/**
 * @brief Function for setting high level on selected the GPIO pins on the given port.
 *
 * @param p_reg    Pointer to the structure of registers of the peripheral.
 * @param set_mask Mask with pins to be set as logical high level.
 */
NRF_STATIC_INLINE void nrf_gpio_port_out_set(NRF_GPIO_Type * p_reg, uint32_t set_mask);

/**
 * @brief Function for setting low level on selected the GPIO pins on the given port.
 *
 * @param p_reg    Pointer to the structure of registers of the peripheral.
 * @param clr_mask Mask with pins to be set as logical low level.
 */
NRF_STATIC_INLINE void nrf_gpio_port_out_clear(NRF_GPIO_Type * p_reg, uint32_t clr_mask);

/**
 * @brief Function for reading pin state of multiple consecutive ports.
 *
 * @param start_port Index of the first port to read.
 * @param length     Number of ports to read.
 * @param p_masks    Pointer to output array where port states will be stored.
 */
NRF_STATIC_INLINE void nrf_gpio_ports_read(uint32_t   start_port,
                                           uint32_t   length,
                                           uint32_t * p_masks);

#if defined(NRF_GPIO_LATCH_PRESENT)
/**
 * @brief Function for reading latch state of multiple consecutive ports.
 *
 * @param start_port Index of the first port to read.
 * @param length     Number of ports to read.
 * @param p_masks    Pointer to output array where latch states will be stored.
 */
NRF_STATIC_INLINE void nrf_gpio_latches_read(uint32_t   start_port,
                                             uint32_t   length,
                                             uint32_t * p_masks);

/**
 * @brief Function for reading and immediate clearing latch state of multiple consecutive ports.
 *
 * @param start_port Index of the first port to read and clear.
 * @param length     Number of ports to read and clear.
 * @param p_masks    Pointer to output array where latch states will be stored.
 */
NRF_STATIC_INLINE void nrf_gpio_latches_read_and_clear(uint32_t   start_port,
                                                       uint32_t   length,
                                                       uint32_t * p_masks);

/**
 * @brief Function for reading latch state of single pin.
 *
 * @param pin_number Pin number.
 *
 * @return 0 if latch is not set. Positive value otherwise.
 */
NRF_STATIC_INLINE uint32_t nrf_gpio_pin_latch_get(uint32_t pin_number);

/**
 * @brief Function for clearing latch state of a single pin.
 *
 * @param pin_number Pin number.
 */
NRF_STATIC_INLINE void nrf_gpio_pin_latch_clear(uint32_t pin_number);
#endif // defined(NRF_GPIO_LATCH_PRESENT)

#if defined(GPIO_PIN_CNF_MCUSEL_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for selecting the MCU to control a GPIO pin.
 *
 * @param pin_number Pin_number.
 * @param mcu        MCU to control the pin.
 */
NRF_STATIC_INLINE void nrf_gpio_pin_mcu_select(uint32_t pin_number, nrf_gpio_pin_mcusel_t mcu);
#endif

/**
 * @brief Function for checking if provided pin is present on the MCU.
 *
 * @param[in] pin_number Number of the pin to be checked.
 *
 * @retval true  Pin is present.
 * @retval false Pin is not present.
 */
NRF_STATIC_INLINE bool nrf_gpio_pin_present_check(uint32_t pin_number);

/**
 * @brief Function for extracting port number and the relative pin number
 *        from the absolute pin number.
 *
 * @param[in,out] p_pin Pointer to the absolute pin number overridden by the pin number
 *                      that is relative to the port.
 *
 * @return Port number.
*/
NRF_STATIC_INLINE uint32_t nrf_gpio_pin_port_number_extract(uint32_t * p_pin);

#ifndef NRF_DECLARE_ONLY

/**
 * @brief Function for extracting port and the relative pin number from the absolute pin number.
 *
 * @param[in,out] p_pin Pointer to the absolute pin number overridden by the pin number
 *                      that is relative to the port.
 *
 * @return Pointer to port register set.
 */
NRF_STATIC_INLINE NRF_GPIO_Type * nrf_gpio_pin_port_decode(uint32_t * p_pin)
{
    NRFX_ASSERT(nrf_gpio_pin_present_check(*p_pin));

    switch (nrf_gpio_pin_port_number_extract(p_pin))
    {
        default:
            NRFX_ASSERT(0);
#if defined(P0_FEATURE_PINS_PRESENT)
        case 0: return NRF_P0;
#endif
#if defined(P1_FEATURE_PINS_PRESENT)
        case 1: return NRF_P1;
#endif
    }
}


NRF_STATIC_INLINE void nrf_gpio_range_cfg_output(uint32_t pin_range_start, uint32_t pin_range_end)
{
    for (; pin_range_start <= pin_range_end; pin_range_start++)
    {
        nrf_gpio_cfg_output(pin_range_start);
    }
}


NRF_STATIC_INLINE void nrf_gpio_range_cfg_input(uint32_t            pin_range_start,
                                                uint32_t            pin_range_end,
                                                nrf_gpio_pin_pull_t pull_config)
{
    for (; pin_range_start <= pin_range_end; pin_range_start++)
    {
        nrf_gpio_cfg_input(pin_range_start, pull_config);
    }
}


NRF_STATIC_INLINE void nrf_gpio_cfg(
    uint32_t             pin_number,
    nrf_gpio_pin_dir_t   dir,
    nrf_gpio_pin_input_t input,
    nrf_gpio_pin_pull_t  pull,
    nrf_gpio_pin_drive_t drive,
    nrf_gpio_pin_sense_t sense)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);

    reg->PIN_CNF[pin_number] = ((uint32_t)dir << GPIO_PIN_CNF_DIR_Pos)
                               | ((uint32_t)input << GPIO_PIN_CNF_INPUT_Pos)
                               | ((uint32_t)pull << GPIO_PIN_CNF_PULL_Pos)
                               | ((uint32_t)drive << GPIO_PIN_CNF_DRIVE_Pos)
                               | ((uint32_t)sense << GPIO_PIN_CNF_SENSE_Pos);
}


NRF_STATIC_INLINE void nrf_gpio_cfg_output(uint32_t pin_number)
{
    nrf_gpio_cfg(
        pin_number,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE);
}


NRF_STATIC_INLINE void nrf_gpio_cfg_input(uint32_t pin_number, nrf_gpio_pin_pull_t pull_config)
{
    nrf_gpio_cfg(
        pin_number,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        pull_config,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE);
}


NRF_STATIC_INLINE void nrf_gpio_cfg_default(uint32_t pin_number)
{
    nrf_gpio_cfg(
        pin_number,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE);
}


NRF_STATIC_INLINE void nrf_gpio_cfg_watcher(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);
    uint32_t cnf = reg->PIN_CNF[pin_number] & ~GPIO_PIN_CNF_INPUT_Msk;

    reg->PIN_CNF[pin_number] = cnf | (GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos);
}


NRF_STATIC_INLINE void nrf_gpio_input_disconnect(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);
    uint32_t cnf = reg->PIN_CNF[pin_number] & ~GPIO_PIN_CNF_INPUT_Msk;

    reg->PIN_CNF[pin_number] = cnf | (GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos);
}


NRF_STATIC_INLINE void nrf_gpio_cfg_sense_input(uint32_t             pin_number,
                                                nrf_gpio_pin_pull_t  pull_config,
                                                nrf_gpio_pin_sense_t sense_config)
{
    nrf_gpio_cfg(
        pin_number,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        pull_config,
        NRF_GPIO_PIN_S0S1,
        sense_config);
}


NRF_STATIC_INLINE void nrf_gpio_cfg_sense_set(uint32_t             pin_number,
                                              nrf_gpio_pin_sense_t sense_config)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);
    uint32_t cnf = reg->PIN_CNF[pin_number] & ~GPIO_PIN_CNF_SENSE_Msk;

    reg->PIN_CNF[pin_number] = cnf | (sense_config << GPIO_PIN_CNF_SENSE_Pos);
}


NRF_STATIC_INLINE void nrf_gpio_pin_dir_set(uint32_t pin_number, nrf_gpio_pin_dir_t direction)
{
    if (direction == NRF_GPIO_PIN_DIR_INPUT)
    {
        nrf_gpio_cfg(
            pin_number,
            NRF_GPIO_PIN_DIR_INPUT,
            NRF_GPIO_PIN_INPUT_CONNECT,
            NRF_GPIO_PIN_NOPULL,
            NRF_GPIO_PIN_S0S1,
            NRF_GPIO_PIN_NOSENSE);
    }
    else
    {
        NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);
        reg->DIRSET = (1UL << pin_number);
    }
}


NRF_STATIC_INLINE void nrf_gpio_pin_set(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);

    nrf_gpio_port_out_set(reg, 1UL << pin_number);
}


NRF_STATIC_INLINE void nrf_gpio_pin_clear(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);

    nrf_gpio_port_out_clear(reg, 1UL << pin_number);
}


NRF_STATIC_INLINE void nrf_gpio_pin_toggle(uint32_t pin_number)
{
    NRF_GPIO_Type * reg        = nrf_gpio_pin_port_decode(&pin_number);
    uint32_t        pins_state = reg->OUT;

    reg->OUTSET = (~pins_state & (1UL << pin_number));
    reg->OUTCLR = (pins_state & (1UL << pin_number));
}


NRF_STATIC_INLINE void nrf_gpio_pin_write(uint32_t pin_number, uint32_t value)
{
    if (value == 0)
    {
        nrf_gpio_pin_clear(pin_number);
    }
    else
    {
        nrf_gpio_pin_set(pin_number);
    }
}


NRF_STATIC_INLINE uint32_t nrf_gpio_pin_read(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);

    return ((nrf_gpio_port_in_read(reg) >> pin_number) & 1UL);
}


NRF_STATIC_INLINE uint32_t nrf_gpio_pin_out_read(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);

    return ((nrf_gpio_port_out_read(reg) >> pin_number) & 1UL);
}


NRF_STATIC_INLINE nrf_gpio_pin_sense_t nrf_gpio_pin_sense_get(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);

    return (nrf_gpio_pin_sense_t)((reg->PIN_CNF[pin_number] &
                                   GPIO_PIN_CNF_SENSE_Msk) >> GPIO_PIN_CNF_SENSE_Pos);
}


NRF_STATIC_INLINE nrf_gpio_pin_dir_t nrf_gpio_pin_dir_get(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);

    return (nrf_gpio_pin_dir_t)((reg->PIN_CNF[pin_number] &
                                 GPIO_PIN_CNF_DIR_Msk) >> GPIO_PIN_CNF_DIR_Pos);
}

NRF_STATIC_INLINE nrf_gpio_pin_input_t nrf_gpio_pin_input_get(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);

    return (nrf_gpio_pin_input_t)((reg->PIN_CNF[pin_number] &
                                   GPIO_PIN_CNF_INPUT_Msk) >> GPIO_PIN_CNF_INPUT_Pos);
}

NRF_STATIC_INLINE nrf_gpio_pin_pull_t nrf_gpio_pin_pull_get(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);

    return (nrf_gpio_pin_pull_t)((reg->PIN_CNF[pin_number] &
                                  GPIO_PIN_CNF_PULL_Msk) >> GPIO_PIN_CNF_PULL_Pos);
}


NRF_STATIC_INLINE void nrf_gpio_port_dir_output_set(NRF_GPIO_Type * p_reg, uint32_t out_mask)
{
    p_reg->DIRSET = out_mask;
}


NRF_STATIC_INLINE void nrf_gpio_port_dir_input_set(NRF_GPIO_Type * p_reg, uint32_t in_mask)
{
    p_reg->DIRCLR = in_mask;
}


NRF_STATIC_INLINE void nrf_gpio_port_dir_write(NRF_GPIO_Type * p_reg, uint32_t value)
{
    p_reg->DIR = value;
}


NRF_STATIC_INLINE uint32_t nrf_gpio_port_dir_read(NRF_GPIO_Type const * p_reg)
{
    return p_reg->DIR;
}


NRF_STATIC_INLINE uint32_t nrf_gpio_port_in_read(NRF_GPIO_Type const * p_reg)
{
    return p_reg->IN;
}


NRF_STATIC_INLINE uint32_t nrf_gpio_port_out_read(NRF_GPIO_Type const * p_reg)
{
    return p_reg->OUT;
}


NRF_STATIC_INLINE void nrf_gpio_port_out_write(NRF_GPIO_Type * p_reg, uint32_t value)
{
    p_reg->OUT = value;
}


NRF_STATIC_INLINE void nrf_gpio_port_out_set(NRF_GPIO_Type * p_reg, uint32_t set_mask)
{
    p_reg->OUTSET = set_mask;
}


NRF_STATIC_INLINE void nrf_gpio_port_out_clear(NRF_GPIO_Type * p_reg, uint32_t clr_mask)
{
    p_reg->OUTCLR = clr_mask;
}


NRF_STATIC_INLINE void nrf_gpio_ports_read(uint32_t   start_port,
                                           uint32_t   length,
                                           uint32_t * p_masks)
{
    NRF_GPIO_Type * gpio_regs[GPIO_COUNT] = GPIO_REG_LIST;

    NRFX_ASSERT(start_port + length <= GPIO_COUNT);
    uint32_t i;

    for (i = start_port; i < (start_port + length); i++)
    {
        *p_masks = nrf_gpio_port_in_read(gpio_regs[i]);
        p_masks++;
    }
}


#if defined(NRF_GPIO_LATCH_PRESENT)
NRF_STATIC_INLINE void nrf_gpio_latches_read(uint32_t   start_port,
                                             uint32_t   length,
                                             uint32_t * p_masks)
{
    NRF_GPIO_Type * gpio_regs[GPIO_COUNT] = GPIO_REG_LIST;
    uint32_t        i;

    for (i = start_port; i < (start_port + length); i++)
    {
        *p_masks = gpio_regs[i]->LATCH;
        p_masks++;
    }
}

NRF_STATIC_INLINE void nrf_gpio_latches_read_and_clear(uint32_t   start_port,
                                                       uint32_t   length,
                                                       uint32_t * p_masks)
{
    NRF_GPIO_Type * gpio_regs[GPIO_COUNT] = GPIO_REG_LIST;
    uint32_t        i;

    for (i = start_port; i < (start_port + length); i++)
    {
        *p_masks = gpio_regs[i]->LATCH;

        // The LATCH register is cleared by writing a '1' to the bit that shall be cleared.
        gpio_regs[i]->LATCH = *p_masks;

        p_masks++;
    }
}

NRF_STATIC_INLINE uint32_t nrf_gpio_pin_latch_get(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);

    return (reg->LATCH & (1 << pin_number)) ? 1 : 0;
}


NRF_STATIC_INLINE void nrf_gpio_pin_latch_clear(uint32_t pin_number)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);

    reg->LATCH = (1 << pin_number);
}
#endif // defined(NRF_GPIO_LATCH_PRESENT)

#if defined(GPIO_PIN_CNF_MCUSEL_Msk)
NRF_STATIC_INLINE void nrf_gpio_pin_mcu_select(uint32_t pin_number, nrf_gpio_pin_mcusel_t mcu)
{
    NRF_GPIO_Type * reg = nrf_gpio_pin_port_decode(&pin_number);
    uint32_t cnf = reg->PIN_CNF[pin_number] & ~GPIO_PIN_CNF_MCUSEL_Msk;
    reg->PIN_CNF[pin_number] = cnf | (mcu << GPIO_PIN_CNF_MCUSEL_Pos);
}
#endif

NRF_STATIC_INLINE bool nrf_gpio_pin_present_check(uint32_t pin_number)
{
    uint32_t port = pin_number >> 5;
    uint32_t mask = 0;

    switch (port)
    {
#ifdef P0_FEATURE_PINS_PRESENT
        case 0:
            mask = P0_FEATURE_PINS_PRESENT;
#if defined(NRF52820_XXAA) && defined(DEVELOP_IN_NRF52833)
            /* Allow use of the following additional GPIOs that are connected to LEDs and buttons
             * on the nRF52833 DK:
             * - P0.11 - Button 1
             * - P0.12 - Button 2
             * - P0.13 - LED 1
             * - P0.24 - Button 3
             * - P0.25 - Button 4
             */
            mask |= 0x03003800;
#endif // defined(NRF52820_XXAA) && defined(DEVELOP_IN_NRF52833)
            break;
#endif
#ifdef P1_FEATURE_PINS_PRESENT
        case 1:
            mask = P1_FEATURE_PINS_PRESENT;
            break;
#endif
    }

    pin_number &= 0x1F;

    return (mask & (1UL << pin_number)) ? true : false;
}

NRF_STATIC_INLINE uint32_t nrf_gpio_pin_port_number_extract(uint32_t * p_pin)
{
    uint32_t pin_number = *p_pin;
    *p_pin = pin_number & 0x1F;

    return pin_number >> 5;
}

#endif // NRF_DECLARE_ONLY

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_GPIO_H__
