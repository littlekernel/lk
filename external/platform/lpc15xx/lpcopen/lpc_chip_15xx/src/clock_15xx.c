/*
 * @brief LPC15XX System clock control functions
 *
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
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
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */

#include "chip.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Compute a PLL frequency */
STATIC uint32_t Chip_Clock_GetPLLFreq(uint32_t PLLReg, uint32_t inputRate)
{
	uint32_t msel = ((PLLReg & 0x3F) + 1);

	return inputRate * msel;
}

/* Return a PLL input (common) */
STATIC uint32_t Chip_Clock_GetPLLInClockRate(uint32_t reg)
{
	uint32_t clkRate;

	switch ((CHIP_SYSCTL_PLLCLKSRC_T) (reg & 0x3)) {
	case SYSCTL_PLLCLKSRC_IRC:
		clkRate = Chip_Clock_GetIntOscRate();
		break;

	case SYSCTL_PLLCLKSRC_MAINOSC:
		clkRate = Chip_Clock_GetMainOscRate();
		break;

	default:
		clkRate = 0;
	}

	return clkRate;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Return System PLL input clock rate */
uint32_t Chip_Clock_GetSystemPLLInClockRate(void)
{
	return Chip_Clock_GetPLLInClockRate(LPC_SYSCTL->SYSPLLCLKSEL);
}

/* Return System PLL output clock rate */
uint32_t Chip_Clock_GetSystemPLLOutClockRate(void)
{
	return Chip_Clock_GetPLLFreq(LPC_SYSCTL->SYSPLLCTRL,
								 Chip_Clock_GetSystemPLLInClockRate());
}

/* Return USB PLL input clock rate */
uint32_t Chip_Clock_GetUSBPLLInClockRate(void)
{
	return Chip_Clock_GetPLLInClockRate(LPC_SYSCTL->USBPLLCLKSEL);
}

/* Return USB PLL output clock rate */
uint32_t Chip_Clock_GetUSBPLLOutClockRate(void)
{
	return Chip_Clock_GetPLLFreq(LPC_SYSCTL->USBPLLCTRL,
								 Chip_Clock_GetUSBPLLInClockRate());
}

/* Return SCT PLL input clock rate */
uint32_t Chip_Clock_GetSCTPLLInClockRate(void)
{
	return Chip_Clock_GetPLLInClockRate(LPC_SYSCTL->SCTPLLCLKSEL);
}

/* Return SCT PLL output clock rate */
uint32_t Chip_Clock_GetSCTPLLOutClockRate(void)
{
	return Chip_Clock_GetPLLFreq(LPC_SYSCTL->SCTPLLCTRL,
								 Chip_Clock_GetSCTPLLInClockRate());
}

/* Return main A clock rate */
uint32_t Chip_Clock_GetMain_A_ClockRate(void)
{
	uint32_t clkRate = 0;

	switch (Chip_Clock_GetMain_A_ClockSource()) {
	case SYSCTL_MAIN_A_CLKSRC_IRC:
		clkRate = Chip_Clock_GetIntOscRate();
		break;

	case SYSCTL_MAIN_A_CLKSRCA_MAINOSC:
		clkRate = Chip_Clock_GetMainOscRate();
		break;

	case SYSCTL_MAIN_A_CLKSRCA_WDTOSC:
		clkRate = Chip_Clock_GetWDTOSCRate();
		break;

	default:
		clkRate = 0;
		break;
	}

	return clkRate;
}

/* Return main B clock rate */
uint32_t Chip_Clock_GetMain_B_ClockRate(void)
{
	uint32_t clkRate = 0;

	switch (Chip_Clock_GetMain_B_ClockSource()) {
	case SYSCTL_MAIN_B_CLKSRC_MAINCLKSELA:
		clkRate = Chip_Clock_GetMain_A_ClockRate();
		break;

	case SYSCTL_MAIN_B_CLKSRC_SYSPLLIN:
		clkRate = Chip_Clock_GetSystemPLLInClockRate();
		break;

	case SYSCTL_MAIN_B_CLKSRC_SYSPLLOUT:
		clkRate = Chip_Clock_GetSystemPLLOutClockRate();
		break;

	case SYSCTL_MAIN_B_CLKSRC_RTC:
		clkRate = Chip_Clock_GetRTCOscRate();
		break;
	}

	return clkRate;
}

/* Set main system clock source */
void Chip_Clock_SetMainClockSource(CHIP_SYSCTL_MAINCLKSRC_T src)
{
	uint32_t clkSrc = (uint32_t) src;

	if (clkSrc >= 4) {
		/* Main B source only, not using main A */
		Chip_Clock_SetMain_B_ClockSource((CHIP_SYSCTL_MAIN_B_CLKSRC_T) (clkSrc - 4));
	}
	else {
		/* Select main A clock source and set main B source to use main A */
		Chip_Clock_SetMain_A_ClockSource((CHIP_SYSCTL_MAIN_A_CLKSRC_T) clkSrc);
		Chip_Clock_SetMain_B_ClockSource(SYSCTL_MAIN_B_CLKSRC_MAINCLKSELA);
	}
}

/* Returns the main clock source */
CHIP_SYSCTL_MAINCLKSRC_T Chip_Clock_GetMainClockSource(void)
{
	CHIP_SYSCTL_MAIN_B_CLKSRC_T srcB;
	uint32_t clkSrc;

	/* Get main B clock source */
	srcB = Chip_Clock_GetMain_B_ClockSource();
	if (srcB == SYSCTL_MAIN_B_CLKSRC_MAINCLKSELA) {
		/* Using source A, so return source A */
		clkSrc = (uint32_t) Chip_Clock_GetMain_A_ClockSource();
	}
	else {
		/* Using source B */
		clkSrc = 4 + (uint32_t) srcB;
	}

	return (CHIP_SYSCTL_MAINCLKSRC_T) clkSrc;
}

/* Return main clock rate */
uint32_t Chip_Clock_GetMainClockRate(void)
{
	uint32_t clkRate;

	if (Chip_Clock_GetMain_B_ClockSource() == SYSCTL_MAIN_B_CLKSRC_MAINCLKSELA) {
		/* Return main A clock rate */
		clkRate = Chip_Clock_GetMain_A_ClockRate();
	}
	else {
		/* Return main B clock rate */
		clkRate = Chip_Clock_GetMain_B_ClockRate();
	}

	return clkRate;
}

/* Return ADC asynchronous clock rate */
uint32_t Chip_Clock_GetADCASYNCRate(void)
{
	uint32_t clkRate = 0;

	switch (Chip_Clock_GetADCASYNCSource()) {
	case SYSCTL_ADCASYNCCLKSRC_IRC:
		clkRate = Chip_Clock_GetIntOscRate();
		break;

	case SYSCTL_ADCASYNCCLKSRC_SYSPLLOUT:
		clkRate = Chip_Clock_GetSystemPLLOutClockRate();
		break;

	case SYSCTL_ADCASYNCCLKSRC_USBPLLOUT:
		clkRate = Chip_Clock_GetUSBPLLOutClockRate();
		break;

	case SYSCTL_ADCASYNCCLKSRC_SCTPLLOUT:
		clkRate = Chip_Clock_GetSCTPLLOutClockRate();
		break;
	}

	return clkRate;
}

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
void Chip_Clock_SetCLKOUTSource(CHIP_SYSCTL_CLKOUTSRC_T src, uint32_t div)
{
	uint32_t srcClk = (uint32_t) src;

	/* Use a clock A source? */
	if (src >= 4) {
		/* Not using a CLKOUT A source */
		LPC_SYSCTL->CLKOUTSEL[1] = srcClk - 4;
	}
	else {
		/* Using a clock A source, select A and then switch B to A */
		LPC_SYSCTL->CLKOUTSEL[0] = srcClk;
		LPC_SYSCTL->CLKOUTSEL[1] = 0;
	}

	LPC_SYSCTL->CLKOUTDIV = div;
}

/* Enable a system or peripheral clock */
void Chip_Clock_EnablePeriphClock(CHIP_SYSCTL_CLOCK_T clk)
{
	uint32_t clkEnab = (uint32_t) clk;

	if (clkEnab >= 32) {
		LPC_SYSCTL->SYSAHBCLKCTRL[1] |= (1 << (clkEnab - 32));
	}
	else {
		LPC_SYSCTL->SYSAHBCLKCTRL[0] |= (1 << clkEnab);
	}
}

/* Disable a system or peripheral clock */
void Chip_Clock_DisablePeriphClock(CHIP_SYSCTL_CLOCK_T clk)
{
	uint32_t clkEnab = (uint32_t) clk;

	if (clkEnab >= 32) {
		LPC_SYSCTL->SYSAHBCLKCTRL[1] &= ~(1 << (clkEnab - 32));
	}
	else {
		LPC_SYSCTL->SYSAHBCLKCTRL[0] &= ~(1 << clkEnab);
	}
}

/* Returns the system tick rate as used with the system tick divider */
uint32_t Chip_Clock_GetSysTickClockRate(void)
{
	uint32_t sysRate, div;

	div = Chip_Clock_GetSysTickClockDiv();

	/* If divider is 0, the system tick clock is disabled */
	if (div == 0) {
		sysRate = 0;
	}
	else {
		sysRate = Chip_Clock_GetMainClockRate() / div;
	}

	return sysRate;
}

/* Get UART base rate */
uint32_t Chip_Clock_GetUARTBaseClockRate(void)
{
	uint64_t inclk;
	uint32_t div;

	div = (uint32_t) Chip_Clock_GetUARTFRGDivider();
	if (div == 0) {
		/* Divider is 0 so UART clock is disabled */
		inclk = 0;
	}
	else {
		uint32_t mult, divmult;

		/* Input clock into FRG block is the divided main system clock */
		inclk = (uint64_t) (Chip_Clock_GetMainClockRate() / div);

		divmult = LPC_SYSCTL->FRGCTRL & 0xFFFF;
		if ((divmult & 0xFF) == 0xFF) {
			/* Fractional part is enabled, get multiplier */
			mult = (divmult >> 8) & 0xFF;

			/* Get fractional error */
			inclk = (inclk * 256) / (uint64_t) (256 + mult);
		}
	}

	return (uint32_t) inclk;
}

/* Set UART base rate */
uint32_t Chip_Clock_SetUARTBaseClockRate(uint32_t rate, bool fEnable)
{
	uint32_t div, inclk;

	/* Input clock into FRG block is the main system cloock */
	inclk = Chip_Clock_GetMainClockRate();

	/* Get integer divider for coarse rate */
	div = inclk / rate;
	if (div == 0) {
		div = 1;
	}

	/* Approximated rate with only integer divider */
	Chip_Clock_SetUARTFRGDivider((uint8_t) div);

	if (fEnable) {
		uint32_t err;
		uint64_t uart_fra_multiplier;

		err = inclk - (rate * div);
		uart_fra_multiplier = ((uint64_t) err  * 256) / (uint64_t) (rate * div);

		/* Enable fractional divider and set multiplier */
		LPC_SYSCTL->FRGCTRL = 0xFF | ((uart_fra_multiplier & 0xFF) << 8);
	}
	else {
		/* Disable fractional generator and use integer divider only */
		LPC_SYSCTL->FRGCTRL = 0;
	}

	return Chip_Clock_GetUARTBaseClockRate();
}

/* Bypass System Oscillator and set oscillator frequency range */
void Chip_Clock_SetPLLBypass(bool bypass, bool highfr)
{
	uint32_t ctrl = 0;

	if (bypass) {
		ctrl |= (1 << 0);
	}
	if (highfr) {
		ctrl |= (1 << 1);
	}

	LPC_SYSCTL->SYSOSCCTRL = ctrl;
}

/* Return system clock rate */
uint32_t Chip_Clock_GetSystemClockRate(void)
{
	/* No point in checking for divide by 0 */
	return Chip_Clock_GetMainClockRate() / LPC_SYSCTL->SYSAHBCLKDIV;
}
