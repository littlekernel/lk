/*
 * @brief LPC15XX System Control registers and control functions
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

#ifndef __SYSCTL_15XX_H_
#define __SYSCTL_15XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup SYSCTL_15XX CHIP: LPC15xx System Control block driver
 * @ingroup CHIP_15XX_Drivers
 * @{
 */

/**
 * @brief LPC15XX System Control block structure
 */
typedef struct {					/*!< SYSCTL Structure */
	__IO uint32_t  SYSMEMREMAP;		/*!< System Memory remap register */
	__I  uint32_t  RESERVED0[2];
	__IO uint32_t  AHBBUFEN0;		
	__IO uint32_t  AHBBUFEN1;		
	__IO uint32_t  SYSTCKCAL;		/*!< System tick counter calibration register */
	__I  uint32_t  RESERVED1[1];
	__IO uint32_t  NMISRC;			/*!< NMI source control register */
	__I  uint32_t  RESERVED2[8];
	__IO uint32_t  SYSRSTSTAT;		/*!< System Reset Status register */
	__IO uint32_t  PRESETCTRL[2];	/*!< Peripheral reset Control registers */
	__I  uint32_t  PIOPORCAP[3];	/*!< POR captured PIO status registers */
	__I  uint32_t  RESERVED3[10];
	__IO uint32_t  MAINCLKSELA;		/*!< Main clock source A select register */
	__IO uint32_t  MAINCLKSELB;		/*!< Main clock source B select register */
	__IO uint32_t  USBCLKSEL;		/*!< USB clock source select register */
	__IO uint32_t  ADCASYNCCLKSEL;	/*!< ADC asynchronous clock source select register */
	__I  uint32_t  RESERVED4[1];
	__IO uint32_t  CLKOUTSEL[2];	/*!< Clock out source select registers */
	__I  uint32_t  RESERVED5[1];
	__IO uint32_t  SYSPLLCLKSEL;	/*!< System PLL clock source select register */
	__IO uint32_t  USBPLLCLKSEL;	/*!< USB PLL clock source select register */
	__IO uint32_t  SCTPLLCLKSEL;	/*!< SCT PLL clock source select register */
	__I  uint32_t  RESERVED6[5];
	__IO uint32_t  SYSAHBCLKDIV;	/*!< System Clock divider register */
	__IO uint32_t  SYSAHBCLKCTRL[2];/*!< System clock control registers */
	__IO uint32_t  SYSTICKCLKDIV;	/*!< SYSTICK clock divider */
	__IO uint32_t  UARTCLKDIV;		/*!< UART clock divider register */
	__IO uint32_t  IOCONCLKDIV;		/*!< programmable glitch filter divider registers for IOCON */
	__IO uint32_t  TRACECLKDIV;		/*!< ARM trace clock divider register */
	__I  uint32_t  RESERVED7[4];
	__IO uint32_t  USBCLKDIV;		/*!< USB clock source divider register */
	__IO uint32_t  ADCASYNCCLKDIV;	/*!< Asynchronous ADC clock divider */
	__I  uint32_t  RESERVED8[1];
	__IO uint32_t  CLKOUTDIV;		/*!< Clock out divider register */
	__I  uint32_t  RESERVED9[9];
	__IO uint32_t  FREQMECTRL;		/*!< Frequency measure register */
	__IO uint32_t  FLASHCFG;		/*!< Flash configuration register */
	__IO uint32_t  FRGCTRL;			/*!< USART fractional baud rate generator control register */
	__IO uint32_t  USBCLKCTRL;		/*!< USB clock control register */
	__I  uint32_t  USBCLKST;		/*!< USB clock status register */
	__I  uint32_t  RESERVED10[19];
	__IO uint32_t  BODCTRL;			/*!< Brown Out Detect register */
	__I  uint32_t  IRCCTRL;			
	__IO uint32_t  SYSOSCCTRL;		/*!< System Oscillator control register */
	__I  uint32_t  RESERVED11[1];
	__IO uint32_t  RTCOSCCTRL;		/*!< RTC Oscillator control register */
	__I  uint32_t  RESERVED12[1];
	__IO uint32_t  SYSPLLCTRL;		/*!< System PLL control register */
	__I  uint32_t  SYSPLLSTAT;		/*!< System PLL status register */
	__IO uint32_t  USBPLLCTRL;		/*!< USB PLL control register */
	__I  uint32_t  USBPLLSTAT;		/*!< USB PLL status register */
	__IO uint32_t  SCTPLLCTRL;		/*!< SCT PLL control register */
	__I  uint32_t  SCTPLLSTAT;		/*!< SCT PLL status register */
	__I  uint32_t  RESERVED13[21];
	__IO uint32_t  PDWAKECFG;		/*!< Power down states in wake up from deep sleep register */
	__IO uint32_t  PDRUNCFG;		/*!< Power configuration register*/
	__I  uint32_t  RESERVED14[3];
	__IO uint32_t  STARTERP[2];		/*!< Start logic interrupt wake-up enable registers */
	__I  uint32_t  RESERVED15[117];
	__I  uint32_t  JTAG_IDCODE;		/*!< JTAG ID code register */
	__I  uint32_t  DEVICEID[2];		/*!< Device ID registers */
} LPC_SYSCTL_T;

