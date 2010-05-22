#ifndef __LIB_FONT_H
#define __LIB_FONT_H

#include <lib/gfx.h>

#define FONT_X	6
#define FONT_Y	12

void font_draw_char(gfx_surface *surface, unsigned char c, int x, int y, uint32_t color);

#endif

