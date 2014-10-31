/*
 * Copyright (c) 2012 Corey Tabaka
 * Copyright (c) 2014 Xiaomi Inc.
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

#ifndef __DEV_UART_16550_H
#define __DEV_UART_16550_H

#include <lib/cbuf.h>

#define UART_RXBUF_SIZE			32

struct uart_16550_state {
	struct cbuf rxbuf;
	char buf[UART_RXBUF_SIZE];
};

struct uart_16550_config {
	unsigned irq;
	uintptr_t base;
	size_t unit;
	size_t stride;
	unsigned clock_rate;
	unsigned baud_rate;
	size_t word_length;
	size_t stop_bits;
	bool parity_enable;
	bool even_parity;
	bool autoflow_enable;
	/* optional reserved state */
	struct uart_16550_state *state;
};

#endif