/**
 * System memory remap modes used to remap interrupt vectors
 */
typedef enum CHIP_SYSCTL_BOOT_MODE_REMAP {
	REMAP_BOOT_LOADER_MODE = 0,	/*!< Interrupt vectors are re-mapped to Boot ROM */
	REMAP_USER_RAM_MODE,		/*!< Interrupt vectors are re-mapped to Static RAM */
	REMAP_USER_FLASH_MODE		/*!< Interrupt vectors are not re-mapped and reside in Flash */
} CHIP_SYSCTL_BOOT_MODE_REMAP_T;

/**
 * @brief	Re-map interrupt vectors
 * @param	remap	: system memory map value
 * @return	Nothing
 */
STATIC INLINE void Chip_SYSCTL_Map(CHIP_SYSCTL_BOOT_MODE_REMAP_T remap)
{
	LPC_SYSCTL->SYSMEMREMAP = (uint32_t) remap;
}

/**
 * Peripheral reset identifiers, not available on all devices
 */
typedef enum {
	/* PRESETCTRL0 resets */
	RESET_FLASH = 7,		/*!< FLASH controller reset control */
	RESET_EEPROM = 9,		/*!< EEPROM controller reset control */
	RESET_MUX = 11,			/*!< Input mux reset control */
	RESET_IOCON = 13,		/*!< IOCON reset control */
	RESET_PININT = 18,		/*!< Pin interrupt (PINT) reset reset control */
	RESET_GINT,				/*!< Grouped interrupt (GINT) reset control */
	RESET_DMA,				/*!< DMA reset control */
	RESET_CRC,				/*!< CRC reset control */
	RESET_ADC0 = 27,		/*!< ADC0 reset control */
	RESET_ADC1,				/*!< ADC1 reset control */
	RESET_ACMP = 30,		/*!< Analog Comparator (all 4 ACMP) reset control */
	RESET_MRT = 32 + 0,		/*!< Multi-rate timer (MRT) reset control */
	RESET_RIT,				/*!< Repetitive interrupt timer (RIT) reset control */
	RESET_SCT0,				/*!< State configurable timer 0 (SCT0) reset control */
	RESET_SCT1,				/*!< State configurable timer 1 (SCT1) reset control */
	RESET_SCT2,				/*!< State configurable timer 2 (SCT2) reset control */
	RESET_SCT3,				/*!< State configurable timer 3 (SCT3) reset control */
	RESET_SCTIPU,			/*!< State configurable timer IPU (SCTIPU) reset control */
	RESET_CAN,				/*!< CAN reset control */
	RESET_SPI0 = 32 + 9,	/*!< SPI0 reset control */
	RESET_SPI1,				/*!< SPI1 reset control */
	RESET_I2C0 = 32 + 13,	/*!< I2C0 reset control */
	RESET_UART0 = 32 + 17,	/*!< UART0 reset control */
	RESET_UART1,			/*!< UART1 reset control */
	RESET_UART2,			/*!< UART2 reset control */
	RESET_QEI0 = 32 + 21,	/*!< QEI0 reset control */
	RESET_USB = 32 + 23		/*!< USB reset control */
} CHIP_SYSCTL_PERIPH_RESET_T;

