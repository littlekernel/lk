/*
 * @brief CLKOUT example
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

#include "board.h"
#include <stdio.h>

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

static volatile uint32_t ticks100;

#if defined(BOARD_NXP_LPCXPRESSO_1549)
/* Use this pin for clockout */
#define CLKOUT_PORT 1
#define CLKOUT_PIN  0

#else
#error "No port/pin defined for the LPC15xx CLKOUT example"
#endif

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle interrupt from SysTick timer
 * @return	Nothing
 */
void SysTick_Handler(void)
{
	ticks100++;
	if ((ticks100 % 100) == 0) {
		Board_LED_Toggle(0);
	}
}

/**
 * @brief	main routine for CLKOUT example
 * @return	Function should not exit.
 */
int main(void)
{
	CHIP_SYSCTL_CLKOUTSRC_T clkoutClks;

	SystemCoreClockUpdate();
	Board_Init();

	Board_LED_Set(0, false);

	/* Enable and setup SysTick Timer at a 100Hz rate */
	SysTick_Config(Chip_Clock_GetSysTickClockRate() / 100);

	/* Enable the power to the WDT */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_WDTOSC_PD);
	/* Setup SCT PLL */
	Chip_SYSCTL_PowerDown(SYSCTL_POWERDOWN_SCTPLL_PD);
	Chip_Clock_SetSCTPLLSource(SYSCTL_PLLCLKSRC_MAINOSC);
	Chip_Clock_SetupSCTPLL(5, 2);
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_SCTPLL_PD);
	/* Wait for PLL to lock */
	while (!Chip_Clock_IsSCTPLLLocked()) {}
	/* Enable RTC Oscillator */
	Chip_Clock_EnableRTCOsc();

	/* Enable SWM clocking prior to switch matrix operations */
	Chip_SWM_Init();
	Chip_GPIO_Init(LPC_GPIO);

	/* Setup pin as CLKOUT */
	Chip_SWM_MovablePortPinAssign(SWM_CLK_OUT_O, CLKOUT_PORT, CLKOUT_PIN);

	/* Configure as a digital pin with no pullups/pulldowns */
	Chip_IOCON_PinMuxSet(LPC_IOCON, CLKOUT_PORT, CLKOUT_PIN,
						 (IOCON_MODE_INACT | IOCON_DIGMODE_EN));

	/* Cycle through all clock sources for the CLKOUT pin */
	while (1) {
		for (clkoutClks = SYSCTL_CLKOUTSRC_IRC;
			 clkoutClks <= SYSCTL_CLKOUTSRC_RTC32K; clkoutClks++) {

			/* Setup CLKOUT pin for specific clock with a divider of 1 */
			Chip_Clock_SetCLKOUTSource(clkoutClks, 1);

			/* Wait 5 seconds */
			ticks100 = 0;
			while (ticks100 < 500) {
				__WFI();
			}
		}
	}

	/* Disable CLKOUT pin by setting divider to 0 */
	Chip_Clock_SetCLKOUTSource(SYSCTL_CLKOUTSRC_MAINSYSCLK, 0);

	return 0;
}
