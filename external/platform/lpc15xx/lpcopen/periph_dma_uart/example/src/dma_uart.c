/*
 * @brief UART DMA example
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
#include "string.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

#define USE_INTEGER_CLOCK

/* DMA send string arrays. DMA buffers must remain in memory during the DMA
   transfer. */
#define DMASENDSTRCNT   6
static char dmaSendStr[DMASENDSTRCNT][32];

/* Number of UART TX descriptors used for DMA */
#define UARTTXDESC 8

/* Next available UART TX DMA descriptor and use counter */
static volatile int nextTXDesc, countTXDescUsed;

/* Number of UART RX descriptors used for DMA */
#define UARTRXDESC 4

/* Maximum size of each UART RX receive buffer */
#define UARTRXBUFFSIZE  8

/* UART RX receive buffers */
static uint8_t dmaRXBuffs[UARTRXDESC][UARTRXBUFFSIZE];

/* UART receive buffer that is available and availa flag */
static volatile int uartRXBuff;
static volatile bool uartRxAvail;

/* DMA descriptors must be aligned to 16 bytes */
#if defined(__CC_ARM)
__align(16) static DMA_CHDESC_T dmaTXDesc[UARTTXDESC];
__align(16) static DMA_CHDESC_T dmaRXDesc[UARTRXDESC];
#endif /* defined (__CC_ARM) */

/* IAR support */
#if defined(__ICCARM__)
#pragma data_alignment=16
static DMA_CHDESC_T dmaTXDesc[UARTTXDESC];
#pragma data_alignment=16
static DMA_CHDESC_T dmaRXDesc[UARTRXDESC];
#endif /* defined (__ICCARM__) */

#if defined( __GNUC__ )
static DMA_CHDESC_T dmaTXDesc[UARTTXDESC] __attribute__ ((aligned(16)));
static DMA_CHDESC_T dmaRXDesc[UARTRXDESC] __attribute__ ((aligned(16)));
#endif /* defined (__GNUC__) */

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

static void Init_UART_PinMux(void)
{
#if defined(BOARD_NXP_LPCXPRESSO_1549)
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 13, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));

	/* UART signal muxing via SWM */
	Chip_SWM_MovablePortPinAssign(SWM_UART0_RXD_I, 0, 13);
	Chip_SWM_MovablePortPinAssign(SWM_UART0_TXD_O, 0, 18);

#else
#error "No UART setup defined"
#endif
}

/* Setup DMA UART TX support, but do not queue descriptors yet */
static void dmaTXSetup(void)
{
	/* Setup UART 0 TX channel for the following configuration:
	   - Peripheral DMA request (UART 0 TX channel)
	   - Single transfer
	   - Low channel priority */
	Chip_DMA_EnableChannel(LPC_DMA, DMAREQ_USART0_TX);
	Chip_DMA_EnableIntChannel(LPC_DMA, DMAREQ_USART0_TX);
	Chip_DMA_SetupChannelConfig(LPC_DMA, DMAREQ_USART0_TX,
								(DMA_CFG_PERIPHREQEN | DMA_CFG_TRIGBURST_SNGL | DMA_CFG_CHPRIORITY(3)));

	countTXDescUsed = 0;
}

/* Setup DMA UART RX support, but do not queue descriptors yet */
static void dmaRXSetup(void)
{
	/* Setup UART 0 RX channel for the following configuration:
	   - Peripheral DMA request (UART 0 RX channel)
	   - Single transfer
	   - Low channel priority */
	Chip_DMA_EnableChannel(LPC_DMA, DMAREQ_USART0_RX);
	Chip_DMA_EnableIntChannel(LPC_DMA, DMAREQ_USART0_RX);
	Chip_DMA_SetupChannelConfig(LPC_DMA, DMAREQ_USART0_RX,
								(DMA_CFG_PERIPHREQEN | DMA_CFG_TRIGBURST_SNGL | DMA_CFG_CHPRIORITY(3)));
}