/**
 * @brief	Assert reset for a peripheral
 * @param	periph	: Peripheral to assert reset for
 * @return	Nothing
 * @note	The peripheral will stay in reset until reset is de-asserted. Call
 * Chip_SYSCTL_DeassertPeriphReset() to de-assert the reset.
 */
void Chip_SYSCTL_AssertPeriphReset(CHIP_SYSCTL_PERIPH_RESET_T periph);

/**
 * @brief	De-assert reset for a peripheral
 * @param	periph	: Peripheral to de-assert reset for
 * @return	Nothing
 */
void Chip_SYSCTL_DeassertPeriphReset(CHIP_SYSCTL_PERIPH_RESET_T periph);

/**
 * @brief	Resets a peripheral
 * @param	periph	:	Peripheral to reset
 * @return	Nothing
 */
STATIC INLINE void Chip_SYSCTL_PeriphReset(CHIP_SYSCTL_PERIPH_RESET_T periph)
{
	Chip_SYSCTL_AssertPeriphReset(periph);
	Chip_SYSCTL_DeassertPeriphReset(periph);
}

/**
 * System reset status
 */
#define SYSCTL_RST_POR    (1 << 0)	/*!< POR reset status */
#define SYSCTL_RST_EXTRST (1 << 1)	/*!< External reset status */
#define SYSCTL_RST_WDT    (1 << 2)	/*!< Watchdog reset status */
#define SYSCTL_RST_BOD    (1 << 3)	/*!< Brown-out detect reset status */
#define SYSCTL_RST_SYSRST (1 << 4)	/*!< software system reset status */

/**
 * @brief	Get system reset status
 * @return	An Or'ed value of SYSCTL_RST_*
 * @note	This function returns the detected reset source(s). Mask with an
 * SYSCTL_RST_* value to determine if a reset has occurred.
 */
STATIC INLINE uint32_t Chip_SYSCTL_GetSystemRSTStatus(void)
{
	return LPC_SYSCTL->SYSRSTSTAT;
}

/**
 * @brief	Clear system reset status
 * @param	reset	: An Or'ed value of SYSCTL_RST_* statuses to clear
 * @return	Nothing
 */
STATIC INLINE void Chip_SYSCTL_ClearSystemRSTStatus(uint32_t reset)
{
	LPC_SYSCTL->SYSRSTSTAT = reset;
}

/**
 * @brief	Read POR captured PIO status at reset
 * @param	index	: POR register index 0, 1, or 2
 * @return	captured POR PIO status for ports 0, 1, or 2
 */
STATIC INLINE uint32_t Chip_SYSCTL_GetPORPIOStatus(int index)
{
	return LPC_SYSCTL->PIOPORCAP[index];
}

/**
 * Brown-out detector reset level
 */
typedef enum CHIP_SYSCTL_BODRSTLVL {
	SYSCTL_BODRSTLVL_RESERVED0,
	SYSCTL_BODRSTLVL_RESERVED1,
	SYSCTL_BODRSTLVL_2_34V,	/*!< Brown-out reset at 2.34v */
	SYSCTL_BODRSTLVL_2_64V,	/*!< Brown-out reset at 2.64v */
} CHIP_SYSCTL_BODRSTLVL_T;

/**
 * Brown-out detector interrupt level
 */
typedef enum CHIP_SYSCTL_BODRINTVAL {
	SYSCTL_BODINTVAL_RESERVED0,	
	SYSCTL_BODINTVAL_RESERVED1,	
	SYSCTL_BODINTVAL_2_55V,	/*!< Brown-out interrupt at 2.55v */
	SYSCTL_BODINTVAL_2_83V,	/*!< Brown-out interrupt at 2.83v */
} CHIP_SYSCTL_BODRINTVAL_T;

/**
 * @brief	Set brown-out detection interrupt and reset levels
 * @param	rstlvl	: Brown-out detector reset level
 * @param	intlvl	: Brown-out interrupt level
 * @return	Nothing
 * @note	Brown-out detection reset will be disabled upon exiting this function.
 * Use Chip_SYSCTL_EnableBODReset() to re-enable.
 */
