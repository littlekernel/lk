/*
 * @brief Grouped GPIO Interrupt example
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

#if defined(BOARD_NXP_LPCXPRESSO_1549)

/* GPIO pin for GROUPED GPIO interrupt.  This is SW1-WAKE button switch input. */
#define TEST_BUTTON_PIN         17	/* GPIO pin number mapped to PININT */
#define TEST_BUTTON_PORT        0	/* GPIO port number mapped to PININT */

#else
#error "Grouped GPIO Interrupt not configured for this example"
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
 * @brief	Handle Group GPIO 0 interrupt
 * @return	Nothing
 */
void GINT0_IRQHandler(void)
{
	Chip_GPIOGP_ClearIntStatus(LPC_GPIOGROUP, 0);
	Board_LED_Toggle(0);
}

/**
 * @brief	Main program body
 * @return	Does not return
 */
int main(void)
{
	/* Generic Initialization */
	SystemCoreClockUpdate();
	Board_Init();
	Board_LED_Set(0, false);

	/* Initialize GPIO grouped interrupts */
	Chip_GPIOGP_Init(LPC_GPIOGROUP);

	/* Set pin back to GPIO (on some boards may have been changed to something
	   else by Board_Init()) */
	Chip_IOCON_PinMuxSet(LPC_IOCON, TEST_BUTTON_PORT, TEST_BUTTON_PIN,
						 (IOCON_DIGMODE_EN | IOCON_MODE_INACT) );

	/* Group GPIO interrupt 0 will be invoked when the button is pressed. */
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, TEST_BUTTON_PORT, TEST_BUTTON_PIN);
	Chip_GPIOGP_SelectLowLevel(LPC_GPIOGROUP, 0, TEST_BUTTON_PORT, 1 << TEST_BUTTON_PIN);
	Chip_GPIOGP_EnableGroupPins(LPC_GPIOGROUP, 0, TEST_BUTTON_PORT, 1 << TEST_BUTTON_PIN);
	Chip_GPIOGP_SelectAndMode(LPC_GPIOGROUP, 0);
	Chip_GPIOGP_SelectEdgeMode(LPC_GPIOGROUP, 0);

	/* Enable Group GPIO interrupt 0 */
	NVIC_EnableIRQ(GINT0_IRQn);

	/* Spin in a loop here.  All the work is done in ISR. */
	while (1) {}

	/* Does not return */
	return 0;
}
