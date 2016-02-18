/*
 * @brief LPC15XX Clock control functions
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
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

#ifndef __CLOCK_15XX_H_
#define __CLOCK_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup CLOCK_15XX CHIP: LPC15xx Clock Control block driver
 * @ingroup CHIP_15XX_Drivers
 * @{
 */

/** Internal oscillator frequency */
#define SYSCTL_IRC_FREQ (12000000)

/** Internal watchdog oscillator frequency */
#define SYSCTL_WDTOSC_FREQ (503000)

/** @defgroup CLOCK_15XXcCHIP_SEL: Clock source selection functions
 * These functions provide selection of clocks for system functions such as
 * the USB clock, main system clock, or the clock output pin.
 * @{
 */

/**
 * Clock source selections for only the main A system clock. The main A system
 * clock is used as an input into the main B system clock selector. Main clock A
 * only needs to be setup if the main clock A input is used in the main clock
 * system selector.
 */
typedef enum CHIP_SYSCTL_MAIN_A_CLKSRC {
	SYSCTL_MAIN_A_CLKSRC_IRC = 0,		/*!< Internal oscillator */
	SYSCTL_MAIN_A_CLKSRCA_MAINOSC,		/*!< Crystal (main) oscillator in */
	SYSCTL_MAIN_A_CLKSRCA_SYSOSC = SYSCTL_MAIN_A_CLKSRCA_MAINOSC,
	SYSCTL_MAIN_A_CLKSRCA_WDTOSC,		/*!< Watchdog oscillator rate */
	SYSCTL_MAIN_A_CLKSRCA_RESERVED,
} CHIP_SYSCTL_MAIN_A_CLKSRC_T;

/**
 * @brief	Set main A system clock source
 * @param	src	: Clock source for main A
 * @return	Nothing
 * @note	This function only neesd to be setup if main clock A will be
 * selected in the Chip_Clock_GetMain_B_ClockRate() function.
 */
STATIC INLINE void Chip_Clock_SetMain_A_ClockSource(CHIP_SYSCTL_MAIN_A_CLKSRC_T src)
{
	LPC_SYSCTL->MAINCLKSELA = (uint32_t) src;
}

/**
 * @brief   Returns the main A clock source
 * @return	Returns which clock is used for the main A
 */
STATIC INLINE CHIP_SYSCTL_MAIN_A_CLKSRC_T Chip_Clock_GetMain_A_ClockSource(void)
{
	return (CHIP_SYSCTL_MAIN_A_CLKSRC_T) (LPC_SYSCTL->MAINCLKSELA);
}

/**
 * @brief	Return main A clock rate
 * @return	main A clock rate in Hz
 */
uint32_t Chip_Clock_GetMain_A_ClockRate(void);

/**
 * Clock sources for only main B system clock
 */
typedef enum CHIP_SYSCTL_MAIN_B_CLKSRC {
	SYSCTL_MAIN_B_CLKSRC_MAINCLKSELA = 0,	/*!< main clock A */
	SYSCTL_MAIN_B_CLKSRC_SYSPLLIN,			/*!< System PLL input */
	SYSCTL_MAIN_B_CLKSRC_SYSPLLOUT,			/*!< System PLL output */
	SYSCTL_MAIN_B_CLKSRC_RTC,				/*!< RTC oscillator 32KHz output */
} CHIP_SYSCTL_MAIN_B_CLKSRC_T;

/**
 * @brief	Set main B system clock source
 * @param	src	: Clock source for main B
 * @return	Nothing
 */
STATIC INLINE void Chip_Clock_SetMain_B_ClockSource(CHIP_SYSCTL_MAIN_B_CLKSRC_T src)
{
	LPC_SYSCTL->MAINCLKSELB = (uint32_t) src;
}

/**
 * @brief   Returns the main B clock source
 * @return	Returns which clock is used for the main B
 */