STATIC INLINE void Chip_SYSCTL_SetBODLevels(CHIP_SYSCTL_BODRSTLVL_T rstlvl,
											CHIP_SYSCTL_BODRINTVAL_T intlvl)
{
	LPC_SYSCTL->BODCTRL = ((uint32_t) rstlvl) | (((uint32_t) intlvl) << 2);
}

/**
 * @brief	Enable brown-out detection reset
 * @return	Nothing
 */
STATIC INLINE void Chip_SYSCTL_EnableBODReset(void)
{
	LPC_SYSCTL->BODCTRL |= (1 << 4);
}

/**
 * @brief	Disable brown-out detection reset
 * @return	Nothing
 */
STATIC INLINE void Chip_SYSCTL_DisableBODReset(void)
{
	LPC_SYSCTL->BODCTRL &= ~(1 << 4);
}

/**
 * @brief	Set System tick timer calibration value
 * @param	sysCalVal	: System tick timer calibration value
 * @return	Nothing
 */
STATIC INLINE void Chip_SYSCTL_SetSYSTCKCAL(uint32_t sysCalVal)
{
	LPC_SYSCTL->SYSTCKCAL = sysCalVal;
}

/**
 * Non-Maskable Interrupt Enable/Disable value
 */
#define SYSCTL_NMISRC_ENABLE   (1UL << 31)	/*!< Enable the Non-Maskable Interrupt (NMI) source */

/**
 * @brief	Set source for non-maskable interrupt (NMI)
 * @param	intsrc	: IRQ number to assign to the NMI
 * @return	Nothing
 * @note	The NMI source will be disabled upon exiting this function. Use the
 * Chip_SYSCTL_EnableNMISource() function to enable the NMI source.
 */
STATIC INLINE void Chip_SYSCTL_SetNMISource(uint32_t intsrc)
{
	LPC_SYSCTL->NMISRC = 0;	/* Disable first */
	LPC_SYSCTL->NMISRC = intsrc;
}

/**
 * @brief	Enable interrupt used for NMI source
 * @return	Nothing
 */
STATIC INLINE void Chip_SYSCTL_EnableNMISource(void)
{
	LPC_SYSCTL->NMISRC |= SYSCTL_NMISRC_ENABLE;
}

/**
 * @brief	Disable interrupt used for NMI source
 * @return	Nothing
 */
STATIC INLINE void Chip_SYSCTL_DisableNMISource(void)
{
	LPC_SYSCTL->NMISRC &= ~(SYSCTL_NMISRC_ENABLE);
}

/**
 * @brief	Starts a frequency measurement cycle
 * @return	Nothing
 * @note	This function is meant to be used with the Chip_INMUX_SetFreqMeasRefClock()
 * and Chip_INMUX_SetFreqMeasTargClock() functions.
 */
STATIC INLINE void Chip_SYSCTL_StartFreqMeas(void)
{
	LPC_SYSCTL->FREQMECTRL = 0;
	LPC_SYSCTL->FREQMECTRL = (1UL << 31);
}

/**
 * @brief	Indicates when a frequency measurement cycle is complete
 * @return	true if a measurement cycle is active, otherwise false
 */
STATIC INLINE bool Chip_SYSCTL_IsFreqMeasComplete(void)
{
	return (bool) ((LPC_SYSCTL->FREQMECTRL & (1UL << 31)) == 0);
}

/**
 * @brief	Returns the raw capture value for a frequency measurement cycle
 * @return	raw cpature value (this is not a frequency)
 */
STATIC INLINE uint32_t Chip_SYSCTL_GetRawFreqMeasCapval(void)
{
	return LPC_SYSCTL->FREQMECTRL & 0x3FFF;
}

/**
 * @brief	Returns the computed value for a frequency measurement cycle
 * @param	refClockRate	: Reference clock rate used during the frequency measurement cycle
 * @return	Computed cpature value
 */
uint32_t Chip_SYSCTL_GetCompFreqMeas(uint32_t refClockRate);

/**
 * @brief FLASH Access time definitions
 */
