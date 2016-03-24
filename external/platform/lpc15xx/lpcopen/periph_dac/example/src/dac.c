/*
 * @brief Digital to Analog converter example.
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

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* DAC internal timer reload value to generate interrupts at 2KHz or 0.5ms*/
#define DAC_TIM_RELOAD ((uint32_t) 0x8C9F)
/* Increments for DAC input at every interrupt to generate a saw tooth wave of 100Hz*/
#define DAC_IN_UPDATE  ((uint32_t) 215)

static volatile uint32_t dac_input;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* DAC OUT Pin mux function - note that SystemInit() may already setup your
   pin muxing at system startup */
static void Init_DAC_PinMux(void)
{
#if defined(BOARD_NXP_LPCXPRESSO_1549)
	/* Disables pullups/pulldowns and disable digital mode */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 12, (IOCON_MODE_INACT | IOCON_ADMODE_EN));

	/* Assign DAC_OUT to P0.12 via SWM (fixed pin) */
	Chip_SWM_EnableFixedPin(SWM_FIXED_DAC_OUT);
#else
	/* Configure your own ACMP pin muxing here if needed */
#warning "No DAC Out pin muxing defined"
#endif
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	DAC interrupt handler sub-routine
 * @return	Nothing
 */
void DAC_IRQHandler(void)
{
	if (Chip_DAC_GetIntStatus(LPC_DAC)) {
		/* Update DAC Input value*/
		dac_input += DAC_IN_UPDATE;
		/* Boundary check for max DAC input*/
		if (dac_input > 4095) {
			/* Cycle DAC values */
			dac_input = 0;
		}
		/* Updating DAC Value register will clear interrupt */
		Chip_DAC_UpdateValue(LPC_DAC, dac_input);
	}
}

/**
 * @brief	Main program body
 * @return	Does not return
 */
int main(void)
{
	/* initialize the board */
	SystemCoreClockUpdate();
	Board_Init();

	/* initialize the DAC */
	Chip_DAC_Init(LPC_DAC);
	/* Setup board specific DAC pin muxing */
	Init_DAC_PinMux();
	/* Initialize DAC input to 0 */
	dac_input = 0;
	/* Set up DAC internal timer to trigger interrupts every 0.5ms/2KHz */
	Chip_DAC_SetReqInterval(LPC_DAC, 2000);
	Chip_DAC_EnableIntTimer(LPC_DAC);
	/* Disable double buffering */
	Chip_DAC_EnableDoubleBuffer(LPC_DAC);
	/* Set trigger source as Internal Timer */
	Chip_DAC_SetTrgSrcInternal(LPC_DAC);
	/* Start DAC with zero voltage */
	Chip_DAC_UpdateValue(LPC_DAC, dac_input);
	/* Enable the Interrupt for DAC */
	NVIC_EnableIRQ(DAC_IRQ);

	while (1) {
		/* Enter low power mode until DAC interrupt */
		__WFI();
	}

	return 0;
}
