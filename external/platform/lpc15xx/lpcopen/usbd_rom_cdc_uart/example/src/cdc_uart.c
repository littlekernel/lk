/*
 * @brief UART Comm port call back routines
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
#include <string.h>
#include "board.h"
#include "app_usbd_cfg.h"
#include "cdc_uart.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Enable this define to use integer clocking instead of the fractional baud
   rate generator */
#define USE_INTEGER_CLOCK

/* Set the default UART, IRQ number, and IRQ handler name */
#if defined(BOARD_NXP_LPCXPRESSO_1549)
#define LPC_USART       LPC_USART0
#define LPC_IRQNUM      UART0_IRQn
#define LPC_UARTHNDLR   UART0_IRQHandler

#else
/* Configure your own UART pin muxing here if needed */
#error "No UART setup defined"
#endif

/* Default baudrate for testing */
#define UART_TEST_DEFAULT_BAUDRATE 115200

/* Ring buffer size */
#define UCOM_BUF_SZ         64			/* The size should always be 2^n type.*/
#define UCOM_TX_CONNECTED   _BIT(8)		/* connection state is for both RX/Tx */
#define UCOM_TX_BUSY        _BIT(0)

#define UCOM_RX_UART_DIS    _BIT(0)
#define UCOM_RX_BUF_FULL    _BIT(1)
#define UCOM_RX_BUF_QUEUED  _BIT(2)
#define UCOM_RX_DB_QUEUED   _BIT(3)

/**
 * Structure containing Virtual Comm port control data
 */
typedef struct UCOM_DATA {
	USBD_HANDLE_T hUsb;		/*!< Handle to USB stack */
	USBD_HANDLE_T hCdc;		/*!< Handle to CDC class controller */

	uint8_t *rxBuf;			/*!< UART Rx buffer */
	uint8_t *txBuf;			/*!< UART Tx buffer */
	uint8_t txBuf_uartIndex;/*!< UART index in Tx buffer */
	int8_t txBuf_count;		/*!< Bytes present in Tx buffer */
	uint8_t rxBuf_uartIndex;/*!< UART index in Rx buffer */
	uint8_t rxBuf_usbIndex;	/*!< USB index in Rx buffer */

	volatile uint8_t usbTxBusy;		/*!< USB is busy sending previous packet */
	volatile uint8_t usbRxPending;	/*!< USB packet is pending in EP buffer */
} UCOM_DATA_T;

/** Virtual Comm port control data instance. */
static UCOM_DATA_T g_uCOM;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

static void Init_UART_PinMux(void)
{
#if defined(BOARD_NXP_LPCXPRESSO_1549)
	/* Disables pull-ups/pull-downs and enable digital mode */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 13, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));

	/* UART signal muxing via SWM */
	Chip_SWM_MovablePortPinAssign(SWM_UART0_RXD_I, 0, 13);
	Chip_SWM_MovablePortPinAssign(SWM_UART0_TXD_O, 0, 18);

#else
#warning "No UART nmuxing defined for this example"
#endif
}

/* UART port init routine */
void UCOM_UartInit(void)
{
	/* Board specific muxing */
	Init_UART_PinMux();

	/* Before setting up the UART, the global UART clock for USARTS 1-4
	   must first be setup. This requires setting the UART divider and
	   the UART base clock rate to 16x the maximum UART rate for all
	   UARTs. */
#if defined(USE_INTEGER_CLOCK)
	/* Use main clock rate as base for UART baud rate divider */
	Chip_Clock_SetUARTBaseClockRate(Chip_Clock_GetMainClockRate(), false);

#else
	/* Use 128x expected UART baud rate for fractional baud mode. */
	Chip_Clock_SetUARTBaseClockRate((115200 * 128), true);
#endif

	/* Setup UART */
	Chip_UART_Init(LPC_USART);
	Chip_UART_ConfigData(LPC_USART, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1);
	Chip_UART_SetBaud(LPC_USART, UART_TEST_DEFAULT_BAUDRATE);
	/* Optional for low clock rates only: Chip_UART_SetBaudWithRTC32K(LPC_USART, 300); */
	Chip_UART_Enable(LPC_USART);
	Chip_UART_TXEnable(LPC_USART);

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_USART, UART_INTEN_RXRDY);

	/* Enable Interrupt for UART channel */
	/* Priority = 1 */
	NVIC_SetPriority(LPC_IRQNUM, 1);
	/* Enable Interrupt for UART channel */
	NVIC_EnableIRQ(LPC_IRQNUM);
}