/* Send data via the UART */
static bool dmaTXSend(uint8_t *data, int bytes)
{
	/* Disable the DMA IRQ to prevent race conditions with shared data */
	NVIC_DisableIRQ(DMA_IRQn);

	/* This is a limited example, limit descriptor and byte count */
	if ((countTXDescUsed >= UARTTXDESC) || (bytes > 1024)) {
		/* Re-enable the DMA IRQ */
		NVIC_EnableIRQ(DMA_IRQn);

		/* All DMA descriptors are used, so just exit */
		return false;
	}
	else if (countTXDescUsed == 0) {
		/* No descriptors are currently used, so take the first one */
		nextTXDesc = 0;
	}

	/* Create a descriptor for the data */
	dmaTXDesc[countTXDescUsed].source = DMA_ADDR(data + bytes - 1);	/* Last address here */
	dmaTXDesc[countTXDescUsed].dest = DMA_ADDR(&LPC_USART0->TXDATA);	/* Byte aligned */

	/* If there are multiple buffers with non-contiguous addresses, they can be chained
	   together here (it is recommended to only use the DMA_XFERCFG_SETINTA on the
	   last chained descriptor). If another TX buffer needs to be sent, the DMA
	   IRQ handler will re-queue and send the buffer there without using chaining. */
	dmaTXDesc[countTXDescUsed].next = DMA_ADDR(0);

	/* Setup transfer configuration */
	dmaTXDesc[countTXDescUsed].xfercfg = DMA_XFERCFG_CFGVALID | DMA_XFERCFG_SETINTA |
										 DMA_XFERCFG_SWTRIG | DMA_XFERCFG_WIDTH_8 | DMA_XFERCFG_SRCINC_1 |
										 DMA_XFERCFG_DSTINC_0 | DMA_XFERCFG_XFERCOUNT(bytes);

	/* If a transfer is currently in progress, then stop here and let the DMA
	   handler re-queue the next transfer. Otherwise, start the transfer here. */
	if (countTXDescUsed == 0) {
		/* Setup transfer descriptor and validate it */
		Chip_DMA_SetupTranChannel(LPC_DMA, DMAREQ_USART0_TX, &dmaTXDesc[countTXDescUsed]);

		/* Setup data transfer */
		Chip_DMA_SetupChannelTransfer(LPC_DMA, DMAREQ_USART0_TX,
									  dmaTXDesc[countTXDescUsed].xfercfg);

		Chip_DMA_SetValidChannel(LPC_DMA, DMAREQ_USART0_TX);
	}

	/* Update used descriptor count */
	countTXDescUsed++;

	/* Re-enable the DMA IRQ */
	NVIC_EnableIRQ(DMA_IRQn);

	return true;
}

/* Queue up DMA descriptors and buffers for UART RX */
static void dmaRXQueue(void)
{
	int i;

	/* Linked list of descriptors that map to the 3 receive buffers */
	for (i = 0; i < UARTRXDESC; i++) {
		/* Setup next descriptor */
		if (i == (UARTRXDESC - 1)) {
			/* Wrap descriptors */
			dmaRXDesc[i].next = DMA_ADDR(&dmaRXDesc[0]);
		}
		else {
			dmaRXDesc[i].next = DMA_ADDR(&dmaRXDesc[i + 1]);
		}

		/* Create a descriptor for the data */
		dmaRXDesc[i].source = DMA_ADDR(&LPC_USART0->RXDATA) + 0;	/* Byte aligned */
		dmaRXDesc[i].dest = DMA_ADDR(&dmaRXBuffs[i][0] + UARTRXBUFFSIZE - 1);

		/* Setup transfer configuration */
		dmaRXDesc[i].xfercfg = DMA_XFERCFG_CFGVALID | DMA_XFERCFG_SETINTA |
							   DMA_XFERCFG_WIDTH_8 | DMA_XFERCFG_SRCINC_0 |
							   DMA_XFERCFG_DSTINC_1 | DMA_XFERCFG_RELOAD |
							   DMA_XFERCFG_XFERCOUNT(UARTRXBUFFSIZE);
	}

	/* Setup transfer descriptor and validate it */
	Chip_DMA_SetupTranChannel(LPC_DMA, DMAREQ_USART0_RX, &dmaRXDesc[0]);

	/* Setup data transfer */
	Chip_DMA_SetupChannelTransfer(LPC_DMA, DMAREQ_USART0_RX,
								  dmaRXDesc[0].xfercfg);
	Chip_DMA_SetValidChannel(LPC_DMA, DMAREQ_USART0_RX);
	Chip_DMA_SWTriggerChannel(LPC_DMA, DMAREQ_USART0_RX);
}

/* Check and return UART RX data if it exists */
static int checkRxData(uint8_t *buff)
{
	int bytesRec = 0;

	if (uartRxAvail) {
		uartRxAvail = false;

		memcpy(buff, dmaRXBuffs[uartRXBuff], UARTRXBUFFSIZE);
		uartRXBuff++;
		if (uartRXBuff >= UARTRXDESC) {
			uartRXBuff = 0;
		}
		bytesRec = UARTRXBUFFSIZE;
	}

	return bytesRec;
}

