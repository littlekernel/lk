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
#include <printf.h>
#include <kernel/thread.h>
#include <platform/armemu/memmap.h>
#include <platform/debug.h>

void _dputc(char c)
{
	*REG8(DEBUG_STDOUT) = c;
}

int dgetc(char *c, bool wait)
{
	for (;;) {
		int8_t result = (int8_t)*REG8(DEBUG_STDIN);

		if (result == -1) {
			if (wait)
				continue;
			else
				return -1;
		}

		*c = (char)result;
		return 0;
	}
}

void debug_dump_regs(void)
{
	*REG32(DEBUG_REGDUMP) = 1;
}

void platform_halt(void)
{
	*REG32(DEBUG_HALT) = 1;
	for(;;);
}

void debug_dump_memory_bytes(void *mem, int len)
{
	*REG32(DEBUG_MEMDUMPADDR) = (unsigned int)mem;
	*REG32(DEBUG_MEMDUMPLEN) = len;
	*REG32(DEBUG_MEMDUMP_BYTE) = 1;
}

void debug_dump_memory_halfwords(void *mem, int len)
{
	len /= 2;

	*REG32(DEBUG_MEMDUMPADDR) = (unsigned int)mem;
	*REG32(DEBUG_MEMDUMPLEN) = len;
	*REG32(DEBUG_MEMDUMP_HALFWORD) = 1;
}

void debug_dump_memory_words(void *mem, int len)
{
	len /= 4;

	*REG32(DEBUG_MEMDUMPADDR) = (unsigned int)mem;
	*REG32(DEBUG_MEMDUMPLEN) = len;
	*REG32(DEBUG_MEMDUMP_WORD) = 1;
}

void debug_set_trace_level(int trace_type, int level)
{
	if(trace_type < 0 || trace_type >= 4)
		return;

	*REG32(DEBUG_SET_TRACELEVEL_CPU + trace_type * 4) = level;
}

uint32_t debug_cycle_count()
{
	return *REG32(DEBUG_CYCLE_COUNT);
}