/* UCOM bulk EP_IN and EP_OUT endpoints handler */
static ErrorCode_t UCOM_bulk_hdlr(USBD_HANDLE_T hUsb, void *data, uint32_t event)
{
	UCOM_DATA_T *pUcom = (UCOM_DATA_T *) data;
	uint32_t count = 0;

	switch (event) {
	/* A transfer from us to the USB host that we queued has completed. */
	case USB_EVT_IN:
		/* check if UART had more data to send */
		if (pUcom->rxBuf_uartIndex < pUcom->rxBuf_usbIndex) {
			count = UCOM_BUF_SZ - pUcom->rxBuf_usbIndex;
		}
		else {
			count = pUcom->rxBuf_uartIndex - pUcom->rxBuf_usbIndex;
		}
		if (count) {
			pUcom->usbTxBusy = 1;
			count = USBD_API->hw->WriteEP(pUcom->hUsb, USB_CDC_IN_EP, &pUcom->rxBuf[g_uCOM.rxBuf_usbIndex], count);
			g_uCOM.rxBuf_usbIndex = (g_uCOM.rxBuf_usbIndex + count) & (UCOM_BUF_SZ - 1);
		}
		else {
			pUcom->usbTxBusy = 0;
		}
		break;

	/* We received a transfer from the USB host . */
	case USB_EVT_OUT:
		if ((Chip_UART_GetIntsEnabled(LPC_USART) & UART_INTEN_TXRDY) == 0) {
			pUcom->txBuf_count = USBD_API->hw->ReadEP(hUsb, USB_CDC_OUT_EP, pUcom->txBuf);
			pUcom->txBuf_uartIndex = 0;
			/* kick start UART tranmission */
			pUcom->txBuf_uartIndex = Chip_UART_Send(LPC_USART,
													&pUcom->txBuf[g_uCOM.txBuf_uartIndex],
													pUcom->txBuf_count);
			pUcom->txBuf_count -= pUcom->txBuf_uartIndex;
			/* Enable UART transmit interrupt */
			Chip_UART_IntEnable(LPC_USART, UART_INTEN_TXRDY);
		}
		else {
			pUcom->usbRxPending++;
		}
		break;

	default:
		break;
	}

	return LPC_OK;
}