STATIC INLINE CHIP_SYSCTL_MAIN_B_CLKSRC_T Chip_Clock_GetMain_B_ClockSource(void)
{
	return (CHIP_SYSCTL_MAIN_B_CLKSRC_T) (LPC_SYSCTL->MAINCLKSELB);
}

/**
 * @brief	Return main B clock rate
 * @return	main B clock rate
 */
uint32_t Chip_Clock_GetMain_B_ClockRate(void);

/**
 * Clock sources for main system clock. This is a mix of both main clock A
 * and B seelctions.
 */
typedef enum CHIP_SYSCTL_MAINCLKSRC {
	SYSCTL_MAINCLKSRC_IRC = 0,			/*!< Internal oscillator */
	SYSCTL_MAINCLKSRCA_MAINOSC,			/*!< Crystal (main) oscillator in */
	SYSCTL_MAINCLKSRCA_SYSOSC = SYSCTL_MAINCLKSRCA_MAINOSC,
	SYSCTL_MAINCLKSRCA_WDTOSC,			/*!< Watchdog oscillator rate */
	SYSCTL_MAINCLKSRC_SYSPLLIN = 5,		/*!< System PLL input */
	SYSCTL_MAINCLKSRC_SYSPLLOUT,		/*!< System PLL output */
	SYSCTL_MAINCLKSRC_RTC,				/*!< RTC oscillator 32KHz output */
} CHIP_SYSCTL_MAINCLKSRC_T;

/**
 * @brief	Set main system clock source
 * @param	src	: Main clock source
 * @return	Nothing
 * @note	This functions handles setup of both A and B main clock sources.
 */
void Chip_Clock_SetMainClockSource(CHIP_SYSCTL_MAINCLKSRC_T src);

/**
 * @brief   Returns the main clock source
 * @return	Returns which clock is used for the main clock source
 * @note	This functions handles both A and B main clock sources.
 */
CHIP_SYSCTL_MAINCLKSRC_T Chip_Clock_GetMainClockSource(void);

/**
 * @brief	Return main clock rate
 * @return	main clock rate
 */
uint32_t Chip_Clock_GetMainClockRate(void);

/**
 * @brief	Return system clock rate
 * @return	system clock rate
 */
uint32_t Chip_Clock_GetSystemClockRate(void);

/**
 * Clock sources for USB (usb_clk)
 */
typedef enum CHIP_SYSCTL_USBCLKSRC {
	SYSCTL_USBCLKSRC_IRC = 0,		/*!< Internal oscillator */
	SYSCTL_USBCLKSRC_MAINOSC,		/*!< Crystal (main) oscillator in */
	SYSCTL_USBCLKSRC_SYSOSC = SYSCTL_USBCLKSRC_MAINOSC,
	SYSCTL_USBCLKSRC_PLLOUT,		/*!< USB PLL out */
	SYSCTL_USBCLKSRC_MAINSYSCLK,	/*!< Main system clock (B) */
} CHIP_SYSCTL_USBCLKSRC_T;

/**
 * @brief	Set USB clock source and divider
 * @param	src	: Clock source for USB
 * @param	div	: divider for USB clock
 * @return	Nothing
 * @note	Use 0 to disable, or a divider value of 1 to 255. The USB clock
 * rate is either the main system clock or USB PLL output clock divided
 * by this value. This function will also toggle the clock source
 * update register to update the clock source.
 */
STATIC INLINE void Chip_Clock_SetUSBClockSource(CHIP_SYSCTL_USBCLKSRC_T src, uint32_t div)
{
	LPC_SYSCTL->USBCLKSEL = (uint32_t) src;
	LPC_SYSCTL->USBCLKDIV = div;
}

/**
 * Clock sources for ADC asynchronous clock source select
 */
typedef enum CHIP_SYSCTL_ADCASYNCCLKSRC {
	SYSCTL_ADCASYNCCLKSRC_IRC = 0,		/*!< Internal oscillator */
	SYSCTL_ADCASYNCCLKSRC_SYSPLLOUT,	/*!< System PLL out */
	SYSCTL_ADCASYNCCLKSRC_USBPLLOUT,	/*!< USB PLL out */
	SYSCTL_ADCASYNCCLKSRC_SCTPLLOUT		/*!< SCT PLL out */
} CHIP_SYSCTL_ADCASYNCCLKSRC_T;

