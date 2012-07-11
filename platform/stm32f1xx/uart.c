/*
 * Copyright (c) 2012 Kent Ryhorchuk
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
#include <stdio.h>
#include <assert.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <arch/ops.h>
#include <dev/uart.h>
#include <target/debugconfig.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_usart.h>
#include <arch/arm/cm3.h>

#ifdef ENABLE_UART1
cbuf_t uart1_rx_buf;
#endif
#ifdef ENABLE_UART2
cbuf_t uart2_rx_buf;
#endif
#ifdef ENABLE_UART3
cbuf_t uart3_rx_buf;
#endif

#ifdef ENABLE_UART1
#endif
#ifdef ENABLE_UART2
#endif
#ifdef ENABLE_UART3
#endif

static void usart_init1(USART_TypeDef *usart, int irqn, cbuf_t *rxbuf)
{
	USART_InitTypeDef init;

	init.USART_BaudRate = 115200;
	init.USART_WordLength = USART_WordLength_8b;
	init.USART_StopBits = USART_StopBits_1;
	init.USART_Parity = USART_Parity_No;
	init.USART_Mode = USART_Mode_Tx|USART_Mode_Rx;
	init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;

	cbuf_initialize(rxbuf, 16);

	USART_Init(usart, &init);
	USART_ITConfig(usart, USART_IT_RXNE, ENABLE);
	NVIC_EnableIRQ(irqn);	
	USART_Cmd(usart, ENABLE);
}

void uart_init(void)
{

#ifdef ENABLE_UART1
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
#endif
#ifdef ENABLE_UART2
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
#endif
#ifdef ENABLE_UART3
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
#endif

#if (ENABLE_UART1 | ENABLE_UART2 | ENABLE_UART3)
	USART_InitTypeDef init;

	init.USART_BaudRate = 115200;
	init.USART_WordLength = USART_WordLength_8b;
	init.USART_StopBits = USART_StopBits_1;
	init.USART_Parity = USART_Parity_No;
	init.USART_Mode = USART_Mode_Tx|USART_Mode_Rx;
	init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
#endif

#ifdef ENABLE_UART1
	usart_init1(USART1, USART1_IRQn, &uart1_rx_buf);
#endif
#ifdef ENABLE_UART2
	usart_init1(USART2, USART2_IRQn, &uart2_rx_buf);
#endif
#ifdef ENABLE_UART3
	usart_init1(USART3, USART3_IRQn, &uart3_rx_buf);
#endif
}

void uart_rx_irq(USART_TypeDef *usart, cbuf_t *rxbuf)
{
	inc_critical_section();

	while (USART_GetFlagStatus(usart, USART_FLAG_RXNE)) {
		char c = USART_ReceiveData(usart);
		cbuf_write(rxbuf, &c, 1, false);
	}

	USART_ClearFlag(usart, USART_IT_RXNE);
	cm3_trigger_preempt();

	dec_critical_section();
}

static void usart_putc(USART_TypeDef *usart, char c)
{
	while (USART_GetFlagStatus(usart, USART_FLAG_TXE) == 0);
	USART_SendData(usart, c);
	while (USART_GetFlagStatus(usart, USART_FLAG_TC) == 0);
}

static int usart_getc(cbuf_t *rxbuf, bool wait)
{
	char c;
	cbuf_read(rxbuf, &c, 1, wait);
	return c;
}

static USART_TypeDef *get_usart(int port)
{
	switch (port) {
#ifdef ENABLE_UART1
	case 1:
		return USART1;
#endif
#ifdef ENABLE_UART2
	case 2:
	    return USART2;
#endif
#ifdef ENABLE_UART3
	case 3:
		return USART3;
#endif
	default:
		ASSERT(false);
		return 0;
	}

}

static cbuf_t *get_rxbuf(int port)
{
	switch (port) {
#ifdef ENABLE_UART1
	case 1:
		return &uart1_rx_buf;
#endif
#ifdef ENABLE_UART2
	case 2:
	    return &uart2_rx_buf;
#endif
#ifdef ENABLE_UART3
	case 3:
		return &uart3_rx_buf;
#endif
	default:
		ASSERT(false);
		return 0;
	}

}

int uart_putc(int port, char c)
{
	USART_TypeDef *usart = get_usart(port);
	usart_putc(usart, c);
	return 1;
}

int uart_getc(int port, bool wait)
{
	cbuf_t *rxbuf = get_rxbuf(port);
	return usart_getc(rxbuf, wait);
}

void uart_flush_tx(int port) {}

void uart_flush_rx(int port) {}

void uart_init_port(int port, uint baud)
{
	// TODO - later
	PANIC_UNIMPLEMENTED;
}