typedef enum {
	SYSCTL_FLASHTIM_25MHZ_CPU = 0,	/*!< Flash accesses use 1 CPU clocks. Use for up to 25 MHz CPU clock*/
	SYSCTL_FLASHTIM_55MHZ_CPU = 1,	/*!< Flash accesses use 2 CPU clocks. Use for up to 55 MHz CPU clock*/
	SYSCTL_FLASHTIM_72MHZ_CPU = 2,	/*!< Flash accesses use 3 CPU clocks. Use for up to 72 MHz CPU clock*/
} SYSCTL_FLASHTIM_T;

/**
 * @brief	Set FLASH access time in clocks
 * @param	clks	: Clock cycles for FLASH access (minus 1)
 * @return	Nothing
 */
STATIC INLINE void Chip_FMC_SetFLASHAccess(SYSCTL_FLASHTIM_T clks)
{
	uint32_t tmp = LPC_SYSCTL->FLASHCFG & (~(0x3 << 12));

	/* Don't alter other bits */
	LPC_SYSCTL->FLASHCFG = tmp | ((clks & 0x03) << 12);
}

/**
 * @brief	Setup USB clock control
 * @param	ap_clk	: USB need_clock signal control (0 or 1)
 * @param	pol_clk	: USB need_clock polarity for triggering the USB wake-up interrupt (0 or 1)
 * @return	Nothing
 * @note	See the USBCLKCTRL register in the user manual for these settings.
 */
STATIC INLINE void Chip_SYSCTL_SetUSBCLKCTRL(uint32_t ap_clk, uint32_t pol_clk)
{
	LPC_SYSCTL->USBCLKCTRL = ap_clk | (pol_clk << 1);
}

/**
 * @brief	Returns the status of the USB need_clock signal
 * @return	true if USB need_clock status is high, otherwise false
 */
STATIC INLINE bool Chip_SYSCTL_GetUSBCLKStatus(void)
{
	return (bool) ((LPC_SYSCTL->USBCLKST & 0x1) != 0);
}

/**
 * Peripheral interrupt wakeup events on STARTERP0 only
 */
#define SYSCTL_ERP0_WAKEUP_WDTINT       (1 << 0)	/*!< WWDT interrupt wake-up */
#define SYSCTL_ERP0_WAKEUP_BODINT       (1 << 1)	/*!< Brown out detector interrupt wake-up */
#define SYSCTL_ERP0_WAKEUP_GINT0INT     (1 << 5)	/*!< Group interrupt 0 wake-up */
#define SYSCTL_ERP0_WAKEUP_GINT1INT     (1 << 6)	/*!< Group interrupt 1 wake-up */
#define SYSCTL_ERP0_WAKEUP_PINT0INT     (1 << 7)	/*!< GPIO pin interrupt 0 wake-up */
#define SYSCTL_ERP0_WAKEUP_PINT1INT     (1 << 8)	/*!< GPIO pin interrupt 1 wake-up */
#define SYSCTL_ERP0_WAKEUP_PINT2INT     (1 << 9)	/*!< GPIO pin interrupt 2 wake-up */
#define SYSCTL_ERP0_WAKEUP_PINT3INT     (1 << 10)	/*!< GPIO pin interrupt 3 wake-up */
#define SYSCTL_ERP0_WAKEUP_PINT4INT     (1 << 11)	/*!< GPIO pin interrupt 4 wake-up */
#define SYSCTL_ERP0_WAKEUP_PINT5INT     (1 << 12)	/*!< GPIO pin interrupt 5 wake-up */
#define SYSCTL_ERP0_WAKEUP_PINT6INT     (1 << 13)	/*!< GPIO pin interrupt 6 wake-up */
#define SYSCTL_ERP0_WAKEUP_PINT7INT     (1 << 14)	/*!< GPIO pin interrupt 7 wake-up */
#define SYSCTL_ERP0_WAKEUP_USART0INT    (1 << 21)	/*!< USART0 interrupt wake-up */
#define SYSCTL_ERP0_WAKEUP_USART1INT    (1 << 22)	/*!< USART1 interrupt wake-up */
#define SYSCTL_ERP0_WAKEUP_USART2INT    (1 << 23)	/*!< USART2 interrupt wake-up */
#define SYSCTL_ERP0_WAKEUP_I2CINT       (1 << 24)	/*!< I2C interrupt wake-up */
#define SYSCTL_ERP0_WAKEUP_SPI0INT      (1 << 25)	/*!< SPI0 interrupt wake-up */
#define SYSCTL_ERP0_WAKEUP_SPI1INT      (1 << 26)	/*!< SPI1 interrupt wake-up */
#define SYSCTL_ERP0_WAKEUP_USB_WAKEUP   (1 << 30)	/*!< USB need_clock signal wake-up */

