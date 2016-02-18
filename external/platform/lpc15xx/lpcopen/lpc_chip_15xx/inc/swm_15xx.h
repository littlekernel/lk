/*
 * @brief LPC15xx Switch Matrix driver
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2014
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#ifndef __SWM_15XX_H_
#define __SWM_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup SWM_15XX CHIP: LPC15xx Switch Matrix Driver
 * @ingroup CHIP_15XX_Drivers
 * @{
 */

/**
 * @brief LPC15XX Switch Matrix register block structure
 */
typedef struct {
	__IO uint32_t PINASSIGN[16];	/*!< Pin Assignment register array */
	__I  uint32_t RESERVED0[96];
	__IO uint32_t PINENABLE[2];		/*!< fixed pin enable/disable registers */
} LPC_SWM_T;

/**
 * @brief LPC15XX Switch Matrix Movable pins
 */
typedef enum CHIP_SWM_PIN_MOVABLE  {
	SWM_UART0_TXD_O         = 0x00,		/*!< PINASSIGN0 - UART0 TXD Output */
	SWM_UART0_RXD_I         = 0x01,		/*!< PINASSIGN0 - UART0 RXD Input */
	SWM_UART0_RTS_O         = 0x02,		/*!< PINASSIGN0 - UART0 RTS Output */
	SWM_UART0_CTS_I         = 0x03,		/*!< PINASSIGN0 - UART0 CTS Input */
	SWM_UART0_SCLK_IO       = 0x10,		/*!< PINASSIGN1 - UART0 SCLK I/O */
	SWM_UART1_TXD_O         = 0x11,		/*!< PINASSIGN1 - UART1 TXD Output */
	SWM_UART1_RXD_I         = 0x12,		/*!< PINASSIGN1 - UART1 RXD Input */
	SWM_UART1_RTS_O         = 0x13,		/*!< PINASSIGN1 - UART1 RTS Output */
	SWM_UART1_CTS_I         = 0x20,		/*!< PINASSIGN2 - UART1 CTS Input */
	SWM_UART1_SCLK_IO       = 0x21,		/*!< PINASSIGN2 - UART1 SCLK I/O */
	SWM_UART2_TXD_O         = 0x22,		/*!< PINASSIGN2 - UART2 TXD Output */
	SWM_UART2_RXD_I         = 0x23,		/*!< PINASSIGN2 - UART2 RXD Input */
	SWM_UART2_SCLK_IO       = 0x30,		/*!< PINASSIGN3 - UART2 SCLK I/O */
	SWM_SSP0_SCK_IO         = 0x31,		/*!< PINASSIGN3 - SSP0 SCK I/O */
	SWM_SPI0_SCK_IO         = SWM_SSP0_SCK_IO,
	SWM_SSP0_MOSI_IO        = 0x32,		/*!< PINASSIGN3 - SSP0 MOSI I/O */
	SWM_SPI0_MOSI_IO        = SWM_SSP0_MOSI_IO,
	SWM_SSP0_MISO_IO        = 0x33,		/*!< PINASSIGN3 - SSP0 MISO I/O */
	SWM_SPI0_MISO_IO        = SWM_SSP0_MISO_IO,
	SWM_SSP0_SSELSN_0_IO    = 0x40,
	SWM_SPI0_SSELSN_0_IO    = SWM_SSP0_SSELSN_0_IO,
	SWM_SSP0_SSELSN_1_IO    = 0x41,
	SWM_SPI0_SSELSN_1_IO    = SWM_SSP0_SSELSN_1_IO,
	SWM_SSP0_SSELSN_2_IO    = 0x42,
	SWM_SPI0_SSELSN_2_IO    = SWM_SSP0_SSELSN_2_IO,
	SWM_SSP0_SSELSN_3_IO    = 0x43,
	SWM_SPI0_SSELSN_3_IO    = SWM_SSP0_SSELSN_3_IO,
	SWM_SSP1_SCK_IO         = 0x50,		/*!< PINASSIGN5 - SPI1 SCK I/O */
	SWM_SPI1_SCK_IO         = SWM_SSP1_SCK_IO,
	SWM_SSP1_MOSI_IO        = 0x51,		/*!< PINASSIGN5 - SPI1 MOSI I/O */
	SWM_SPI1_MOSI_IO        = SWM_SSP1_MOSI_IO,
	SWM_SSP1_MISO_IO        = 0x52,		/*!< PINASSIGN5 - SPI1 MISO I/O */
	SWM_SPI1_MISO_IO        = SWM_SSP1_MISO_IO,
	SWM_SSP1_SSELSN_0_IO    = 0x53,		/*!< PINASSIGN5 - SPI1 SSEL I/O */
	SWM_SPI1_SSELSN_0_IO    = SWM_SSP1_SSELSN_0_IO,
	SWM_SSP1_SSELSN_1_IO    = 0x60,
	SWM_SPI1_SSELSN_1_IO    = SWM_SSP1_SSELSN_1_IO,
	SWM_CAN_TD1_O           = 0x61,
	SWM_CAN_RD1_I           = 0x62,
	SWM_USB_VBUS_I          = 0x70,
	SWM_SCT0_OUT0_O         = 0x71,
	SWM_SCT0_OUT1_O         = 0x72,
	SWM_SCT0_OUT2_O         = 0x73,
	SWM_SCT1_OUT0_O         = 0x80,
	SWM_SCT1_OUT1_O         = 0x81,
	SWM_SCT1_OUT2_O         = 0x82,
	SWM_SCT2_OUT0_O         = 0x83,
	SWM_SCT2_OUT1_O         = 0x90,
	SWM_SCT2_OUT2_O         = 0x91,
	SWM_SCT3_OUT0_O         = 0x92,
	SWM_SCT3_OUT1_O         = 0x93,
	SWM_SCT3_OUT2_O         = 0xA0,
	SWM_SCT_ABORT0_I        = 0xA1,
	SWM_SCT_ABORT1_I        = 0xA2,
	SWM_ADC0_PIN_TRIG0_I    = 0xA3,
	SWM_ADC0_PIN_TRIG1_I    = 0xB0,
	SWM_ADC1_PIN_TRIG0_I    = 0xB1,
	SWM_ADC1_PIN_TRIG1_I    = 0xB2,
	SWM_DAC_PIN_TRIG_I      = 0xB3,
	SWM_DAC_SHUTOFF_I       = 0xC0,
	SWM_ACMP0_OUT_O         = 0xC1,
	SWM_ACMP1_OUT_O         = 0xC2,
	SWM_ACMP2_OUT_O         = 0xC3,
	SWM_ACMP3_OUT_O         = 0xD0,
	SWM_CLK_OUT_O           = 0xD1,
	SWM_ROSC0_O             = 0xD2,
	SWM_ROSC_RST0_I         = 0xD3,
	SWM_USB_FRAME_TOG_O     = 0xE0,
	SWM_QEI0_PHA_I          = 0xE1,
	SWM_QEI0_PHB_I          = 0xE2,
	SWM_QEI0_IDX_I          = 0xE3,
	SWM_GPIO_INT_BMATCH_O   = 0xF0,
	SWM_SWO_O               = 0xF1,
} CHIP_SWM_PIN_MOVABLE_T;

