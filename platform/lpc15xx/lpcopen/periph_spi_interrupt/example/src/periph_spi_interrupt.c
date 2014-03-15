/*
 * @brief SPI bus master example in interrupt mode
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

/* Transmit and Receive Buffers */
static uint16_t xferArray[] = {0x1111, 0x2222, 0x3333, 0x4444};
static uint16_t rx_buff[sizeof(xferArray) / sizeof(xferArray[0])];
/* Interrupt error code (used as semaphore) */
static volatile int intErrCode;
/* SPI Transfer Setup */
static SPI_DATA_SETUP_T XferSetup;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* Initializes pin muxing for SPI interface - note that SystemInit() may
   already setup your pin muxing at system startup */
static void Init_SPI_PinMux(void)
{
#if (defined(BOARD_NXP_LPCXPRESSO_1549))

	/* Enable the clock to the Switch Matrix */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
	/*
	 * Initialize SPI0 pins connect
	 * SCK0: PINASSIGN3[15:8]: Select P0.0
	 * MOSI0: PINASSIGN3[23:16]: Select P0.16
	 * MISO0: PINASSIGN3[31:24] : Select P0.10
	 * SSEL0: PINASSIGN4[7:0]: Select P0.9
	 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 0, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 16, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 10, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));

	Chip_SWM_MovablePinAssign(SWM_SPI0_SCK_IO, 0);	/* P0.0 */
	Chip_SWM_MovablePinAssign(SWM_SPI0_MOSI_IO, 16);/* P0.16 */
	Chip_SWM_MovablePinAssign(SWM_SPI0_MISO_IO, 10);/* P0.10 */
	Chip_SWM_MovablePinAssign(SWM_SPI0_SSELSN_0_IO, 9);	/* P0.9 */

	/* Disable the clock to the Switch Matrix to save power */
	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);
#else
	/* Configure your own SPI pin muxing here if needed */
#warning "No SPI pin muxing defined"
#endif
}

/* Turn on LED to indicate an error */
static void errorSPI(void)
{
	Board_LED_Set(0, true);
	while (1) {}
}

/* Setup SPI handle and parameters */
static void setupSpiMaster()
{
	SPI_CFG_T spiCfg;
	SPI_DELAY_CONFIG_T spiDelayCfg;
	/* Initialize SPI Block */
	Chip_SPI_Init(LPC_SPI0);
	/* Set SPI Config register */
	spiCfg.ClkDiv = 0xFFFF;	/* Set Clock divider to maximum */
	spiCfg.Mode = SPI_MODE_MASTER;	/* Enable Master Mode */
	spiCfg.ClockMode = SPI_CLOCK_MODE0;	/* Enable Mode 0 */
	spiCfg.DataOrder = SPI_DATA_MSB_FIRST;	/* Transmit MSB first */
	/* Slave select polarity is active low */
	spiCfg.SSELPol = (SPI_CFG_SPOL0_LO | SPI_CFG_SPOL1_LO | SPI_CFG_SPOL2_LO | SPI_CFG_SPOL3_LO);
	Chip_SPI_SetConfig(LPC_SPI0, &spiCfg);
	/* Set Delay register */
	spiDelayCfg.PreDelay = 2;
	spiDelayCfg.PostDelay = 2;
	spiDelayCfg.FrameDelay = 2;
	spiDelayCfg.TransferDelay = 2;
	Chip_SPI_DelayConfig(LPC_SPI0, &spiDelayCfg);
	/* Enable Loopback mode for this example */
	Chip_SPI_EnableLoopBack(LPC_SPI0);
	/* Enable SPI0 */
	Chip_SPI_Enable(LPC_SPI0);
}

/* Master SPI transmit in interrupt mode */
static void WriteSpiMssg(uint16_t *xferPtr, uint32_t xferSize)
{
	/* Init variable used as semaphore */
	intErrCode = -1;
	/* Setup Transfer structure, this data should be retained for the entire transmission */
	XferSetup.pTx = xferArray;	/* Transmit Buffer */
	XferSetup.pRx = rx_buff;/* Receive Buffer */
	XferSetup.DataSize = sizeof(xferArray[0]) * 8;	/* Data size in bits */
	XferSetup.Length = sizeof(xferArray) / sizeof(xferArray[0]);	/* Total frame length */
	/* Assert only SSEL0 */
	XferSetup.ssel = SPI_TXCTL_ASSERT_SSEL0 | SPI_TXCTL_DEASSERT_SSEL1 | SPI_TXCTL_DEASSERT_SSEL2 |
					 SPI_TXCTL_DEASSERT_SSEL3;
	XferSetup.TxCnt = 0;
	XferSetup.RxCnt = 0;
	if (Chip_SPI_Int_RWFrames(LPC_SPI0, &XferSetup) == ERROR) {
		errorSPI();
	}
	/* Enable interrupts after initiating transmission */
	Chip_SPI_Int_Cmd(LPC_SPI0,
					 SPI_INTENSET_RXRDYEN | SPI_INTENSET_TXRDYEN | SPI_INTENSET_RXOVEN | SPI_INTENSET_TXUREN,
					 ENABLE);

	/* Sleep until transfer is complete, but allow IRQ to wake system
	   to handle SPI IRQ */
	while (intErrCode == -1) {
		__WFI();
	}
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle SPI0 interrupt by calling SPI ROM handler
 * @return	Nothing
 */
void SPI0_IRQHandler(void)
{
	uint32_t i;

	if (XferSetup.RxCnt < XferSetup.Length) {
		/* Call driver function until transmission is complete */
		if (Chip_SPI_Int_RWFrames(LPC_SPI0, &XferSetup) == ERROR) {
			errorSPI();
		}
	}
	else {
		/* Disable interrupts after transmission is complete */
		Chip_SPI_Int_Cmd(LPC_SPI0,
						 SPI_INTENSET_RXRDYEN | SPI_INTENSET_TXRDYEN | SPI_INTENSET_RXOVEN | SPI_INTENSET_TXUREN,
						 DISABLE);
		/* Verify if received data is same as transmit */
		for (i = 0; i < sizeof(xferArray) / sizeof(xferArray[0]); i++) {
			if (rx_buff[i] != xferArray[i]) {
				errorSPI();
			}
		}
		intErrCode = (int) LPC_OK;
	}
}

/**
 * @brief	Main routine for SPI example
 * @return	Function should not exit
 */
int main(void)
{
	/* Generic Initialization */
	SystemCoreClockUpdate();
	Board_Init();

	/* Clear activity LED */
	Board_LED_Set(0, false);

	/* Setup SPI pin muxing */
	Init_SPI_PinMux();

	/* Allocate SPI handle, setup rate, and initialize clocking */
	setupSpiMaster();

	/* Enable SPI0 interrupt */
	NVIC_EnableIRQ(SPI0_IRQn);

	/* Loop forever */
	while (1) {
		/* Write simple message over SPI */
		WriteSpiMssg(xferArray, sizeof(xferArray) / sizeof(xferArray[0]));

		/* Toggle LED to show activity. */
		Board_LED_Toggle(0);
	}

	/* Code never reaches here. Only used to satisfy standard main() */
	return 0;
}
