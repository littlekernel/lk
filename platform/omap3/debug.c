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
#include <dev/uart.h>
#include <target/debugconfig.h>

void _dputc(char c)
{
	if (c == '\n')
		uart_putc(DEBUG_UART, '\r');
	uart_putc(DEBUG_UART, c);
}

int dgetc(char *c, bool wait)
{
	int _c;

	if ((_c = uart_getc(DEBUG_UART, false)) < 0)
		return -1;

	*c = _c;
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

