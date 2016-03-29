/*
 * @brief UART DMA ROM example
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
#define UARTRXDESC 3

/* Maximum size of each UART RX receive buffer */
#define UARTRXBUFFSIZE  8

/* UART RX receive buffers */
static volatile uint8_t dmaRXBuffs[UARTRXDESC][UARTRXBUFFSIZE];

/* UART receive buffer that is available and available flag */
static volatile int uartRXBuff;
static volatile bool uartRxAvail;

/* RAM Block for the DMA ROM Handle which contains the DMA
     Channel Descriptor Map which should be aligned to 512 bytes */
#define RAMBLOCK 400
#if defined(__CC_ARM)
/* Keil alignment to 512 bytes */
__align(512) static uint8_t dma_ram_block[RAMBLOCK];
#endif /* defined (__CC_ARM) */

/* IAR support */
#if defined(__ICCARM__)
/* IAR EWARM alignment to 512 bytes */
#pragma data_alignment=512
static uint8_t dma_ram_block[RAMBLOCK];
#endif /* defined (__ICCARM__) */

#if defined( __GNUC__ )
/* GNU alignment to 512 bytes */
static uint8_t dma_ram_block[RAMBLOCK]; __attribute__ ((aligned(512)));
#endif /* defined (__GNUC__) */

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

/* DMA ROM structures */
static const DMAD_API_T *pDMAApi;
static uint32_t             size_in_bytes;
static DMA_HANDLE_T     dma_handle;	/* handle to DMA */
static DMA_CHANNEL_T    dma_ch_cfg;	/* Channel Configuration for DMA ROM API */
static DMA_TASK_T           dma_task_cfg;	/* Transfer Configuration for DMA ROM API */
static volatile ErrorCode_t     err_code;

static void dmaRXQueue(void);

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

/* Transmit Callback for DMA Tx Channel */
static void dmaTXDone(uint32_t err_code, uint32_t int_type)
{
	if ((ErrorCode_t) err_code == LPC_OK) {
		/* No error, so decrement descriptor count */
		countTXDescUsed--;
	}
	else {
		/* DMA error, channel needs to be reset */
		pDMAApi->dma_abort(dma_handle, DMAREQ_USART0_TX);
	}
	/* Is another DMA descriptor waiting that was not chained? */
	if (countTXDescUsed > 0) {
		nextTXDesc++;
		/* Setup the Channel configuration structure with Peripheral request enabled for UART Tx
		     priority is 3 and setup callback routine */
		dma_ch_cfg.event = DMA_ROM_CH_EVENT_PERIPH;
		dma_ch_cfg.hd_trigger = 0;
		dma_ch_cfg.priority = 3;
		dma_ch_cfg.cb_func = dmaTXDone;
		/* Setup Task Configuration structure, enable SW Trigger and INTA, 8 bit data width,
		   with no destination increment. The transfer length is retrieved from the descriptor */
		dma_task_cfg.ch_num = DMAREQ_USART0_TX;
		dma_task_cfg.config = DMA_ROM_TASK_CFG_SW_TRIGGER | DMA_ROM_TASK_CFG_SEL_INTA;
		dma_task_cfg.data_type = DMA_ROM_TASK_DATA_WIDTH_8 | DMA_ROM_TASK_SRC_INC_1 | DMA_ROM_TASK_DEST_INC_0;
		dma_task_cfg.data_length = dmaTXDesc[nextTXDesc].xfercfg;
		dma_task_cfg.src = dmaTXDesc[nextTXDesc].source;
		dma_task_cfg.dst = dmaTXDesc[nextTXDesc].dest;
		dma_task_cfg.task_addr = (uint32_t) &dmaTXDesc[nextTXDesc];
		/* Call DMA Init routine to start Tx transfer for newly added descriptor */
		err_code = pDMAApi->dma_init(dma_handle, &dma_ch_cfg, &dma_task_cfg);
		err_code = pDMAApi->dma_set_valid(dma_handle, DMAREQ_USART0_TX);
	}
}

