/*
 * Copyright (c) 2008-2010 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
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

#include <lk/debug.h>
#include <lib/gfx.h>
#include <lib/font.h>

#include "font.h"

/**
 * @brief Draw one character from the built-in font
 *
 * @ingroup graphics
 */
void font_draw_char(gfx_surface *surface, unsigned char c, int x, int y, uint32_t color) {
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