/**
 * @brief	Enables a peripheral's wakeup logic (STARTERP0 only)
 * @param	periphmask	: OR'ed values of SYSCTL_ERP0_* for wakeup
 * @return	Nothing
 * @note	Use this function only with definitions of type SYSCTL_ERP0_*. Do
 * not use or mix with SYSCTL_ERP1_* definitions.
 */
STATIC INLINE void Chip_SYSCTL_EnableERP0PeriphWakeup(uint32_t periphmask)
{
	LPC_SYSCTL->STARTERP[0] |= periphmask;
}

/**
 * @brief	Disables a peripheral's wakeup logic (STARTERP0 only)
 * @param	periphmask	: OR'ed values of SYSCTL_ERP0_* for wakeup
 * @return	Nothing
 * @note	Use this function only with definitions of type SYSCTL_ERP0_*. Do
 * not use or mix with SYSCTL_ERP1_* definitions.
 */
STATIC INLINE void Chip_SYSCTL_DisableERP0PeriphWakeup(uint32_t periphmask)
{
	LPC_SYSCTL->STARTERP[0] &= ~periphmask;
}

/**
 * Peripheral interrupt wakeup events on STARTERP1 only
 */
#define SYSCTL_ERP1_WAKEUP_ACMP0INT     (1 << 8)	/*!< Analog comparator 0 interrupt wake-up */
#define SYSCTL_ERP1_WAKEUP_ACMP1INT     (1 << 9)	/*!< Analog comparator 1 interrupt wake-up */
#define SYSCTL_ERP1_WAKEUP_ACMP2INT     (1 << 10)	/*!< Analog comparator 2 interrupt wake-up */
#define SYSCTL_ERP1_WAKEUP_ACMP3INT     (1 << 11)	/*!< Analog comparator 3 interrupt wake-up */
#define SYSCTL_ERP1_WAKEUP_RTCALARMINT  (1 << 13)	/*!< RTC alarm interrupt wake-up */
#define SYSCTL_ERP1_WAKEUP_RTCWAKEINT   (1 << 14)	/*!< RTC wake (1KHz wake) interrupt wake-up */

/**
 * @brief	Enables a peripheral's wakeup logic (STARTERP0 only)
 * @param	periphmask	: OR'ed values of SYSCTL_ERP1_* for wakeup
 * @return	Nothing
 * @note	Use this function only with definitions of type SYSCTL_ERP1_*. Do
 * not use or mix with SYSCTL_ERP1_* definitions.
 */
STATIC INLINE void Chip_SYSCTL_EnableERP1PeriphWakeup(uint32_t periphmask)
{
	LPC_SYSCTL->STARTERP[1] |= periphmask;
}

/**
 * @brief	Disables a peripheral's wakeup logic (STARTERP1 only)
 * @param	periphmask	: OR'ed values of SYSCTL_ERP1_* for wakeup
 * @return	Nothing
 * @note	Use this function only with definitions of type SYSCTL_ERP1_*. Do
 * not use or mix with SYSCTL_ERP1_* definitions.
 */
STATIC INLINE void Chip_SYSCTL_DisableERP1PeriphWakeup(uint32_t periphmask)
{
	LPC_SYSCTL->STARTERP[1] &= ~periphmask;
}

/**
 * Deep sleep to wakeup setup values
 */