/**
 * @brief	Set the ADC asynchronous clock source
 * @param	src	: ADC asynchronous clock source
 * @return	Nothing
 */
STATIC INLINE void Chip_Clock_SetADCASYNCSource(CHIP_SYSCTL_ADCASYNCCLKSRC_T src)
{
	LPC_SYSCTL->ADCASYNCCLKSEL = (uint32_t) src;
}

/**
 * @brief   Returns the ADC asynchronous clock source
 * @return	Returns which clock is used for the ADC asynchronous clock source
 */
STATIC INLINE CHIP_SYSCTL_ADCASYNCCLKSRC_T Chip_Clock_GetADCASYNCSource(void)
{
	return (CHIP_SYSCTL_ADCASYNCCLKSRC_T) (LPC_SYSCTL->ADCASYNCCLKSEL);
}

/**
 * @brief	Return ADC asynchronous clock rate
 * @return	ADC asynchronous clock rate (not including divider)
 */
uint32_t Chip_Clock_GetADCASYNCRate(void);

/**
 * Clock sources for CLKOUT
 */
typedef enum CHIP_SYSCTL_CLKOUTSRC {
	SYSCTL_CLKOUTSRC_IRC = 0,		/*!< Internal oscillator for CLKOUT */
	SYSCTL_CLKOUTSRC_MAINOSC,		/*!< Main oscillator for CLKOUT */
	SYSCTL_CLKOUTSRC_SYSOSC = SYSCTL_CLKOUTSRC_MAINOSC,
	SYSCTL_CLKOUTSRC_WDTOSC,		/*!< Watchdog oscillator for CLKOUT */
	SYSCTL_CLKOUTSRC_MAINSYSCLK,	/*!< Main (B) system clock for CLKOUT */
	SYSCTL_CLKOUTSRC_USBPLLOUT = 5,	/*!< USB PLL out */
	SYSCTL_CLKOUTSRC_SCTPLLOUT,		/*!< SCT PLL out */
	SYSCTL_CLKOUTSRC_RTC32K			/*!< RTC 32 kHz output */
} CHIP_SYSCTL_CLKOUTSRC_T;

/**
 * @brief	Set CLKOUT clock source and divider
 * @param	src	: Clock source for CLKOUT
 * @param	div	: divider for CLKOUT clock
 * @return	Nothing
 * @note	Use 0 to disable, or a divider value of 1 to 255. The CLKOUT clock
 * rate is the clock source divided by the divider. This function will
 * also toggle the clock source update register to update the clock
 * source.
 */
void Chip_Clock_SetCLKOUTSource(CHIP_SYSCTL_CLKOUTSRC_T src, uint32_t div);

/**
 * @}
 */

/** @defgroup CLOCK_15XX_CHIP_PLL: PLL setup functions
 * @{
 */

/**
 * Clock sources for system, USB, and SCT PLLs
 */
typedef enum CHIP_SYSCTL_PLLCLKSRC {
	SYSCTL_PLLCLKSRC_IRC = 0,		/*!< Internal oscillator in (may not work for USB) */
	SYSCTL_PLLCLKSRC_MAINOSC,		/*!< Crystal (main) oscillator in */
	SYSCTL_PLLCLKSRC_SYSOSC = SYSCTL_PLLCLKSRC_MAINOSC,
	SYSCTL_PLLCLKSRC_RESERVED1,		/*!< Reserved */
	SYSCTL_PLLCLKSRC_RESERVED2,		/*!< Reserved */
} CHIP_SYSCTL_PLLCLKSRC_T;

/**
 * @brief	Set System PLL clock source
 * @param	src	: Clock source for system PLL
 * @return	Nothing
 */
