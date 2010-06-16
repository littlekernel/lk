/*
 * Copyright (c) 2009-2010 Travis Geiselbrecht
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
#include <debug.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/avr32.h>
#include <arch/defines.h>

void arch_disable_cache(uint flags)
{
	PANIC_UNIMPLEMENTED;
}

void arch_enable_cache(uint flags)
{
	PANIC_UNIMPLEMENTED;
}

void arch_clean_cache_range(addr_t start, size_t len)
{
	addr_t end = start + len;	

	for (start = start & ~(CACHE_LINE - 1); start < end; start += CACHE_LINE)
		avr32_dcache_clean_line((void *)start);
}

void arch_clean_invalidate_cache_range(addr_t start, size_t len)
{
	addr_t end = start + len;	

	for (start = start & ~(CACHE_LINE - 1); start < end; start += CACHE_LINE)
		avr32_dcache_clean_invalidate_line((void *)start);
}

