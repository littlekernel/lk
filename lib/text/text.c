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
 * @addtogroup graphics
 * @{
 */

/**
 * @file
 * @brief  Console text display
 *
 * This module displays text on the console.  The text is retained so that
 * it can be redisplayed if the display is cleared.
 *
 * Output is to the default graphics display
 */

#include <debug.h>
#include <list.h>
#include <stdlib.h>
#include <string.h>
#include <dev/display.h>
#include <lib/gfx.h>
#include <lib/font.h>
#include <lib/text.h>

#define TEXT_COLOR 0xffffffff

static struct list_node text_list = LIST_INITIAL_VALUE(text_list);

struct text_line {
	struct list_node node;
	const char *str;
	int x, y;
};

/**
 * @brief  Add a string to the console text
 */
void text_draw(int x, int y, const char *string)
{
	struct text_line *line = malloc(sizeof(struct text_line));

	line->str = strdup(string);
	line->x = x;
	line->y = y;

	list_add_head(&text_list, &line->node);

	text_update();
}

/**
 * @brief  Refresh the display
 */
void text_update(void)
{
	struct display_info info;
	display_get_info(&info);

	/* get the display's surface */
	gfx_surface *surface = gfx_create_surface_from_display(&info);

	struct text_line *line;
	list_for_every_entry(&text_list, line, struct text_line, node) {
		const char *c;
		int x = line->x;
		for (c = line->str; *c; c++) {
			font_draw_char(surface, *c, x, line->y, TEXT_COLOR);
			x += FONT_X;
		}
	}

	gfx_flush(surface);

	gfx_surface_destroy(surface);
}