/* Clear an error on a DMA channel */
static void dmaClearChannel(DMA_CHID_T ch) {
	Chip_DMA_DisableChannel(LPC_DMA, ch);
	while ((Chip_DMA_GetBusyChannels(LPC_DMA) & (1 << ch)) != 0) {}

	Chip_DMA_AbortChannel(LPC_DMA, ch);
	Chip_DMA_ClearErrorIntChannel(LPC_DMA, ch);
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
	uint32_t errors, pending;

	/* Get DMA error and interrupt channels */
	errors = Chip_DMA_GetErrorIntChannels(LPC_DMA);
	pending = Chip_DMA_GetActiveIntAChannels(LPC_DMA);

	/* Check DMA interrupts of UART 0 TX channel */
	if ((errors | pending) & (1 << DMAREQ_USART0_TX)) {
		/* Clear DMA interrupt for the channel */
		Chip_DMA_ClearActiveIntAChannel(LPC_DMA, DMAREQ_USART0_TX);

		/* Handle errors if needed */
		if (errors & (1 << DMAREQ_USART0_TX)) {
			/* DMA error, channel needs to be reset */
			dmaClearChannel(DMAREQ_USART0_TX);
			dmaTXSetup();
		}
		else {
			/* Descriptor is consumed */
			countTXDescUsed--;
		}

		/* Is another DMA descriptor waiting that was not chained? */
		if (countTXDescUsed > 0) {
			nextTXDesc++;

			/* Setup transfer descriptor and validate it */
			Chip_DMA_SetupTranChannel(LPC_DMA, DMAREQ_USART0_TX, &dmaTXDesc[nextTXDesc]);

			/* Setup data transfer */
			Chip_DMA_SetupChannelTransfer(LPC_DMA, DMAREQ_USART0_TX,
										  dmaTXDesc[nextTXDesc].xfercfg);

			Chip_DMA_SetValidChannel(LPC_DMA, DMAREQ_USART0_TX);
		}
	}

	/* Check DMA interrupts of UART 0 RX channel */
	if ((errors | pending) & (1 << DMAREQ_USART0_RX)) {
		/* Clear DMA interrupt for the channel */
		Chip_DMA_ClearActiveIntAChannel(LPC_DMA, DMAREQ_USART0_RX);

		/* Handle errors if needed */
		if (errors & (1 << DMAREQ_USART0_RX)) {
			/* DMA error, channel needs to be reset */
			dmaClearChannel(DMAREQ_USART0_RX);
			dmaRXSetup();
			dmaRXQueue();
		}
		else {
			uartRxAvail = true;
		}
	}
}

/**
 * @brief	Main UART/DMA program body
 * @return	Does not exit
 */
int main(void)
{
	int bytes, idx;
	uint8_t buff[UARTRXBUFFSIZE];

	SystemCoreClockUpdate();
	Board_Init();
	Init_UART_PinMux();
	Board_LED_Set(0, false);

#if defined(USE_INTEGER_CLOCK)
	/* Use main clock rate as base for UART baud rate divider */
	Chip_Clock_SetUARTBaseClockRate(Chip_Clock_GetMainClockRate(), false);

#else
	/* Use 128x expected UART baud rate for fractional baud mode. */
	Chip_Clock_SetUARTBaseClockRate((115200 * 128), true);
#endif
	/* Setup UART */
	Chip_UART_Init(LPC_USART0);
	Chip_UART_ConfigData(LPC_USART0, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1);
	Chip_UART_SetBaud(LPC_USART0, 115200);
	/* Optional for low clock rates only: Chip_UART_SetBaudWithRTC32K(LPC_USART, 300); */
	Chip_UART_Enable(LPC_USART0);
	Chip_UART_TXEnable(LPC_USART0);

	/* DMA initialization - enable DMA clocking and reset DMA if needed */
	Chip_DMA_Init(LPC_DMA);

	/* Enable DMA controller and use driver provided DMA table for current descriptors */
	Chip_DMA_Enable(LPC_DMA);
	Chip_DMA_SetSRAMBase(LPC_DMA, DMA_ADDR(Chip_DMA_Table));

	/* Setup UART 0 TX DMA support */
	dmaTXSetup();

	/* Setup UART 0 RX DMA support */
	dmaRXSetup();

	/* Enable the DMA IRQ */
	NVIC_EnableIRQ(DMA_IRQn);

	/* Enqueue a bunch of strings in DMA transmit descriptors and start
	   transmit. In this use of DMA, the descriptors aren't chained, so
	     the DMA restarts the next queued descriptor in the DMA interrupt
	     handler. */
	for (idx = 0; idx < DMASENDSTRCNT; idx++) {
		sprintf(dmaSendStr[idx], "DMA send string (unlinked) #%d\r\n", idx);
		dmaTXSend((uint8_t *) dmaSendStr[idx], strlen(dmaSendStr[idx]));
	}

	/* Wait for UART TX DMA channel to go inactive */
	while (1) {
		__WFI();
		if (Chip_DMA_GetActiveChannels(LPC_DMA) & (1 << DMAREQ_USART0_TX)) {
			break;
		}
	}

	/* Receive buffers are queued. The DMA interrupt will only trigger on a
	   full DMA buffer receive, so if the UART is idle, but the DMA is only
	   partially complete, the DMA interrupt won't fire. For UART data
	   receive where data is not continuous, a timeout method will be
	   required to flush the DMA when the DMA has pending data and no
	   data has been received on the UART in a specified timeout */
	dmaRXQueue();

	/* Get RX data via DMA and send it out on TX via DMA */
	while (1) {
		/* Sleep until something happens */
		__WFI();

		/* Did any data come in? */
		bytes = checkRxData(buff);
		if (bytes > 0) {
			/* RX data received, send it via TX DMA */
			dmaTXSend(buff, bytes);
		}
	}

	return 1;
}
