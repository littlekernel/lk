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
#include <platform/at91sam7.h>
#include <platform/mux.h>

void mux_init(void)
{
	AT91PIO *pio = AT91PIOA_ADDR;

	pio->output_disable = BOARD_OUTPUT_DISABLE;
	pio->output_enable = BOARD_OUTPUT_ENABLE;
	pio->pullup_disable = BOARD_PULLUP_DISABLE;
	pio->pullup_enable = BOARD_PULLUP_ENABLE;
	pio->pio_disable = BOARD_PIO_DISABLE;
	pio->pio_enable = BOARD_PIO_ENABLE;
	pio->select_a = BOARD_SELECT_A;
	pio->select_b = BOARD_SELECT_B;
}

