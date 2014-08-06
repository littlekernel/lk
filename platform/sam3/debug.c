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
#include <stdio.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <arch/ops.h>
#include <target/debugconfig.h>
#include <arch/arm/cm.h>

#include <uart/uart.h>
#include <pmc/pmc.h>
#include <pio/pio.h>

static cbuf_t debug_rx_buf;

void sam3_uart_irq(void)
{
	arm_cm_irq_entry();

	bool resched = false;
	unsigned char c;
	if (uart_read(UART, &c) == 0) {
		cbuf_write_char(&debug_rx_buf, c, false);
		resched = true;
	}

	arm_cm_irq_exit(resched);
}

void sam_debug_early_init(void)
{
	pmc_enable_periph_clk(ID_UART);
	pmc_enable_periph_clk(ID_PIOA);

	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA8);
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA9);

	sam_uart_opt_t opt;

	opt.ul_mck = 84000000;
	opt.ul_baudrate = 115200;
	opt.ul_mode = UART_MR_PAR_NO | UART_MR_CHMODE_NORMAL;

	NVIC_DisableIRQ(UART_IRQn);

	uart_init(UART, &opt);

	uart_enable(UART);
}

void sam_debug_init(void)
{
	cbuf_initialize(&debug_rx_buf, 16);
	NVIC_EnableIRQ(UART_IRQn);
	uart_enable_interrupt(UART, UART_IER_RXRDY);
}

void platform_dputc(char c)
{
	if (c == '\n') {
		_dputc('\r');
	}

	while (!uart_is_tx_ready(UART))
		;
	uart_write(UART, c);
}

int platform_dgetc(char *c, bool wait)
{
	return cbuf_read_char(&debug_rx_buf, c, wait);
}