STATIC INLINE void Chip_Clock_SetSystemPLLSource(CHIP_SYSCTL_PLLCLKSRC_T src)
{
	LPC_SYSCTL->SYSPLLCLKSEL  = (uint32_t) src;
}

/**
 * @brief	Set System PLL divider values
 * @param	msel    : PLL feedback divider value. M = msel + 1.
 * @param	psel    : PLL post divider value. P =  (1<<psel).
 * @return	Nothing
 * @note	See the user manual for how to setup the PLL.
 */
STATIC INLINE void Chip_Clock_SetupSystemPLL(uint8_t msel, uint8_t psel)
{
	LPC_SYSCTL->SYSPLLCTRL = (msel & 0x3F) | ((psel & 0x3) << 6);
}

/**
 * @brief	Read System PLL lock status
 * @return	true of the PLL is locked. false if not locked
 */
STATIC INLINE bool Chip_Clock_IsSystemPLLLocked(void)
{
	return (bool) ((LPC_SYSCTL->SYSPLLSTAT & 1) != 0);
}

/**
 * @brief	Return System PLL input clock rate
 * @return	System PLL input clock rate
 */
uint32_t Chip_Clock_GetSystemPLLInClockRate(void);

/**
 * @brief	Return System PLL output clock rate
 * @return	System PLL output clock rate
 */
uint32_t Chip_Clock_GetSystemPLLOutClockRate(void);

/**
 * @brief	Set USB PLL clock source
 * @param	src	: Clock source for USB PLL
 * @return	Nothing
 */
STATIC INLINE void Chip_Clock_SetUSBPLLSource(CHIP_SYSCTL_PLLCLKSRC_T src)
{
	LPC_SYSCTL->USBPLLCLKSEL  = (uint32_t) src;
}

/**
 * @brief	Set USB PLL divider values
 * @param	msel    : PLL feedback divider value. M = msel + 1.
 * @param	psel    : PLL post divider value. P = (1<<psel).
 * @return	Nothing
 * @note	See the user manual for how to setup the PLL.
 */
STATIC INLINE void Chip_Clock_SetupUSBPLL(uint8_t msel, uint8_t psel)
{
	LPC_SYSCTL->USBPLLCTRL = (msel & 0x3F) | ((psel & 0x3) << 6);
}

/**
 * @brief	Read USB PLL lock status
 * @return	true of the PLL is locked. false if not locked
 */
STATIC INLINE bool Chip_Clock_IsUSBPLLLocked(void)
{
	return (bool) ((LPC_SYSCTL->USBPLLSTAT & 1) != 0);
}

/**
 * @brief	Return USB PLL input clock rate
 * @return	USB PLL input clock rate
 */
uint32_t Chip_Clock_GetUSBPLLInClockRate(void);

/**
 * @brief	Return USB PLL output clock rate
 * @return	USB PLL output clock rate
 */
uint32_t Chip_Clock_GetUSBPLLOutClockRate(void);

/**
 * @brief	Set SCT PLL clock source
 * @param	src	: Clock source for SCT PLL
 * @return	Nothing
 */
STATIC INLINE void Chip_Clock_SetSCTPLLSource(CHIP_SYSCTL_PLLCLKSRC_T src)
{
	LPC_SYSCTL->SCTPLLCLKSEL  = (uint32_t) src;
}

/**
 * @brief	Set SCT PLL divider values
 * @param	msel    : PLL feedback divider value. M = msel + 1.
 * @param	psel    : PLL post divider value. P = (1<<psel).
 * @return	Nothing
 * @note	See the user manual for how to setup the PLL.
 */
STATIC INLINE void Chip_Clock_SetupSCTPLL(uint8_t msel, uint8_t psel)
{
	LPC_SYSCTL->SCTPLLCTRL = (msel & 0x3F) | ((psel & 0x3) << 6);
}

/**
 * @brief	Read SCT PLL lock status
 * @return	true of the PLL is locked. false if not locked
 */
