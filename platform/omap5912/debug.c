/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <kernel/thread.h>
#include <kernel/timer.h>
#include <platform/debug.h>
#include <arch/ops.h>
#include <platform/omap5912.h>
#include <lib/cbuf.h>

static cbuf_t debug_buf;
static timer_t debug_timer;

static void write_uart_reg(int uart, int reg, unsigned char data)
{
	unsigned long base;
	int mul = 4;

	switch(uart) {
		case 0: base = UART0_BASE; break;
		case 1: base = UART1_BASE; break;
		case 2: base = UART2_BASE; break;
		default: return;
	}

	*(volatile unsigned char *)(base + reg * mul) = data;
}

static unsigned char read_uart_reg(int uart, int reg)
{
	unsigned long base;
	int mul = 4;

	switch(uart) {
		case 0: base = UART0_BASE; break;
		case 1: base = UART1_BASE; break;
		case 2: base = UART2_BASE; break;
		default: return 0;
	}

	return *(volatile unsigned char *)(base + reg * mul);
}

static int uart_init(void)
{
	/* clear the tx & rx fifo and disable */
	write_uart_reg(0, UART_FCR, 0x6);

	return 0;
}

static int uart_putc(int port, char c )
{
	while (!(read_uart_reg(port, UART_LSR) & (1<<6))) // wait for the shift register to empty
		;
  	write_uart_reg(port, UART_THR, c);
	return 0;
}

static int uart_getc(int port, bool wait)  /* returns -1 if no data available */
{
	if (wait) {
		while (!(read_uart_reg(port, UART_LSR) & (1<<0))) // wait for data to show up in the rx fifo
			;
	} else {
		if (!(read_uart_reg(port, UART_LSR) & (1<<0)))
			return -1;
	}
	return read_uart_reg(port, UART_RHR);
}

void _dputc(char c)
{
	if (c == '\n')
		uart_putc(0, '\r');
	uart_putc(0, c);
}

static enum handler_return debug_timer_callback(timer_t *t, time_t now, void *arg)
{
	signed char c;
	c = uart_getc(0, false);
	if (c > 0) {
		cbuf_write(&debug_buf, &c, 1, false);
		return INT_RESCHEDULE;
	} else {
		return INT_NO_RESCHEDULE;
	}
}

int dgetc(char *c, bool wait)
{
	ssize_t len;

	len = cbuf_read(&debug_buf, c, 1, wait);
	return len;
}

void debug_dump_regs(void)
{
	PANIC_UNIMPLEMENTED;
}

void platform_halt(void)
{
	dprintf(ALWAYS, "HALT: spinning forever...\n");
	for(;;);
}

void debug_dump_memory_bytes(void *mem, int len)
{
	PANIC_UNIMPLEMENTED;
}

void debug_dump_memory_halfwords(void *mem, int len)
{
	PANIC_UNIMPLEMENTED;
}

void debug_dump_memory_words(void *mem, int len)
{
	PANIC_UNIMPLEMENTED;
}

void debug_set_trace_level(int trace_type, int level)
{
	PANIC_UNIMPLEMENTED;
}

void platform_init_debug(void)
{
	cbuf_initialize(&debug_buf, 512);
	timer_set_periodic(&debug_timer, 10, &debug_timer_callback, NULL);
}
