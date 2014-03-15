/*
 * @brief DMA memory to memory transfer example
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

#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "stopwatch.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Number of memory operations to use for application */
#define NUMBER_TRANSFER_OPS     50000

/* Size of the source and destination buffers in 32-bit words.
   Allowable values  = 128, 256, 512, or 1024 */
#define SIZE_BUFFERS            (512)

/* Source and destination buffers */
uint32_t src[SIZE_BUFFERS], dst[SIZE_BUFFERS];

/* DMA completion flag */
static volatile bool dmaDone;

/* Enable this define to test the data after memory transfers */
/* #define XFERTEST */

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Populate source array with changing data, clear destination data */
static void modifyData(uint32_t *src, uint32_t *dst, int words)
{
#if defined(XFERTEST)
	int i;

	/* Put some data in the source buffer for test */
	for (i = 0; i < words; i++) {
		src[i] = src[i] + i + 1;
		dst[i] = 0;
	}
#endif
}

/* Simple data verification */
static void verifyData(uint32_t *src, uint32_t *dst, int words)
{
#if defined(XFERTEST)
	int i, errIdx = -1;

	for (i = 0; ((i < words) && (errIdx == -1)); i++) {
		if (src[i] != dst[i]) {
			errIdx = i;
		}
	}

	if (errIdx != -1) {
		Board_LED_Set(1, true);	/* LED 1 indicates a compare error */
		DEBUGOUT("Compare error on index %d\r\n", errIdx);
	}
#endif
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	DMA Interrupt Handler
 * @return	None
 */
void DMA_IRQHandler(void)
{
	/* Error interrupt on channel 0? */
	if ((Chip_DMA_GetIntStatus(LPC_DMA) & DMA_INTSTAT_ACTIVEERRINT) != 0) {
		/* This shouldn't happen for this simple DMA example, so set the LED
		   to indicate an error occurred. This is the correct method to clear
		   an abort. */
		Chip_DMA_DisableChannel(LPC_DMA, DMA_CH0);
		while ((Chip_DMA_GetBusyChannels(LPC_DMA) & (1 << DMA_CH0)) != 0) {}
		Chip_DMA_AbortChannel(LPC_DMA, DMA_CH0);
		Chip_DMA_ClearErrorIntChannel(LPC_DMA, DMA_CH0);
		Chip_DMA_EnableChannel(LPC_DMA, DMA_CH0);
		Board_LED_Set(0, true);
	}

	/* Clear DMA interrupt for the channel */
	Chip_DMA_ClearActiveIntAChannel(LPC_DMA, DMA_CH0);

	dmaDone = true;
}

/**
 * @brief	Main program body
 * @return	int
 */
int main(void)
{
	DMA_CHDESC_T dmaDesc;
	int i;
	uint32_t startTime, ticks[3];

	/* Setup SystemCoreClock and any needed board code */
	SystemCoreClockUpdate();
	Board_Init();

	/* Initialize stopwatch */
	StopWatch_Init();

	DEBUGOUT("Test transfer size is %d blocks @ %d bytes each\r\n",
			 NUMBER_TRANSFER_OPS, (SIZE_BUFFERS * 4));
	DEBUGOUT("Total test size = %d bytes\r\n",
			 (NUMBER_TRANSFER_OPS * SIZE_BUFFERS * 4));

	/* Reset timer and perform a bunch memory to memory moves with memmove */
	DEBUGOUT("Starting memmove test\r\n");
	startTime = StopWatch_Start();
	for (i = 0; i < NUMBER_TRANSFER_OPS; i++) {
		modifyData(src, dst, SIZE_BUFFERS);
		memmove(dst, src, sizeof(src));
		verifyData(src, dst, SIZE_BUFFERS);
	}
	ticks[0] = StopWatch_Elapsed(startTime);

	/* Reset timer and perform a bunch memory to memory moves with memcpy */
	DEBUGOUT("Starting memcpy test\r\n");
	startTime = StopWatch_Start();
	for (i = 0; i < NUMBER_TRANSFER_OPS; i++) {
		modifyData(src, dst, SIZE_BUFFERS);
		memcpy(dst, src, sizeof(src));
		verifyData(src, dst, SIZE_BUFFERS);
	}
	ticks[1] = StopWatch_Elapsed(startTime);

	/* DMA initialization - enable DMA clocking and reset DMA if needed */
	Chip_DMA_Init(LPC_DMA);

	/* Enable DMA controller and use driver provided DMA table for current descriptors */
	Chip_DMA_Enable(LPC_DMA);
	Chip_DMA_SetSRAMBase(LPC_DMA, DMA_ADDR(Chip_DMA_Table));

	/* Setup channel 0 for the following configuration:
	   - High channel priority
	   - Interrupt A fires on descriptor completion */
	Chip_DMA_EnableChannel(LPC_DMA, DMA_CH0);
	Chip_DMA_EnableIntChannel(LPC_DMA, DMA_CH0);
	Chip_DMA_SetupChannelConfig(LPC_DMA, DMA_CH0,
								(DMA_CFG_HWTRIGEN | DMA_CFG_TRIGTYPE_EDGE | DMA_CFG_TRIGPOL_HIGH |
								 DMA_CFG_BURSTPOWER_128 | DMA_CFG_CHPRIORITY(0)));

	/* DMA descriptor for memory to memory operation - note that addresses must
	   be the END address for src and destination, not the starting address.
	     DMA operations moves from end to start. */
	dmaDesc.source = DMA_ADDR(&src[SIZE_BUFFERS - 1]) + 3;
	dmaDesc.dest = DMA_ADDR(&dst[SIZE_BUFFERS - 1]) + 3;
	dmaDesc.next = DMA_ADDR(0);

	/* Enable DMA interrupt */
	NVIC_EnableIRQ(DMA_IRQn);

	/* Reset timer and perform a bunch memory to memory moves with DMA */
	DEBUGOUT("Starting DMA test\r\n");
	startTime = StopWatch_Start();
	for (i = 0; i < NUMBER_TRANSFER_OPS; i++) {
		dmaDone = false;

		modifyData(src, dst, SIZE_BUFFERS);

		/* Setup transfer descriptor and validate it */
		Chip_DMA_SetupTranChannel(LPC_DMA, DMA_CH0, &dmaDesc);
		Chip_DMA_SetValidChannel(LPC_DMA, DMA_CH0);

		/* Setup data transfer and software trigger in same call */
		Chip_DMA_SetupChannelTransfer(LPC_DMA, DMA_CH0,
									  (DMA_XFERCFG_CFGVALID | DMA_XFERCFG_SETINTA | DMA_XFERCFG_SWTRIG |
									   DMA_XFERCFG_WIDTH_32 | DMA_XFERCFG_SRCINC_1 | DMA_XFERCFG_DSTINC_1 |
									   DMA_XFERCFG_XFERCOUNT(SIZE_BUFFERS)));

		/* Wait for DMA completion */
		while (dmaDone == false) {
			Chip_PMU_SleepState(LPC_PMU);
		}

		verifyData(src, dst, SIZE_BUFFERS);
	}
	ticks[2] = StopWatch_Elapsed(startTime);
	DEBUGOUT("Transfer time with memmove (mS) = %d\r\n", StopWatch_TicksToMs(ticks[0]));
	DEBUGOUT("Transfer time with memcpy (mS) = %d\r\n", StopWatch_TicksToMs(ticks[1]));
	DEBUGOUT("Transfer time with DMA (mS) = %d\r\n", StopWatch_TicksToMs(ticks[2]));

	return 0;
}
