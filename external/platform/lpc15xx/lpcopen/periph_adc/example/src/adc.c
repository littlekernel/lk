/*
 * @brief LPC15xx ADC example
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

static volatile int ticks;
static bool sequence0Complete, sequence1Complete, threshold1Crossed;

#define TICKRATE_HZ (100)	/* 100 ticks per second */

#if defined(BOARD_NXP_LPCXPRESSO_1549)
/* ADC is connected to the pot on LPCXPresso base boards */
#define BOARD_ADC_CH 1

#else
#warning "Using ADC channel 8 for this example, please select for your board"
#define BOARD_ADC_CH 8
#endif

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

void showValudeADC(LPC_ADC_T *pADC)
{
	int index, j;
	uint32_t rawSample;

	if (pADC == LPC_ADC0) {
		index = 0;
	}
	else {
		index = 1;
	}

	/* Get raw sample data for channels 0-11 */
	for (j = 0; j < 12; j++) {
		rawSample = Chip_ADC_GetDataReg(pADC, j);

		/* Show some ADC data */
		if ((rawSample & (ADC_DR_OVERRUN | ADC_SEQ_GDAT_DATAVALID)) != 0) {
			DEBUGOUT("ADC%d_%d: Sample value = 0x%x (Data sample %d)\r\n", index, j,
					 ADC_DR_RESULT(rawSample), j);

			/* Threshold events are only on ADC1 */
			if (index == 1) {
				DEBUGOUT("ADC%d_%d: Threshold range = 0x%x\r\n", index, j,
						 ADC_DR_THCMPRANGE(rawSample));
				DEBUGOUT("ADC%d_%d: Threshold cross = 0x%x\r\n", index, j,
						 ADC_DR_THCMPCROSS(rawSample));
			}
		}
	}
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
	if (count >= (TICKRATE_HZ / 2)) {
		count = 0;

		/* Manual start for ADC1 conversion sequence A */
		Chip_ADC_StartSequencer(LPC_ADC0, ADC_SEQA_IDX);
		Chip_ADC_StartSequencer(LPC_ADC1, ADC_SEQA_IDX);
	}
}

/**
 * @brief	Handle interrupt from ADC0 sequencer A
 * @return	Nothing
 */
void ADC0A_IRQHandler(void)
{
	uint32_t pending;

	/* Get pending interrupts */
	pending = Chip_ADC_GetFlags(LPC_ADC0);

	/* Sequence A completion interrupt */
	if (pending & ADC_FLAGS_SEQA_INT_MASK) {
		sequence0Complete = true;
	}

	/* Clear any pending interrupts */
	Chip_ADC_ClearFlags(LPC_ADC0, pending);
}

/**
 * @brief	Handle interrupt from ADC1 sequencer A
 * @return	Nothing
 */
void ADC1A_IRQHandler(void)
{
	uint32_t pending;

	/* Get pending interrupts */
	pending = Chip_ADC_GetFlags(LPC_ADC1);

	/* Sequence A completion interrupt */
	if (pending & ADC_FLAGS_SEQA_INT_MASK) {
		sequence1Complete = true;
	}

	/* Clear Sequence A completion interrupt */
	Chip_ADC_ClearFlags(LPC_ADC1, ADC_FLAGS_SEQA_INT_MASK);
}

/**
 * @brief	Handle threshold interrupt from ADC1
 * @return	Nothing
 */
void ADC1_THCMP_IRQHandler(void)
{
	uint32_t pending;

	/* Get pending interrupts */
	pending = Chip_ADC_GetFlags(LPC_ADC1);

	/* Threshold crossing interrupt on ADC input channel */
	if (pending & ADC_FLAGS_THCMP_MASK(BOARD_ADC_CH)) {
		threshold1Crossed = true;
	}

	/* Clear threshold interrupt */
	Chip_ADC_ClearFlags(LPC_ADC1, ADC_FLAGS_THCMP_MASK(BOARD_ADC_CH));
}

/**
 * @brief	main routine for ADC example
 * @return	Function should not exit
 */
int main(void)
{
	SystemCoreClockUpdate();
	Board_Init();
	DEBUGSTR("ADC sequencer demo\r\n");

	/* Setup ADC for 12-bit mode and normal power */
	Chip_ADC_Init(LPC_ADC0, 0);
	Chip_ADC_Init(LPC_ADC1, 0);

	/* Setup for maximum ADC clock rate */
	Chip_ADC_SetClockRate(LPC_ADC0, ADC_MAX_SAMPLE_RATE);
	Chip_ADC_SetClockRate(LPC_ADC1, ADC_MAX_SAMPLE_RATE);

	/* For ADC0, seqeucner A will be used without threshold events.
	   It will be triggered manually by the sysTick interrupt and
	   only monitor the internal temperature sensor. */
	Chip_ADC_SetupSequencer(LPC_ADC0, ADC_SEQA_IDX, (ADC_SEQ_CTRL_CHANSEL(0) |
													 ADC_SEQ_CTRL_MODE_EOS));

	/* Power up the internal temperature sensor */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_TS_PD);

	/* For ADC0, select temperature sensor for channel 0 on ADC0 */
	Chip_ADC_SetADC0Input(LPC_ADC0, ADC_INSEL_TS);

