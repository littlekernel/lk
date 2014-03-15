/*
 * @brief Freqeuency measurement example
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

#include "board.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Measurement cycle with value display */
static uint32_t measureDisplay(char *str, FREQMSR_SRC_T src, uint32_t freqRef)
{
	uint32_t freqComp;

	/* Setup to measure the selected target */
	Chip_INMUX_SetFreqMeasTargClock(src);

	/* Start a measurement cycle and wait for it to complete. If the target
	   clock is not running, the measurement cycle will remain active
	   forever, so a timeout may be necessary if the target clock can stop */
	Chip_SYSCTL_StartFreqMeas();
	while (!Chip_SYSCTL_IsFreqMeasComplete()) {}

	/* Get computed frequency */
	freqComp = Chip_SYSCTL_GetCompFreqMeas(freqRef);

	/* Show the raw capture value and the compute frequency */
	DEBUGOUT("Capture source : %s, reference frequency = %dHz\r\n", str, freqRef);
	DEBUGOUT("Raw frequency capture value = %d\r\n", Chip_SYSCTL_GetRawFreqMeasCapval());
	DEBUGOUT("Computed frequency value = %dHz\r\n\r\n", freqComp);

	return freqComp;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Application main function
 * @return	Does not return
 */
int main(void)
{
	uint32_t freqRef;

	/* Board Initialization */
	SystemCoreClockUpdate();
	Board_Init();

	/* Enable watchdog oscillator needed for measurement later. Enabling it
	   early allows it to settle. */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_WDTOSC_PD);

	/* Setup to use the main oscillator for the frequency reference since
	   we already know this rate */
	Chip_INMUX_SetFreqMeasRefClock(FREQMSR_MAIN_OSC);
	freqRef = Chip_Clock_GetMainOscRate();

	/* Start RTC oscillator frequency measurement and display results */
	measureDisplay("RTC32K oscillator (main osc reference)", FREQMSR_32KHZOSC, freqRef);

	/* Start watchdog oscillator frequency measurement and display results */
	measureDisplay("Watchdog oscillator (main osc reference)", FREQMSR_WDOSC, freqRef);

	/* Note that the accuracy of a target measurement requires a reference
	   clock rate faster than then target clock, so using the main oscillator
	   source (typically 12MHz) can't be used to get the IRC clock rate. To
	   get around this, the main PLL can be used as a reference clock by
	   routing the PLL output to CLKOUT and then using the switch matrix
	   pin assign function to map CLKOUT to one of the frequency measurement
	   input mux pins. The sequence below shows how to do this and measure
	   the watchdog oscillator and internal RC oscillator frequencies. */

	/* Route the CLKOUT pin to pin PIO0_5 */
	Chip_SWM_MovablePortPinAssign(SWM_CLK_OUT_O, 0, 5);

	/* Enable CLKOUT with the PLL output (main system clock) and a divider of 2 */
	Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 2);

	/* Set reference frequency to the clock on PIO0_5 */
	Chip_INMUX_SetFreqMeasRefClock(FREQMSR_PIO0_5);
	freqRef = Chip_Clock_GetSystemClockRate() / 2;

	/* Start RTC oscillator frequency measurement and display results */
	measureDisplay("RTC32K oscillator (CLKOUT reference)", FREQMSR_32KHZOSC, freqRef);

	/* Start watchdog oscillator frequency measurement and display results */
	measureDisplay("Watchdog oscillator (CLKOUT reference)", FREQMSR_WDOSC, freqRef);

	/* Try a few measurements with the IRC and it's computed frequency */
	Chip_INMUX_SetFreqMeasRefClock(FREQMSR_IRC);
	freqRef = Chip_Clock_GetIntOscRate();

	/* Start RTC oscillator frequency measurement and display results */
	measureDisplay("RTC32K oscillator (IRC reference)", FREQMSR_32KHZOSC, freqRef);

	/* Start watchdog oscillator frequency measurement and display results */
	measureDisplay("Watchdog oscillator (IRC reference)", FREQMSR_WDOSC, freqRef);

	return 0;
}
