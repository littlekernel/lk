/*
 * Copyright (c) 2008-2010 Travis Geiselbrecht
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

/**
 * @file
 * @brief  Font display
 *
 * This file contains functions to render fonts onto the graphics drawing
 * surface.
 *
 * @ingroup graphics
 */

#include <debug.h>
#include <lib/gfx.h>
#include <lib/font.h>

#include "font.h"

/**
 * @brief Draw one character from the built-in font
 *
 * @ingroup graphics
 */
void font_draw_char(gfx_surface *surface, unsigned char c, int x, int y, uint32_t color)
{
	uint i,j;
	uint line;

	// draw this char into a buffer
	for (i = 0; i < FONT_Y; i++) {
		line = FONT[c * FONT_Y + i];
		for (j = 0; j < FONT_X; j++) {
			if (line & 0x1)
				gfx_putpixel(surface, x + j, y + i, color);
			line = line >> 1;
		}
	}
	gfx_flush_rows(surface, y, y + FONT_Y);
}