STATIC INLINE bool Chip_Clock_IsSCTPLLLocked(void)
{
	return (bool) ((LPC_SYSCTL->SCTPLLSTAT & 1) != 0);
}

/**
 * @brief	Return SCT PLL input clock rate
 * @return	SCT PLL input clock rate
 */
uint32_t Chip_Clock_GetSCTPLLInClockRate(void);

/**
 * @brief	Return SCT PLL output clock rate
 * @return	SCT PLL output clock rate
 */
uint32_t Chip_Clock_GetSCTPLLOutClockRate(void);

/**
 * @}
 */

/** @defgroup CLOCK_15XX_CHIP_SUP: Clock support functions
 * Functions in this group include oscillator control and rates, peripheral
 * clock control, and peripheral dividers.
 * @{
 */

/**
 * @brief	Set system clock divider
 * @param	div	: divider for system clock
 * @return	Nothing
 * @note	Use 0 to disable, or a divider value of 1 to 255. The system clock
 * rate is the main system clock divided by this value.
 */
STATIC INLINE void Chip_Clock_SetSysClockDiv(uint32_t div)
{
	LPC_SYSCTL->SYSAHBCLKDIV  = div;
}

/**
 * System and peripheral clocks
 */
typedef enum CHIP_SYSCTL_CLOCK {
	/* Peripheral clock enables for SYSAHBCLKCTRL0 */
	SYSCTL_CLOCK_SYS = 0,				/*!< System clock */
	SYSCTL_CLOCK_ROM,					/*!< ROM clock */
	SYSCTL_CLOCK_SRAM1 = 3,				/*!< SRAM1 clock */
	SYSCTL_CLOCK_SRAM2,					/*!< SRAM2 clock */
	SYSCTL_CLOCK_FLASH = 7,				/*!< FLASH controller clock */
	SYSCTL_CLOCK_EEPROM = 9,			/*!< EEPROM controller clock */
	SYSCTL_CLOCK_MUX = 11,				/*!< Input mux clock */
	SYSCTL_CLOCK_SWM,					/*!< Switch matrix clock */
	SYSCTL_CLOCK_IOCON,					/*!< IOCON clock */
	SYSCTL_CLOCK_GPIO0,					/*!< GPIO0 clock */
	SYSCTL_CLOCK_GPIO1,					/*!< GPIO1 clock */
	SYSCTL_CLOCK_GPIO2,					/*!< GPIO2 clock */
	SYSCTL_CLOCK_PININT = 18,			/*!< PININT clock */
	SYSCTL_CLOCK_GINT,					/*!< grouped pin interrupt block clock */
	SYSCTL_CLOCK_DMA,					/*!< DMA clock */
	SYSCTL_CLOCK_CRC,					/*!< CRC clock */
	SYSCTL_CLOCK_WDT,					/*!< WDT clock */
	SYSCTL_CLOCK_RTC,					/*!< RTC clock */
	SYSCTL_CLOCK_ADC0 = 27,				/*!< ADC0 clock */
	SYSCTL_CLOCK_ADC1,					/*!< ADC1 clock */
	SYSCTL_CLOCK_DAC,					/*!< DAC clock */
	SYSCTL_CLOCK_ACMP,					/*!< ACMP clock */
	/* Peripheral clock enables for SYSAHBCLKCTRL1 */
	SYSCTL_CLOCK_MRT = 32,				/*!< multi-rate timer clock */
	SYSCTL_CLOCK_RIT,					/*!< repetitive interrupt timer clock */
	SYSCTL_CLOCK_SCT0,					/*!< SCT0 clock */
	SYSCTL_CLOCK_SCT1,					/*!< SCT1 clock */
	SYSCTL_CLOCK_SCT2,					/*!< SCT2 clock */
	SYSCTL_CLOCK_SCT3,					/*!< SCT3 clock */
	SYSCTL_CLOCK_SCTIPU,				/*!< SCTIPU clock */
	SYSCTL_CLOCK_CAN,					/*!< CAN clock */
	SYSCTL_CLOCK_SPI0 = 32 + 9,			/*!< SPI0 clock */
	SYSCTL_CLOCK_SPI1,					/*!< SPI1 clock */
	SYSCTL_CLOCK_I2C0 = 32 + 13,		/*!< I2C0 clock */
	SYSCTL_CLOCK_UART0 = 32 + 17,		/*!< UART0 clock */
	SYSCTL_CLOCK_UART1,					/*!< UART1 clock */
	SYSCTL_CLOCK_UART2,					/*!< UART2 clock */
	SYSCTL_CLOCK_QEI = 32 + 21,			/*!< QEI clock */
	SYSCTL_CLOCK_USB = 32 + 23,			/*!< USB clock */
} CHIP_SYSCTL_CLOCK_T;

