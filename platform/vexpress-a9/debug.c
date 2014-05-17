/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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
#include <stdio.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <platform/vexpress-a9.h>
#include <reg.h>

#define DR (0x00)
#define FR (0x18)

#define UARTREG(reg)  (*REG32(UART0 + (reg)))

void platform_dputc(char c)
{
	UARTREG(DR) = c;
}

int platform_dgetc(char *c, bool wait)
{
	if (!wait) {
		if (UARTREG(FR) & (1<<4)) {
			/* fifo empty */
			return -1;
		}
		*c = UARTREG(DR) & 0xff;
		return 0;
	} else {
		while ((UARTREG(FR) & (1<<4))) {
			// XXX actually block on interrupt
			thread_yield();
		}

		*c = UARTREG(DR) & 0xff;
		return 0;
	}
}

void platform_halt(void)
{
	arch_disable_ints();
	for (;;);
}

