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
#include <err.h>
#include <debug.h>
#include <platform.h>
#include <system_sam3x.h>
#include <sam3x8h.h>
#include <uart/uart.h>
#include <pmc/pmc.h>
#include <pio/pio.h>
#include <wdt/wdt.h>

void sam_debug_early_init(void);
void sam_debug_init(void);

void sam_timer_early_init(void);
void sam_timer_init(void);

void platform_early_init(void)
{
	SystemInit();

	sam_timer_early_init();

	wdt_disable(WDT);

	pmc_enable_periph_clk(ID_PIOC);
	pio_set_output(PIOC, PIO_PC9, 0, 0, 1);

	sam_debug_early_init();
}

void platform_init(void)
{
	sam_timer_init();
	sam_debug_init();
}

