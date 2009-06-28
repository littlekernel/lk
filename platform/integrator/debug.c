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
#include <platform/debug.h>
#include <arch/ops.h>
#include <platform/integrator.h>

static void write_uart_reg(int uart, int reg, unsigned char data)
{
	unsigned long base;
	int mul = 4;

	switch(uart) {
		case 0: base = INTEGRATOR_UART0_REG_BASE; break;
		case 1: base = INTEGRATOR_UART1_REG_BASE; break;
		default: return;
	}

	*(volatile unsigned char *)(base + reg * mul) = data;
}

static unsigned char read_uart_reg(int uart, int reg)
{
	unsigned long base;
	int mul = 4;

	switch(uart) {
		case 0: base = INTEGRATOR_UART0_REG_BASE; break;
		case 1: base = INTEGRATOR_UART1_REG_BASE; break;
		default: return 0;
	}

	return *(volatile unsigned char *)(base + reg * mul);
}

static int uart_init(void)
{
#if 0
	/* clear the tx & rx fifo and disable */
	write_uart_reg(0, UART_FCR, 0x6);
#endif

	return 0;
}

static int uart_putc(int port, char c )
{
	write_uart_reg(0, PL011_UARTDR, c);
#if 0
	while (!(read_uart_reg(port, UART_LSR) & (1<<6))) // wait for the shift register to empty
		;
  	write_uart_reg(port, UART_THR, c);
#endif
	return 0;
}

static int uart_getc(int port, bool wait)  /* returns -1 if no data available */
{
#if 0
	if (wait) {
		while (!(read_uart_reg(port, UART_LSR) & (1<<0))) // wait for data to show up in the rx fifo
			;
	} else {
		if (!(read_uart_reg(port, UART_LSR) & (1<<0)))
			return -1;
	}
	return read_uart_reg(port, UART_RHR);
#endif
	return -1;
}

void _dputc(char c)
{
	uart_putc(0, c);
}

int dgetc(char *c, bool wait)
{
	int result = uart_getc(0, false);

	if (result < 0)
		return -1;

	*c = result;
	return 0;
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