#define SYSCTL_SLPWAKE_TBD0_PD      (1 << 0)	/*!< TBD0 wake-up configuration */
#define SYSCTL_SLPWAKE_TBD1_PD      (1 << 1)	/*!< TBD1 wake-up configuration */
#define SYSCTL_SLPWAKE_TBD2_PD      (1 << 2)	/*!< TBD2 wake-up configuration */
#define SYSCTL_SLPWAKE_IRCOUT_PD    (1 << 3)	/*!< IRC oscillator output wake-up configuration */
#define SYSCTL_SLPWAKE_IRC_PD       (1 << 4)	/*!< IRC oscillator power-down wake-up configuration */
#define SYSCTL_SLPWAKE_FLASH_PD     (1 << 5)	/*!< Flash wake-up configuration */
#define SYSCTL_SLPWAKE_EEPROM_PD    (1 << 6)	/*!< EEPROM wake-up configuration */
#define SYSCTL_SLPWAKE_BOD_PD       (1 << 8)	/*!< BOD wake-up configuration */
#define SYSCTL_SLPWAKE_USBPHY_PD    (1 << 9)	/*!< USB PHY wake-up configuration */
#define SYSCTL_SLPWAKE_ADC0_PD      (1 << 10)	/*!< ADC0 wake-up configuration */
#define SYSCTL_SLPWAKE_ADC1_PD      (1 << 11)	/*!< ADC1 wake-up configuration */
#define SYSCTL_SLPWAKE_DAC_PD       (1 << 12)	/*!< DAC wake-up configuration */
#define SYSCTL_SLPWAKE_ACMP0_PD     (1 << 13)	/*!< ACMP0 wake-up configuration */
#define SYSCTL_SLPWAKE_ACMP1_PD     (1 << 14)	/*!< ACMP0 wake-up configuration */
#define SYSCTL_SLPWAKE_ACMP2_PD     (1 << 15)	/*!< ACMP0 wake-up configuration */
#define SYSCTL_SLPWAKE_ACMP3_PD     (1 << 16)	/*!< ACMP0 wake-up configuration */
#define SYSCTL_SLPWAKE_IREF_PD      (1 << 17)	/*!< Internal voltage reference wake-up configuration */
#define SYSCTL_SLPWAKE_TS_PD        (1 << 18)	/*!< Temperature sensor wake-up configuration */
#define SYSCTL_SLPWAKE_VDDADIV_PD   (1 << 19)	/*!< VDDA divider wake-up configuration */
#define SYSCTL_SLPWAKE_WDTOSC_PD    (1 << 20)	/*!< Watchdog oscillator wake-up configuration */
#define SYSCTL_SLPWAKE_SYSOSC_PD    (1 << 21)	/*!< System oscillator wake-up configuration */
#define SYSCTL_SLPWAKE_SYSPLL_PD    (1 << 22)	/*!< System PLL wake-up configuration */
#define SYSCTL_SLPWAKE_USBPLL_PD    (1 << 23)	/*!< USB PLL wake-up configuration */
#define SYSCTL_SLPWAKE_SCTPLL_PD    (1 << 24)	/*!< SCT PLL wake-up configuration */

/**
 * @brief	Setup wakeup behaviour from deep sleep
 * @param	wakeupmask	: OR'ed values of SYSCTL_SLPWAKE_* values (high is powered down)
 * @return	Nothing
 * @note	This must be setup prior to using deep sleep. See the user manual
 * (PDWAKECFG register) for more info on setting this up. This function selects
 * which peripherals are powered up on exit from deep sleep.
 * This function should only be called once with all options for wakeup
 * in that call.
 */
void Chip_SYSCTL_SetWakeup(uint32_t wakeupmask);

/**
 * @brief	Return current wakeup mask
 * @return	OR'ed values of SYSCTL_SLPWAKE_* values
 * @note	A high state indicates the peripehral will powerup on wakeup.
 */
STATIC INLINE uint32_t Chip_SYSCTL_GetWakeup(void)
{
	return LPC_SYSCTL->PDWAKECFG;
}

/**
 * Power down configuration values
 */
