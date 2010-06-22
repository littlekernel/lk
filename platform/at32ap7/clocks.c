/*
 * Copyright (c) 2010 Travis Geiselbrecht
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
#include <reg.h>
#include <platform/at32ap7.h>
#include "platform_p.h"

void platform_init_clocks(void)
{
	TRACE_ENTRY;

	TRACE_EXIT;
}

void platform_set_clock_enable(uint clock, bool enable)
{
	uint32_t reg;

	switch (CLOCK_TO_GROUP(clock)) {
		case CLOCK_GROUP_CPU: reg = PM_CPUMASK; break;
		case CLOCK_GROUP_HSB: reg = PM_HSBMASK; break;
		case CLOCK_GROUP_PBA: reg = PM_PBAMASK; break;
		case CLOCK_GROUP_PBB: reg = PM_PBBMASK; break;
		default:
			panic("platform_set_clock_enable: bad clock input 0x%x\n", clock);
	}

	if (enable) {
		*REG32(reg) |= 1 << CLOCK_IN_GROUP(clock);
	} else {
		*REG32(reg) &= ~(1 << CLOCK_IN_GROUP(clock));
	}
}

