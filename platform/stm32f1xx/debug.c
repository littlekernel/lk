/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdarg.h>
#include <reg.h>
#include <debug.h>
#include <printf.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <arch/ops.h>
#include <dev/uart.h>
#include <target/debugconfig.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_usart.h>
#include <arch/arm/cm3.h>

static cbuf_t debug_rx_buf;

void stm32_debug_early_init(void)
{
	// XXX move this into usart driver
	if (DEBUG_UART == USART1) {
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	} else if (DEBUG_UART == USART2) {
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	} else if (DEBUG_UART == USART3) {
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	}

	USART_InitTypeDef init;

	init.USART_BaudRate = 115200;
	init.USART_WordLength = USART_WordLength_8b;
	init.USART_StopBits = USART_StopBits_1;
	init.USART_Parity = USART_Parity_No;
	init.USART_Mode = USART_Mode_Tx|USART_Mode_Rx;
	init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

	USART_Init(DEBUG_UART, &init);

	USART_ITConfig(DEBUG_UART, USART_IT_RXNE, DISABLE);
	NVIC_DisableIRQ(DEBUG_UART_IRQ);	

	USART_Cmd(DEBUG_UART, ENABLE);
}

/* later in the init process */
void stm32_debug_init(void)
{
	cbuf_initialize(&debug_rx_buf, 16);

	USART_ITConfig(DEBUG_UART, USART_IT_RXNE, ENABLE);
	NVIC_EnableIRQ(DEBUG_UART_IRQ);	
}

void DEBUG_UART_IRQ_HANDLER(void)
{
	inc_critical_section();

	while (USART_GetFlagStatus(DEBUG_UART, USART_FLAG_RXNE)) {
		char c = USART_ReceiveData(DEBUG_UART);
		cbuf_write(&debug_rx_buf, &c, 1, false);
	}

	USART_ClearFlag(DEBUG_UART, USART_IT_RXNE);

	cm3_trigger_preempt();

	dec_critical_section();
}

void _dputc(char c)
{
	if (c == '\n') {
		_dputc('\r');
	}
	while (USART_GetFlagStatus(DEBUG_UART, USART_FLAG_TXE) == 0)
		;
	USART_SendData(DEBUG_UART, c);
	while (USART_GetFlagStatus(DEBUG_UART, USART_FLAG_TC) == 0)
		;
}

int dgetc(char *c, bool wait)
{
	return cbuf_read(&debug_rx_buf, c, 1, wait);
}

void platform_halt(void)
{
	dprintf(ALWAYS, "HALT: spinning forever...\n");
	for(;;);
}

