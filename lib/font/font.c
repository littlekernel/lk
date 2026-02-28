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

#include <lib/font.h>
#include <lib/gfx.h>
#include <lk/debug.h>

#include "font.h"

/**
 * @brief Draw one character from the built-in font
 *
 * @ingroup graphics
 */
void font_draw_char(gfx_surface *surface, unsigned char c, int x, int y, uint32_t color) {
    uint i, j;
    uint line;

    // 1bpp batched per-byte, for performance boost
    if (surface->format == GFX_FORMAT_MONO_1 && surface->spanmono1) {
        uint32_t c1 = surface->translate_color ? surface->translate_color(color) : color;
        gfx_span_op op = (c1 != 0) ? GFX_SPAN_SET : GFX_SPAN_CLR;

        for (i = 0; i < FONT_Y; i++) {
            line = FONT[(c * FONT_Y) + i];

            for (j = 0; j < FONT_X;) {
                if ((line & 0x1) == 0) {
                    line >>= 1;
                    j++;
                    continue;
                }

                uint start = j;
                do {
                    line >>= 1;
                    j++;
                } while (j < FONT_X && (line & 0x1));

                surface->spanmono1(surface, (x + start), (y + i), (j - start), op);
            }
        }

        gfx_flush_rows(surface, y, (y + FONT_Y));
        return;
    }

    // Per-pixel default
    for (i = 0; i < FONT_Y; i++) {
        line = FONT[(c * FONT_Y) + i];
        for (j = 0; j < FONT_X; j++) {
            if (line & 0x1) {
                gfx_putpixel(surface, (x + j), (y + i), color);
            }
            line = line >> 1;
        }
    }
    gfx_flush_rows(surface, y, (y + FONT_Y));
}
