/*
 * Copyright (c) 2008-2010 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
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

#include <lk/debug.h>
#include <lk/list.h>
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
void text_draw(int x, int y, const char *string) {
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
void text_update(void) {
    struct display_framebuffer fb;
    if (display_get_framebuffer(&fb) < 0)
        return;

    /* get the display's surface */
    gfx_surface *surface = gfx_create_surface_from_display(&fb);

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

