/*
 * @brief LPC11u6x ADC example
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
#include <stdio.h>

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

static volatile bool tempSequenceComplete;
static volatile int tempSampleIdx;

/* Temperature sample ADC read buffer - only index 9 is used for the valid
   temperature sample. */
#define TEMPSAMPLES 10
static uint32_t temp[TEMPSAMPLES];

#define TICKRATE_HZ (100)	/* 100 ticks per second */

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* This starts an ADC temperature sampling sequence using the recommended
   burst method. It bursts 10 temperature sensor samples via the ADC and
   then stops the ADC sequencer. The single sequence (non-burst) mode can
   also be used for ADC temperature sensor read. */
static void tempStartCycle(void)
{
	tempSampleIdx = 0;
	tempSequenceComplete = false;

	/* Enable burst mode */
	Chip_ADC_StartBurstSequencer(LPC_ADC0, ADC_SEQA_IDX);
}

/* Used to indicate when a temperature cycle is complete and the sample
   is ready */
static bool tempCycleComplete(void)
{
	return tempSequenceComplete;
}

/* Returns the last temperature sample only. Only valid if tempCycleComplete()
   returns true */
static uint32_t tempGetSample(void)
{
	tempSequenceComplete = false;
	return temp[TEMPSAMPLES - 1];
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle interrupt from SysTick timer
 * @return	Nothing
 */
void SysTick_Handler(void)
{
	static uint32_t count;

	/* Every 1/2 second */
	count++;
	if (count >= (TICKRATE_HZ / 1)) {
		count = 0;

		/* Restart temperature cycle */
		tempStartCycle();
	}
}

/**
 * @brief	Handle interrupt from ADC sequencer A
 * @return	Nothing
 */
void ADC0A_IRQHandler(void)
{
	uint32_t pending;

	/* Get pending interrupts */
	pending = Chip_ADC_GetFlags(LPC_ADC0);

	/* Sequence A completion interrupt */
	if (pending & ADC_FLAGS_SEQA_INT_MASK) {
		if (tempSampleIdx < TEMPSAMPLES) {
			/* Save sample */
			temp[tempSampleIdx] = Chip_ADC_GetDataReg(LPC_ADC0, 0);
			tempSampleIdx++;

			if (tempSampleIdx >= TEMPSAMPLES) {
				Chip_ADC_StopBurstSequencer(LPC_ADC0, ADC_SEQA_IDX);
				tempSequenceComplete = true;
			}
		}
	}

	/* Clear any pending interrupts */
	Chip_ADC_ClearFlags(LPC_ADC0, pending);
}

/**
 * @brief	main routine for ADC example
 * @return	Function should not exit
 */
int main(void)
{
	uint32_t rawSample;

	SystemCoreClockUpdate();
	Board_Init();
	DEBUGSTR("Temperature sensor demo\r\n");

	/* Setup ADC for 12-bit mode and normal power */
	Chip_ADC_Init(LPC_ADC0, 0);

	/* Setup ADC clock rate */
	Chip_ADC_SetClockRate(LPC_ADC0, 250000);

	/* For ADC0, select temperature sensor for channel 0 on ADC0 */
	Chip_ADC_SetADC0Input(LPC_ADC0, ADC_INSEL_TS);

	/* Setup a sequencer to do the following:
	   Perform ADC conversion of ADC channels 0 with EOS interrupt */
	Chip_ADC_SetupSequencer(LPC_ADC0, ADC_SEQA_IDX, (ADC_SEQ_CTRL_CHANSEL(0) |
													 ADC_SEQ_CTRL_MODE_EOS));

	/* Power up the internal temperature sensor - this also selects the
	    temperature sensor as the input for the ADC0 input */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_TS_PD);

	/* Use higher voltage trim */
	Chip_ADC_SetTrim(LPC_ADC0, ADC_TRIM_VRANGE_HIGHV);

	/* Need to do a calibration after initialization and trim */
	Chip_ADC_StartCalibration(LPC_ADC0);
	while (!(Chip_ADC_IsCalibrationDone(LPC_ADC0))) {}

	/* Clear all pending interrupts */
	Chip_ADC_ClearFlags(LPC_ADC0, Chip_ADC_GetFlags(LPC_ADC0));

	/* Enable ADC sequence A completion interrupt */
	Chip_ADC_EnableInt(LPC_ADC0, ADC_INTEN_SEQA_ENABLE);

	/* Enable ADC NVIC interrupt */
	NVIC_EnableIRQ(ADC0_SEQA_IRQn);

	/* Enable sequencer */
	Chip_ADC_EnableSequencer(LPC_ADC0, ADC_SEQA_IDX);

	/* This example uses the periodic sysTick to manually trigger
	   the ADC burst cycle */
	SysTick_Config(SystemCoreClock / TICKRATE_HZ);

	/* Endless loop */
	while (1) {
		/* Sleep until something happens */
		__WFI();

		/* Is a conversion sequence complete? */
		if (tempCycleComplete()) {
			rawSample = tempGetSample();
			if ((rawSample & ADC_SEQ_GDAT_DATAVALID) != 0) {
				DEBUGOUT("Sampled temp value = 0x%04x (first = 0x%04x)\r", ADC_DR_RESULT(rawSample),
						 ADC_DR_RESULT(temp[0]));
			}
			else {
				DEBUGSTR("\r\nInvalid sample read\r\n");
			}
		}
	}

	/* Should not run to here */
	return 0;
}