/**
 * @brief LPC15XX Switch Matrix Fixed pins
 */
typedef enum CHIP_SWM_PIN_FIXED    {
	SWM_FIXED_ADC0_0    = 0x00,	/*!< ADC0_0 fixed pin enable/disable on pin P0_8 */
	SWM_FIXED_ADC0_1    = 0x01,	/*!< ADC0_1 fixed pin enable/disable on pin P0_7 */
	SWM_FIXED_ADC0_2    = 0x02,	/*!< ADC0_2 fixed pin enable/disable on pin P0_6 */
	SWM_FIXED_ADC0_3    = 0x03,	/*!< ADC0_3 fixed pin enable/disable on pin P0_5 */
	SWM_FIXED_ADC0_4    = 0x04,	/*!< ADC0_4 fixed pin enable/disable on pin P0_4 */
	SWM_FIXED_ADC0_5    = 0x05,	/*!< ADC0_5 fixed pin enable/disable on pin P0_3 */
	SWM_FIXED_ADC0_6    = 0x06,	/*!< ADC0_6 fixed pin enable/disable on pin P0_2 */
	SWM_FIXED_ADC0_7    = 0x07,	/*!< ADC0_7 fixed pin enable/disable on pin P0_1 */
	SWM_FIXED_ADC0_8    = 0x08,	/*!< ADC0_8 fixed pin enable/disable on pin P1_0 */
	SWM_FIXED_ADC0_9    = 0x09,	/*!< ADC0_9 fixed pin enable/disable on pin P0_31 */
	SWM_FIXED_ADC0_10   = 0x0A,	/*!< ADC0_10 fixed pin enable/disable on pin P0_0 */
	SWM_FIXED_ADC0_11   = 0x0B,	/*!< ADC0_11 fixed pin enable/disable on pin P0_30 */
	SWM_FIXED_ADC1_0    = 0x0C,	/*!< ADC1_0 fixed pin enable/disable/disable on pin P1_1 */
	SWM_FIXED_ADC1_1    = 0x0D,	/*!< ADC1_1 fixed pin enable/disable on pin P0_9 */
	SWM_FIXED_ADC1_2    = 0x0E,	/*!< ADC1_2 fixed pin enable/disable on pin P0_10 */
	SWM_FIXED_ADC1_3    = 0x0F,	/*!< ADC1_3 fixed pin enable/disable on pin P0_11 */
	SWM_FIXED_ADC1_4    = 0x10,	/*!< ADC1_4 fixed pin enable/disable on pin P1_2 */
	SWM_FIXED_ADC1_5    = 0x11,	/*!< ADC1_5 fixed pin enable/disable on pin P1_3 */
	SWM_FIXED_ADC1_6    = 0x12,	/*!< ADC1_6 fixed pin enable/disable on pin P0_13 */
	SWM_FIXED_ADC1_7    = 0x13,	/*!< ADC1_7 fixed pin enable/disable on pin P0_14 */
	SWM_FIXED_ADC1_8    = 0x14,	/*!< ADC1_8 fixed pin enable/disable on pin P0_15 */
	SWM_FIXED_ADC1_9    = 0x15,	/*!< ADC1_9 fixed pin enable/disable on pin P0_16 */
	SWM_FIXED_ADC1_10   = 0x16,	/*!< ADC1_10 fixed pin enable/disable on pin P1_4 */
	SWM_FIXED_ADC1_11   = 0x17,	/*!< ADC1_11 fixed pin enable/disable on pin P1_5 */
	SWM_FIXED_DAC_OUT   = 0x18,	/*!< DAC_OUT fixed pin enable/disable on pin P0_12 */
	SWM_FIXED_ACMP_I1   = 0x19,	/*!< ACMP input 1 (common input) fixed pin enable/disable on pin P0_27 */
	SWM_FIXED_ACMP_I2   = 0x1A,	/*!< ACMP input 1 (common input) fixed pin enable/disable on pin P1_6 */
	SWM_FIXED_ACMP0_I3  = 0x1B,	/*!< ACMP comparator 0 input 3 fixed pin enable/disable on pin P0_26 */
	SWM_FIXED_ACMP0_I4  = 0x1C,	/*!< ACMP comparator 0 input 4 fixed pin enable/disable on pin P0_25 */
	SWM_FIXED_ACMP1_I3  = 0x1D,	/*!< ACMP comparator 1 input 3 fixed pin enable/disable on pin P0_28 */
	SWM_FIXED_ACMP1_I4  = 0x1E,	/*!< ACMP comparator 1 input 4 fixed pin enable/disable on pin P1_10 */
	SWM_FIXED_ACMP2_I3  = 0x1F,	/*!< ACMP comparator 2 input 3 fixed pin enable/disable on pin P0_29 */
	SWM_FIXED_ACMP2_I4  = 0x80,	/*!< ACMP comparator 2 input 4 fixed pin enable/disable on pin P1_9 */
	SWM_FIXED_ACMP3_I3  = 0x81,	/*!< ACMP comparator 3 input 3 fixed pin enable/disable on pin P1_8 */
	SWM_FIXED_ACMP3_I4  = 0x82,	/*!< ACMP comparator 3 input 4 fixed pin enable/disable on pin P1_7 */
	SWM_FIXED_I2C0_SDA  = 0x83,	/*!< I2C0_SDA fixed pin enable/disable on pin P0_23 */
	SWM_FIXED_I2C0_SCL  = 0x84,	/*!< I2C0_SCL fixed pin enable/disable on pin P0_22 */
	SWM_FIXED_SCT0_OUT3 = 0x85,	/*!< SCT0_OUT3 fixed pin enable/disable on pin P0_0 */
	SWM_FIXED_SCT0_OUT4 = 0x86,	/*!< SCT0_OUT4 fixed pin enable/disable on pin P0_1 */
	SWM_FIXED_SCT0_OUT5 = 0x87,	/*!< SCT0_OUT5 fixed pin enable/disable on pin P0_18 */
	SWM_FIXED_SCT0_OUT6 = 0x88,	/*!< SCT0_OUT6 fixed pin enable/disable on pin P0_24 */
	SWM_FIXED_SCT0_OUT7 = 0x89,	/*!< SCT0_OUT7 fixed pin enable/disable on pin P1_14 */
	SWM_FIXED_SCT1_OUT3 = 0x8A,	/*!< SCT1_OUT3 fixed pin enable/disable on pin P0_2 */
	SWM_FIXED_SCT1_OUT4 = 0x8B,	/*!< SCT1_OUT4 fixed pin enable/disable on pin P0_3 */
	SWM_FIXED_SCT1_OUT5 = 0x8C,	/*!< SCT1_OUT5 fixed pin enable/disable on pin P0_14 */
	SWM_FIXED_SCT1_OUT6 = 0x8D,	/*!< SCT1_OUT6 fixed pin enable/disable on pin P0_20 */
	SWM_FIXED_SCT1_OUT7 = 0x8E,	/*!< SCT1_OUT7 fixed pin enable/disable on pin P1_17 */
	SWM_FIXED_SCT2_OUT3 = 0x8F,	/*!< SCT2_OUT3 fixed pin enable/disable on pin P0_6 */
	SWM_FIXED_SCT2_OUT4 = 0x90,	/*!< SCT2_OUT4 fixed pin enable/disable on pin P0_29 */
	SWM_FIXED_SCT2_OUT5 = 0x91,	/*!< SCT2_OUT5 fixed pin enable/disable on pin P1_20 */
	SWM_FIXED_SCT3_OUT3 = 0x92,	/*!< SCT3_OUT3 fixed pin enable/disable on pin P0_26 */
	SWM_FIXED_SCT3_OUT4 = 0x93,	/*!< SCT3_OUT4 fixed pin enable/disable on pin P1_8 */
	SWM_FIXED_SCT3_OUT5 = 0x94,	/*!< SCT3_OUT5 fixed pin enable/disable on pin P1_24 */
	SWM_FIXED_RESETN    = 0x95,	/*!< RESETN fixed pin enable/disable on pin P0_21 */
	SWM_FIXED_SWCLK_TCK = 0x96,	/*!< SWCLK_TCK fixed pin enable/disable on pin P0_19 */
	SWM_FIXED_SWDIO     = 0x97,	/*!< SWDIO fixed pin enable/disable on pin P0_20 */
} CHIP_SWM_PIN_FIXED_T;