#if defined(BOARD_NXP_LPCXPRESSO_1549)
	/* Use higher voltage trim for both ADCs */
	Chip_ADC_SetTrim(LPC_ADC0, ADC_TRIM_VRANGE_HIGHV);
	Chip_ADC_SetTrim(LPC_ADC1, ADC_TRIM_VRANGE_HIGHV);

	/* For ADC1, seqeucner A will be used with threshold events.
	   It will be triggered manually by the sysTick interrupt and
	   only monitors the ADC1 input. */
	Chip_ADC_SetupSequencer(LPC_ADC1, ADC_SEQA_IDX,
							(ADC_SEQ_CTRL_CHANSEL(BOARD_ADC_CH) | ADC_SEQ_CTRL_MODE_EOS));

	/* Disables pullups/pulldowns and disable digital mode */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));

	/* Assign ADC1_1 to PIO0_9 via SWM (fixed pin) */
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_1);

#else
#warning "No ADC setup for this example"
#endif

	/* Need to do a calibration after initialization and trim */
	Chip_ADC_StartCalibration(LPC_ADC0);
	Chip_ADC_StartCalibration(LPC_ADC1);
	while (!(Chip_ADC_IsCalibrationDone(LPC_ADC0))) {}
	while (!(Chip_ADC_IsCalibrationDone(LPC_ADC1))) {}

	/* Setup threshold 0 low and high values to about 25% and 75% of max for
	     ADC1 only */
	Chip_ADC_SetThrLowValue(LPC_ADC1, 0, ((1 * 0xFFF) / 4));
	Chip_ADC_SetThrHighValue(LPC_ADC1, 0, ((3 * 0xFFF) / 4));

	/* Clear all pending interrupts */
	Chip_ADC_ClearFlags(LPC_ADC0, Chip_ADC_GetFlags(LPC_ADC0));
	Chip_ADC_ClearFlags(LPC_ADC1, Chip_ADC_GetFlags(LPC_ADC1));

	/* Enable sequence A completion interrupts for ADC0 */
	Chip_ADC_EnableInt(LPC_ADC0, ADC_INTEN_SEQA_ENABLE);

	/* Enable sequence A completion and threshold crossing interrupts for ADC1_1 */
	Chip_ADC_EnableInt(LPC_ADC1, ADC_INTEN_SEQA_ENABLE |
					   ADC_INTEN_CMP_ENABLE(ADC_INTEN_CMP_CROSSTH, BOARD_ADC_CH));

	/* Use threshold 0 for ADC channel and enable threshold interrupt mode for
	   channel as crossing */
	Chip_ADC_SelectTH0Channels(LPC_ADC1, ADC_THRSEL_CHAN_SEL_THR1(BOARD_ADC_CH));
	Chip_ADC_SetThresholdInt(LPC_ADC1, BOARD_ADC_CH, ADC_INTEN_THCMP_CROSSING);

	/* Enable related ADC NVIC interrupts */
	NVIC_EnableIRQ(ADC0_SEQA_IRQn);
	NVIC_EnableIRQ(ADC1_SEQA_IRQn);
	NVIC_EnableIRQ(ADC1_THCMP);

	/* Enable sequencers */
	Chip_ADC_EnableSequencer(LPC_ADC0, ADC_SEQA_IDX);
	Chip_ADC_EnableSequencer(LPC_ADC1, ADC_SEQA_IDX);

	/* This example uses the periodic sysTick to manually trigger the ADC,
	   but a periodic timer can be used in a match configuration to start
	   an ADC sequence without software intervention. */
	SysTick_Config(Chip_Clock_GetSysTickClockRate() / TICKRATE_HZ);

	/* Endless loop */
	while (1) {
		/* Sleep until something happens */
		__WFI();

		if (threshold1Crossed) {
			threshold1Crossed = false;
			DEBUGSTR("********ADC1 threshold event********\r\n");
		}

		/* Is a conversion sequence complete? */
		if (sequence0Complete) {
			showValudeADC(LPC_ADC0);
		}
		if (sequence1Complete) {
			showValudeADC(LPC_ADC1);
		}
	}

	/* Should not run to here */
	return 0;
}