/* Receive Callback for DMA Rx Channel */
static void dmaRXDone(uint32_t err_code, uint32_t int_type)
{
	/* Handle errors if needed */
	if ((ErrorCode_t) err_code == LPC_OK) {
		uartRxAvail = true;
	}
	else {
		/* DMA error, channel needs to be reset */
		pDMAApi->dma_abort(dma_handle, DMAREQ_USART0_RX);
		dmaRXQueue();
	}
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
	dmaTXDesc[countTXDescUsed].source = (uint32_t) (data + bytes - 1);	/* Last address here */
	dmaTXDesc[countTXDescUsed].dest = (uint32_t) &LPC_USART0->TXDATA;	/* Byte aligned */

	/* If there are multiple buffers with non-contiguous addresses, they can be chained
	   together here. If another TX buffer needs to be sent, the DMA
	   IRQ handler will re-queue and send the buffer there without using chaining. */
	dmaTXDesc[countTXDescUsed].next = DMA_ADDR(0);

	/* Temporarily store length in transfer configuration */
	dmaTXDesc[countTXDescUsed].xfercfg = bytes - 1;

	/* If a transfer is currently in progress, then stop here and let the DMA
	   handler re-queue the next transfer. Otherwise, start the transfer here. */
	if (countTXDescUsed == 0) {
		/* Setup the Channel configuration structure with Peripheral request enabled for UART Tx
		     priority is 3 and setup callback routine */
		dma_ch_cfg.event = DMA_ROM_CH_EVENT_PERIPH;
		dma_ch_cfg.hd_trigger = 0;
		dma_ch_cfg.priority = 3;
		dma_ch_cfg.cb_func = dmaTXDone;
		/* Setup Task Configuration structure, enable SW Trigger and INTA, 8 bit data width,
		   with no destination increment. The transfer length is retrieved from the descriptor */
		dma_task_cfg.ch_num = DMAREQ_USART0_TX;
		dma_task_cfg.config = DMA_ROM_TASK_CFG_SW_TRIGGER | DMA_ROM_TASK_CFG_SEL_INTA;
		dma_task_cfg.data_type = DMA_ROM_TASK_DATA_WIDTH_8 | DMA_ROM_TASK_SRC_INC_1 | DMA_ROM_TASK_DEST_INC_0;
		dma_task_cfg.data_length = dmaTXDesc[countTXDescUsed].xfercfg;
		dma_task_cfg.src = dmaTXDesc[countTXDescUsed].source;
		dma_task_cfg.dst = dmaTXDesc[countTXDescUsed].dest;
		dma_task_cfg.task_addr = (uint32_t) &dmaTXDesc[countTXDescUsed];
		/* Call DMA Init routine to start Tx transfer for newly added descriptor */
		err_code = pDMAApi->dma_init(dma_handle, &dma_ch_cfg, &dma_task_cfg);
		err_code = pDMAApi->dma_set_valid(dma_handle, DMAREQ_USART0_TX);
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
			/* Wrap descriptors for the last one*/
			dma_task_cfg.config = DMA_ROM_TASK_CFG_SEL_INTA | DMA_ROM_TASK_CFG_PING_PONG_EN;
		}
		else {
			dma_task_cfg.config = DMA_ROM_TASK_CFG_SEL_INTA;
		}
		/* Update the Task Config structure for DMA ROM driver with no source increment, 8 bit data width*/
		dma_task_cfg.ch_num = DMAREQ_USART0_RX;
		dma_task_cfg.data_type = DMA_ROM_TASK_DATA_WIDTH_8 | DMA_ROM_TASK_SRC_INC_0 | DMA_ROM_TASK_DEST_INC_1;
		dma_task_cfg.data_length = UARTRXBUFFSIZE - 1;
		dma_task_cfg.src = (uint32_t) &LPC_USART0->RXDATA;
		dma_task_cfg.dst = (uint32_t) (&dmaRXBuffs[i][0] + UARTRXBUFFSIZE - 1);
		dma_task_cfg.task_addr = (uint32_t) &dmaRXDesc[i];
		/* If it is the first descriptor then setup channel config structure and call Init */
		if (i == 0) {
			dma_ch_cfg.event = DMA_ROM_CH_EVENT_PERIPH;
			dma_ch_cfg.hd_trigger = 0;
			dma_ch_cfg.priority = 3;
			dma_ch_cfg.cb_func = dmaRXDone;
			err_code = pDMAApi->dma_init(dma_handle, &dma_ch_cfg, &dma_task_cfg);
		}
		/* Link descriptors */
		else {
			err_code = pDMAApi->dma_link(dma_handle, &dma_task_cfg, 1);
		}
	}
	err_code = pDMAApi->dma_set_valid(dma_handle, DMAREQ_USART0_RX);
	Chip_DMA_SWTriggerChannel(LPC_DMA, DMAREQ_USART0_RX);
}

/* Check and return UART RX data if it exists */
static int checkRxData(uint8_t *buff)
{
	int bytesRec = 0;

	if (uartRxAvail) {
		uartRxAvail = false;

		memcpy(buff, (void *) dmaRXBuffs[uartRXBuff], UARTRXBUFFSIZE);
		uartRXBuff++;
		if (uartRXBuff >= UARTRXDESC) {
			/* Since we are using Ping Pong transfers
			     the buffer is reset to 1 and not 0*/
			uartRXBuff = 1;
		}
		bytesRec = UARTRXBUFFSIZE;
	}

	return bytesRec;
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
	/* Call ROM driver handler */
	pDMAApi->dma_isr(dma_handle);
}

/**
 * @brief	Main UART/DMA program body
 * @return	Does not exit
 */
int main(void)
{
	int bytes = 0, idx;
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
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_DMA);
	Chip_SYSCTL_PeriphReset(RESET_DMA);
	/* Initialize ROM API pointer */
	pDMAApi = LPC_DMAD_API;
	size_in_bytes = pDMAApi->dma_get_mem_size();
	if (size_in_bytes > RAMBLOCK) {
		/* Adjust RAMBLOCK size in this case */
		return 1;
	}
	/* Setup DMA ROM driver, provided RAM blocks for DMA Handle */
	dma_handle = pDMAApi->dma_setup(LPC_DMA_BASE, dma_ram_block);

	/* Init UART 0 TX descriptor */
	countTXDescUsed = 0;

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
		if (countTXDescUsed == 0) {
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