/**
 * @brief	Initialize the SWM module
 * @return	Nothing
 * @note	This function only enables the SWM clock.
 */
STATIC INLINE void Chip_SWM_Init(void)
{
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
}

/**
 * @brief	Deinitialise the SWM module
 * @return	Nothing
 * @note	This function only disables the SWM clock.
 */
STATIC INLINE void Chip_SWM_Deinit(void)
{
	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);
}

/**
 * @brief	Assign movable pin function to physical pin in Switch Matrix
 * @param	movable	: Movable pin function
 * @param	assign	: Physical pin to be assigned
 * @return	Nothing
 */
void Chip_SWM_MovablePinAssign(CHIP_SWM_PIN_MOVABLE_T movable, uint8_t assign);

/**
 * @brief	Assign movable pin function to port and pin in the Switch Matrix
 * @param	movable	: Movable pin function
 * @param	port	: Port number
 * @param	pin		: Pin number
 * @return	Nothing
 * @note	This function does the same thing as Chip_SWM_MovablePinAssign()
 *			except works with a port and pin number instead of a physical
 *			pin number.
 */
STATIC INLINE void Chip_SWM_MovablePortPinAssign(CHIP_SWM_PIN_MOVABLE_T movable, uint8_t port, uint8_t pin)
{
	Chip_SWM_MovablePinAssign(movable, ((port * 32) + pin));
}

/**
 * @brief	Enables a fixed function pin in the Switch Matrix
 * @param	pin	: Pin to be enabled
 * @return	Nothing
 */
void Chip_SWM_EnableFixedPin(CHIP_SWM_PIN_FIXED_T pin);

/**
 * @brief	Disables a fixed function pin in the Switch Matrix
 * @param	pin	: Pin to be disabled
 * @return	Nothing
 */
void Chip_SWM_DisableFixedPin(CHIP_SWM_PIN_FIXED_T pin);

/**
 * @brief	Enables or disables a fixed function pin in the Switch Matrix
 * @param	pin		: Pin to be enabled or disabled
 * @param	enable	: True to enable the pin, False to disable the pin
 * @return	Nothing
 */
void Chip_SWM_FixedPinEnable(CHIP_SWM_PIN_FIXED_T pin, bool enable);

/**
 * @brief	Tests whether a fixed function pin is enabled or disabled in the Switch Matrix
 * @param	pin	: The pin to test whether it is enabled or disabled
 * @return	True if the pin is enabled, False if disabled
 */
bool Chip_SWM_IsFixedPinEnabled(CHIP_SWM_PIN_FIXED_T pin);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __SWM_15XX_H_ */