/**
 * @brief	Enable a system or peripheral clock
 * @param	clk	: Clock to enable
 * @return	Nothing
 */
void Chip_Clock_EnablePeriphClock(CHIP_SYSCTL_CLOCK_T clk);

/**
 * @brief	Disable a system or peripheral clock
 * @param	clk	: Clock to disable
 * @return	Nothing
 */
void Chip_Clock_DisablePeriphClock(CHIP_SYSCTL_CLOCK_T clk);

/**
 * @brief	Set system tick clock divider
 * @param	div	: divider for system clock
 * @return	Nothing
 * @note	Use 0 to disable, or a divider value of 1 to 255. The system tick
 * rate is the main system clock divided by this value. Use caution when using
 * the CMSIS SysTick_Config() functions as they typically use SystemCoreClock
 * for setup.
 */
STATIC INLINE void Chip_Clock_SetSysTickClockDiv(uint32_t div)
{
	LPC_SYSCTL->SYSTICKCLKDIV = div;
}

/**
 * @brief	Returns system tick clock divider
 * @return	system tick clock divider
 */
STATIC INLINE uint32_t Chip_Clock_GetSysTickClockDiv(void)
{
	return LPC_SYSCTL->SYSTICKCLKDIV;
}

/**
 * @brief	Returns the system tick rate as used with the system tick divider
 * @return	the system tick rate
 */
uint32_t Chip_Clock_GetSysTickClockRate(void);

/**
 * @brief	Set IOCON glitch filter clock divider value
 * @param	div		: value for IOCON filter divider
 * @return	Nothing
 * @note	Use 0 to disable, or a divider value of 1 to 255.
 */
STATIC INLINE void Chip_Clock_SetIOCONFiltClockDiv(uint32_t div)
{
	LPC_SYSCTL->IOCONCLKDIV  = div;
}

/**
 * @brief	Return IOCON glitch filter clock divider value
 * @return	IOCON glitch filter clock divider value
 */
STATIC INLINE uint32_t Chip_Clock_GetIOCONFiltClockDiv(void)
{
	return LPC_SYSCTL->IOCONCLKDIV;
}

/**
 * @brief	Set Asynchronous ADC clock divider value
 * @param	div	: value for UART fractional generator multiplier value
 * @return	Nothing
 * @note	Use 0 to disable, or a divider value of 1 to 255.
 */
STATIC INLINE void Chip_Clock_SetADCASYNCClockDiv(uint32_t div)
{
	LPC_SYSCTL->ADCASYNCCLKDIV  = div;
}

/**
 * @brief	Return Asynchronous ADC clock divider value
 * @return	Asynchronous ADC clock divider value
 */
STATIC INLINE uint32_t Chip_Clock_GetADCASYNCClockDiv(void)
{
	return LPC_SYSCTL->ADCASYNCCLKDIV;
}

