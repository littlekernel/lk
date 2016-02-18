/*
 * @brief General Purpose Input/Output example
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

/* Number of tickrate per second */
#define TICKRATE_HZ (10)

#if defined(BOARD_NXP_LPCXPRESSO_1549)
/* LPCXpresso LPC1549 board has LEDs on P0_0, P0_1, and P0_24. We'll toggle them
   all at once. */
#define PORT_MASK       ((1 << 0) | (1 << 1) | (1 << 24))

#else
#error "No PORT_MASK defined"
#endif /* defined(BOARD_NXP_LPCXPRESSO_1549) */

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
	uint32_t states;

	/* Get current masked port states */
	states = Chip_GPIO_GetMaskedPortValue(LPC_GPIO, 0);

	/* Toggle all the states */
	states = ~states;

	/* Write states back via masked set function. Only the enanled
	   (masked states) will be changed. */
	Chip_GPIO_SetMaskedPortValue(LPC_GPIO, 0, states);
}

/**
 * @brief	Main program body
 * @return	Does not return
 */
int main(void) {

	/* Generic Initialization */
	SystemCoreClockUpdate();
	Board_Init();

	/* Chip_GPIO_Init(LPC_GPIO) is called as part of Board_Init() */

	/* Set port 0 pins to the output direction */
	Chip_GPIO_SetPortDIROutput(LPC_GPIO, 0, PORT_MASK);

	/* Set GPIO port mask value to make sure only port 0
	    selected pins are activated during writes */
	Chip_GPIO_SetPortMask(LPC_GPIO, 0, ~PORT_MASK);

	/* Enable SysTick Timer */
	SysTick_Config(SystemCoreClock / TICKRATE_HZ);

	/* All work happens in the systick interrupt handler */
	while (1) {
		__WFI();
	}

	return 0;
}