/* Set line coding call back routine */
static ErrorCode_t UCOM_SetLineCode(USBD_HANDLE_T hCDC, CDC_LINE_CODING *line_coding)
{
	uint32_t config_data = 0;

	switch (line_coding->bDataBits) {
	case 7:
		config_data |= UART_CFG_DATALEN_7;
		break;

	case 9:
		config_data |= UART_CFG_DATALEN_9;
		break;

	case 8:
	default:
		config_data |= UART_CFG_DATALEN_8;
		break;
	}

	switch (line_coding->bCharFormat) {
	case 2:	/* 2 Stop Bits */
		config_data |= UART_CFG_STOPLEN_2;
		break;

	default:
	case 1:	/* 1 Stop Bit */
		config_data |= UART_CFG_STOPLEN_1;
		break;
	}

	switch (line_coding->bParityType) {
	case 1:
		config_data |= (UART_CFG_PARITY_ODD);
		break;

	case 2:
		config_data |= (UART_CFG_PARITY_EVEN);
		break;

	default:
	case 0:
		config_data |= UART_CFG_PARITY_NONE;
		break;
	}

	if (line_coding->dwDTERate < 3125000) {
		Chip_UART_SetBaud(LPC_USART, line_coding->dwDTERate);
	}
	Chip_UART_ConfigData(LPC_USART, config_data);

	return LPC_OK;
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	UART interrupt handler sub-routine
 * @return	Nothing
 */
void LPC_UARTHNDLR(void)
{
	uint32_t count = 0;
	/* Handle transmit interrupt if enabled */
	if ((Chip_UART_GetStatus(LPC_USART) & UART_STAT_TXRDY) != 0) {
		if (g_uCOM.txBuf_count > 0) {
			count = Chip_UART_Send(LPC_USART, &g_uCOM.txBuf[g_uCOM.txBuf_uartIndex], g_uCOM.txBuf_count);
			g_uCOM.txBuf_count -= count;
			g_uCOM.txBuf_uartIndex += count;
		}
		/* If  usbRxBuf empty check if any packet pending USB EP RAM */
		if (g_uCOM.txBuf_count < 1) {
			if ((g_uCOM.usbRxPending > 0) && USB_IsConfigured(g_uCOM.hUsb)) {
				g_uCOM.usbRxPending--;
				g_uCOM.txBuf_count = USBD_API->hw->ReadEP(g_uCOM.hUsb, USB_CDC_OUT_EP, g_uCOM.txBuf);
				g_uCOM.txBuf_uartIndex = 0;

			}
			else {
				/* all data transmitted on UART disable UART_INTEN_TXRDY */
				Chip_UART_IntDisable(LPC_USART, UART_INTEN_TXRDY);
			}
		}
	}

	/* Handle receive interrupt */
	count = Chip_UART_Read(LPC_USART, &g_uCOM.rxBuf[g_uCOM.rxBuf_uartIndex], UCOM_BUF_SZ - g_uCOM.rxBuf_uartIndex);

	if (count) {
		/* Note, following logic works if UCOM_BUF_SZ is 2^n size only. */
		g_uCOM.rxBuf_uartIndex = (g_uCOM.rxBuf_uartIndex + count) & (UCOM_BUF_SZ - 1);
		/* If USB Tx is not busy kick start USB Tx */
		if ((g_uCOM.usbTxBusy == 0) && USB_IsConfigured(g_uCOM.hUsb)) {
			g_uCOM.usbTxBusy = 1;
			count = USBD_API->hw->WriteEP(g_uCOM.hUsb, USB_CDC_IN_EP, &g_uCOM.rxBuf[g_uCOM.rxBuf_usbIndex], count);
			g_uCOM.rxBuf_usbIndex = (g_uCOM.rxBuf_usbIndex + count) & (UCOM_BUF_SZ - 1);
		}
	}

}

/* UART to USB com port init routine */
ErrorCode_t UCOM_init(USBD_HANDLE_T hUsb, USB_CORE_DESCS_T *pDesc, USBD_API_INIT_PARAM_T *pUsbParam)
{
	USBD_CDC_INIT_PARAM_T cdc_param;
	ErrorCode_t ret = LPC_OK;
	uint32_t ep_indx;
	USB_CDC_CTRL_T *pCDC;

	/* Store USB stack handle for future use. */
	g_uCOM.hUsb = hUsb;
	/* Initi CDC params */
	memset((void *) &cdc_param, 0, sizeof(USBD_CDC_INIT_PARAM_T));
	cdc_param.mem_base = pUsbParam->mem_base;
	cdc_param.mem_size = pUsbParam->mem_size;
	cdc_param.cif_intf_desc = (uint8_t *) find_IntfDesc(pDesc->high_speed_desc, CDC_COMMUNICATION_INTERFACE_CLASS);
	cdc_param.dif_intf_desc = (uint8_t *) find_IntfDesc(pDesc->high_speed_desc, CDC_DATA_INTERFACE_CLASS);
	cdc_param.SetLineCode = UCOM_SetLineCode;

	/* Init CDC interface */
	ret = USBD_API->cdc->init(hUsb, &cdc_param, &g_uCOM.hCdc);

	if (ret == LPC_OK) {
		/* allocate transfer buffers */
		g_uCOM.txBuf = (uint8_t *) cdc_param.mem_base;
		cdc_param.mem_base += UCOM_BUF_SZ;
		cdc_param.mem_size -= UCOM_BUF_SZ;
		g_uCOM.rxBuf = (uint8_t *) cdc_param.mem_base;
		cdc_param.mem_base += UCOM_BUF_SZ;
		cdc_param.mem_size -= UCOM_BUF_SZ;

		/* register endpoint interrupt handler */
		ep_indx = (((USB_CDC_IN_EP & 0x0F) << 1) + 1);
		ret = USBD_API->core->RegisterEpHandler(hUsb, ep_indx, UCOM_bulk_hdlr, &g_uCOM);

		if (ret == LPC_OK) {
			/* register endpoint interrupt handler */
			ep_indx = ((USB_CDC_OUT_EP & 0x0F) << 1);
			ret = USBD_API->core->RegisterEpHandler(hUsb, ep_indx, UCOM_bulk_hdlr, &g_uCOM);
			/* Set the line coding values as per UART Settings */
			pCDC = (USB_CDC_CTRL_T *) g_uCOM.hCdc;
			pCDC->line_coding.dwDTERate = 115200;
			pCDC->line_coding.bDataBits = 8;
			/* Init UART port for bridging */
			UCOM_UartInit();
		}

		/* update mem_base and size variables for cascading calls. */
		pUsbParam->mem_base = cdc_param.mem_base;
		pUsbParam->mem_size = cdc_param.mem_size;
	}

	return ret;
}
