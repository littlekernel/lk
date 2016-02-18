/*
 * @brief UART0 ROM API interrupt example
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

/* UART handle and memory for ROM API */
static UART_HANDLE_T *uartHandle;

/* Use a buffer size larger than the expected return value of
   uart_get_mem_size() for the static UART handle type */
static uint32_t uartHandleMEM[0x10];

/* Receive buffer */
#define RECV_BUFF_SIZE 32
static char recv_buf[RECV_BUFF_SIZE];
static int indexIn;

/* ASCII code for escapre key */
#define ESCKEY 27

/* Flag used to indicate callback fired */
static volatile bool gotCallback;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/* UART Pin mux function - note that SystemInit() may already setup your
   pin muxing at system startup */
static void Init_UART0_PinMux(void)
{
	/* UART signals on pins PIO0_13 (FUNC0, U0_TXD) and PIO0_18 (FUNC0, U0_RXD) */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 13, (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, (IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGMODE_EN));

	/* UART signal muxing via SWM */
	Chip_SWM_MovablePortPinAssign(SWM_UART0_RXD_I, 0, 13);
	Chip_SWM_MovablePortPinAssign(SWM_UART0_TXD_O, 0, 18);
}

/* Turn on LED to indicate an error */
static void errorUART(void)
{
	Board_LED_Set(0, true);
	while (1) {}
}

/* Setup UART handle and parameters */
static void setupUART()
{
	uint32_t ret_value;

	/* 115.2KBPS, 8N1, ASYNC mode, no errors, clock filled in later */
	UART_CONFIG_T cfg = {
		0,				/* U_PCLK frequency in Hz */
		115200,		/* Baud Rate in Hz */
		1,				/* 8N1 */
		0,				/* Asynchronous Mode */
		NO_ERR_EN	/* Enable No Errors */
	};

	/* Initialize UART0 */
	Chip_UART_Init(LPC_USART0);

	Chip_Clock_SetUARTFRGDivider(1);

	/* Perform a sanity check on the storage allocation */
	if (LPC_UARTD_API->uart_get_mem_size() > sizeof(uartHandleMEM)) {
		/* Example only: this should never happen and probably isn't needed for
		   most UART code. */
		errorUART();
	}

	/* Setup the UART handle */
	uartHandle = LPC_UARTD_API->uart_setup((uint32_t) LPC_USART0, (uint8_t *) &uartHandleMEM);
	if (uartHandle == NULL) {
		errorUART();
	}

	/* Need to tell UART ROM API function the current UART peripheral clock
	     speed */
	cfg.sys_clk_in_hz = Chip_Clock_GetSystemClockRate();

	/* Initialize the UART with the configuration parameters */
	ret_value = LPC_UARTD_API->uart_init(uartHandle, &cfg);
	LPC_SYSCTL->FRGCTRL = ret_value;

}

/* UART callback */
static void waitCallback(uint32_t err_code, uint32_t n)
{
	gotCallback = true;
}

/* Sleep until callback is called */
void sleepUntilCB(void)
{
	/* Wait until the transmit callback occurs. When it hits, the
	     transfer is complete. */
	while (gotCallback == false) {
		/* Sleep until the callback signals transmit completion */
		__WFI();
	}
}

/* Send a string on the UART terminated by a NULL character using
   interrupt mode. */
static void putLineUART(const char *send_data)
{
	UART_PARAM_T param;

	param.buffer = (uint8_t *) send_data;
	param.size = strlen(send_data);

	/* Interrupt mode, do not append CR/LF to sent data */
	param.transfer_mode = TX_MODE_SZERO;
	param.driver_mode = DRIVER_MODE_INTERRUPT;

	/* Setup the transmit callback, this will get called when the
	   transfer is complete */
	param.callback_func_pt = (UART_CALLBK_T) waitCallback;

	/* Transmit the data using interrupt mode, the function will
	   return */
	gotCallback = false;

	if (LPC_UARTD_API->uart_put_line(uartHandle, &param)) {
		errorUART();
	}

	/* Wait until the transmit callback occurs. When it hits, the
	   transfer is complete. */
	sleepUntilCB();
}

/* Receive a string on the UART terminated by a LF character using
   polling mode. */
static void getLineUART(char *receive_buffer, uint32_t length)
{
	UART_PARAM_T param;

	param.buffer = (uint8_t *) receive_buffer;
	param.size = length;

	/* Receive data up to the CR/LF character in polling mode. Will
	   truncate at length if too long.	*/
	param.transfer_mode = RX_MODE_CRLF_RECVD;
	param.driver_mode = DRIVER_MODE_INTERRUPT;

	/* Setup the receive callback, this will get called when the
	   transfer is complete */
	param.callback_func_pt = (UART_CALLBK_T) waitCallback;

	/* Receive the data */
	gotCallback = false;
	if (LPC_UARTD_API->uart_get_line(uartHandle, &param)) {
		errorUART();
	}

	/* Wait until the transmit callback occurs. When it hits, the
	   transfer is complete. */
	sleepUntilCB();
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	Handle UART interrupt
 * @return	Nothing
 */
void UART0_IRQHandler(void)
{
	LPC_UARTD_API->uart_isr(uartHandle);
}

/**
 * @brief	Main UART program body
 * @return	Always returns 1
 */
int main(void)
{
	SystemCoreClockUpdate();
	Board_Init();
	Init_UART0_PinMux();
	Board_LED_Set(0, false);

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

	/* Allocate UART handle, setup UART parameters, and initialize UART
	   clocking */
	setupUART();

	NVIC_EnableIRQ(UART0_IRQn);

	/* Transmit the welcome message and instructions using the
	   putline function */

	putLineUART("LPC15XX USART ROM API interrupt Example\r\n");
	putLineUART("String receive (no echo): "
				"Enter a string and press enter to echo if back\r\n");

	/* Get a string for the UART and echo it back to the caller. Data is NOT
	   echoed back via the UART using this function. */
	getLineUART(recv_buf, sizeof(recv_buf));
	recv_buf[sizeof(recv_buf) - 1] = '\0';	/* Safety */
	if (strlen(recv_buf) == (sizeof(recv_buf) - 1)) {
		putLineUART("**String was truncated, input data longer than "
					"receive buffer***\r\n");
	}
	putLineUART(recv_buf);

	/* Transmit the message for byte/character part of the exampel */
	putLineUART("\r\nByte receive with echo using uart_get_line(): "
				"Send16 bytes to fill the ring buffers\r\n");

	/* An (inefficient) ring buffer can be emulated in interrupt
	   mode using the uart_get_line() with a buffer size of 1. Read
	   16 bytes into the ring buffer and then exit.	*/
	indexIn = 0;
	while (indexIn < 16) {
		/* Get byte in next index in buffer */
		getLineUART(&recv_buf[indexIn], 1);

		/* Echo back using polling function */
		LPC_UARTD_API->uart_put_char(uartHandle, recv_buf[indexIn]);

		/* Next index */
		indexIn++;
	}

	/* Safe string termination */
	recv_buf[indexIn] = '\0';

	/* Transmit the message for byte/character part of the example */
	putLineUART("\r\nData in ring buffer: ");
	putLineUART(recv_buf);

	return 1;
}