#define SYSCTL_POWERDOWN_TBD0_PD    (1 << 0)	/*!< TBD0 wake-up power down */
#define SYSCTL_POWERDOWN_TBD1_PD    (1 << 1)	/*!< TBD1 wake-up power down */
#define SYSCTL_POWERDOWN_TBD2_PD    (1 << 2)	/*!< TBD2 wake-up power down */
#define SYSCTL_POWERDOWN_IRCOUT_PD  (1 << 3)	/*!< IRC oscillator output wake-up power down */
#define SYSCTL_POWERDOWN_IRC_PD     (1 << 4)	/*!< IRC oscillator power-down wake-up power down */
#define SYSCTL_POWERDOWN_FLASH_PD   (1 << 5)	/*!< Flash wake-up power down */
#define SYSCTL_POWERDOWN_EEPROM_PD  (1 << 6)	/*!< EEPROM wake-up power down */
#define SYSCTL_POWERDOWN_BOD_PD     (1 << 8)	/*!< BOD wake-up power down */
#define SYSCTL_POWERDOWN_USBPHY_PD  (1 << 9)	/*!< USB PHY wake-up power down */
#define SYSCTL_POWERDOWN_ADC0_PD    (1 << 10)	/*!< ADC0 wake-up power down */
#define SYSCTL_POWERDOWN_ADC1_PD    (1 << 11)	/*!< ADC1 wake-up power down */
#define SYSCTL_POWERDOWN_DAC_PD     (1 << 12)	/*!< DAC wake-up power down */
#define SYSCTL_POWERDOWN_ACMP0_PD   (1 << 13)	/*!< ACMP0 wake-up power down */
#define SYSCTL_POWERDOWN_ACMP1_PD   (1 << 14)	/*!< ACMP0 wake-up power down */
#define SYSCTL_POWERDOWN_ACMP2_PD   (1 << 15)	/*!< ACMP0 wake-up power down */
#define SYSCTL_POWERDOWN_ACMP3_PD   (1 << 16)	/*!< ACMP0 wake-up power down */
#define SYSCTL_POWERDOWN_IREF_PD    (1 << 17)	/*!< Internal voltage reference wake-up power down */
#define SYSCTL_POWERDOWN_TS_PD      (1 << 18)	/*!< Temperature sensor wake-up power down */
#define SYSCTL_POWERDOWN_VDDADIV_PD (1 << 19)	/*!< VDDA divider wake-up power down */
#define SYSCTL_POWERDOWN_WDTOSC_PD  (1 << 20)	/*!< Watchdog oscillator wake-up power down */
#define SYSCTL_POWERDOWN_SYSOSC_PD  (1 << 21)	/*!< System oscillator wake-up power down */
#define SYSCTL_POWERDOWN_SYSPLL_PD  (1 << 22)	/*!< System PLL wake-up power down */
#define SYSCTL_POWERDOWN_USBPLL_PD  (1 << 23)	/*!< USB PLL wake-up power down */
#define SYSCTL_POWERDOWN_SCTPLL_PD  (1 << 24)	/*!< SCT PLL wake-up power down */

/**
 * @brief	Power down one or more blocks or peripherals
 * @param	powerdownmask	: OR'ed values of SYSCTL_POWERDOWN_* values
 * @return	Nothing
 */
void Chip_SYSCTL_PowerDown(uint32_t powerdownmask);

/**
 * @brief	Power up one or more blocks or peripherals
 * @param	powerupmask	: OR'ed values of SYSCTL_POWERDOWN_* values
 * @return	Nothing
 */
void Chip_SYSCTL_PowerUp(uint32_t powerupmask);

/**
 * @brief	Get power status
 * @return	OR'ed values of SYSCTL_POWERDOWN_* values
 * @note	A high state indicates the peripheral is powered down.
 */
STATIC INLINE uint32_t Chip_SYSCTL_GetPowerStates(void)
{
	return LPC_SYSCTL->PDRUNCFG;
}

/**
 * @brief	Return the JTAG ID code
 * @return	the JTAG ID code
 */
STATIC INLINE uint32_t Chip_SYSCTL_GetJTAGIDCode(void)
{
	return LPC_SYSCTL->JTAG_IDCODE;
}

/**
 * @brief	Return the device ID
 * @param	index	: Index of device ID to get, 0 or 1
 * @return	the device ID
 */
STATIC INLINE uint32_t Chip_SYSCTL_GetDeviceID(int index)
{
	return LPC_SYSCTL->DEVICEID[index];
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /*!< __SYSCTL_15XX_H_ */