/**
 * @brief	Set UART base rate base rate (up to main clock rate) (all UARTs)
 * @param	rate	: Desired rate for fractional divider/multipler output
 * @param	fEnable	: true to use fractional clocking, false for integer clocking
 * @return	Actual rate generated
 * @note	All UARTs use the same base clock for their baud rate
 *			basis. This function is used to generate that clock, while the
 *			UART driver's SetBaud functions will attempt to get the closest
 *			baud rate from this base clock without altering it. This needs
 *			to be setup prior to individual UART setup.<br>
 *			UARTs need a base clock 16x faster than the baud rate, so if you
 *			need a 115.2Kbps baud rate, you will need a clock rate of at
 *			least (115.2K * 16). The UART base clock is generated from the
 *			main system clock, so fractional clocking may be the only
 *			possible choice when using a low main system clock frequency.
 *			Do not alter the FRGCTRL or UARTCLKDIV registers after this call.
 */
uint32_t Chip_Clock_SetUARTBaseClockRate(uint32_t rate, bool fEnable);

/**
 * @brief	Get UART base rate (all UARTs)
 * @return	UART base rate in Hz
 */
uint32_t Chip_Clock_GetUARTBaseClockRate(void);

/**
 * @brief	Set The UART Fractional Generator Divider (all UARTs)
 * @param   div  :  Fractional Generator Divider value, should be 0xFF
 * @return	Nothing
 */
STATIC INLINE void Chip_Clock_SetUARTFRGDivider(uint8_t div)
{
	LPC_SYSCTL->UARTCLKDIV = (uint32_t) div;
}

/**
 * @brief	Get The UART Fractional Generator Divider (all UARTs)
 * @return	Value of UART Fractional Generator Divider
 */
STATIC INLINE uint32_t Chip_Clock_GetUARTFRGDivider(void)
{
	return LPC_SYSCTL->UARTCLKDIV;
}

/**
 * @brief	Enable the RTC 32KHz output
 * @return	Nothing
 * @note	This clock can be used for the main clock directly, but
 *			do not use this clock with the system PLL.
 */
STATIC INLINE void Chip_Clock_EnableRTCOsc(void)
{
	LPC_SYSCTL->RTCOSCCTRL  = 1;
}

/**
 * @brief	Disable the RTC 32KHz output
 * @return	Nothing
 */
STATIC INLINE void Chip_Clock_DisableRTCOsc(void)
{
	LPC_SYSCTL->RTCOSCCTRL  = 0;
}

/**
 * @brief	Returns the main oscillator clock rate
 * @return	main oscillator clock rate in Hz
 */
STATIC INLINE uint32_t Chip_Clock_GetMainOscRate(void)
{
	return OscRateIn;
}

/**
 * @brief	Returns the internal oscillator (IRC) clock rate
 * @return	internal oscillator (IRC) clock rate in Hz
 */
STATIC INLINE uint32_t Chip_Clock_GetIntOscRate(void)
{
	return SYSCTL_IRC_FREQ;
}

/**
 * @brief	Returns the RTC clock rate
 * @return	RTC oscillator clock rate in Hz
 */
STATIC INLINE uint32_t Chip_Clock_GetRTCOscRate(void)
{
	return RTCOscRateIn;
}

/**
 * @brief	Return estimated watchdog oscillator rate
 * @return	Estimated watchdog oscillator rate
 * @note	This rate is accurate to plus or minus 40%.
 */
STATIC INLINE uint32_t Chip_Clock_GetWDTOSCRate(void)
{
	return SYSCTL_WDTOSC_FREQ;
}

/**
 * @}
 */

/** @defgroup CLOCK_15XX_CHIP_MISC: Misc clock functions
 * @{
 */

/**
 * @brief	Bypass System Oscillator and set oscillator frequency range
 * @param	bypass	: Flag to bypass oscillator
 * @param	highfr	: Flag to set oscillator range from 15-25 MHz
 * @return	Nothing
 * @note	Sets the PLL input to bypass the oscillator. This would be
 * used if an external clock that is not an oscillator is attached
 * to the XTALIN pin.
 */
void Chip_Clock_SetPLLBypass(bool bypass, bool highfr);

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __CLOCK_15XX_H_ */
