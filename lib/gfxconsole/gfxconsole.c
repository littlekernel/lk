/*
 * Copyright (c) 2008-2010, 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/**
 * @file
 * @brief  Manage graphics console
 *
 * This file contains functions to provide stdout to the graphics console.
 *
 * @ingroup graphics
 */

#include <lk/debug.h>
#include <assert.h>
#include <lib/io.h>
#include <lk/init.h>
#include <lib/gfx.h>
#include <lib/gfxconsole.h>
#include <lib/font.h>
#include <dev/display.h>

/** @addtogroup graphics
 * @{
 */

/**
 * @brief  Represent state of graphics console
 */
static struct {
    gfx_surface *surface;
    uint rows, columns;
    uint extray; // extra pixels left over if the rows doesn't fit precisely

    uint x, y;

    uint32_t front_color;
    uint32_t back_color;
} gfxconsole;

static void gfxconsole_putc(char c) {
    static enum { NORMAL, ESCAPE } state = NORMAL;
    static uint32_t p_num = 0;

    switch (state) {
        case NORMAL: {
            if (c == '\n' || c == '\r') {
                gfxconsole.x = 0;
                gfxconsole.y++;
            } else if (c == 0x1b) {
                p_num = 0;
                state = ESCAPE;
            } else {
                font_draw_char(gfxconsole.surface, c, gfxconsole.x * FONT_X, gfxconsole.y * FONT_Y, gfxconsole.front_color);
                gfxconsole.x++;
            }
            break;
        }

        case ESCAPE: {
            if (c >= '0' && c <= '9') {
                p_num = (p_num * 10) + (c - '0');
            } else if (c == 'D') {
                if (p_num <= gfxconsole.x)
                    gfxconsole.x -= p_num;
                state = NORMAL;
            } else if (c == '[') {
                // eat this character
            } else {
                font_draw_char(gfxconsole.surface, c, gfxconsole.x * FONT_X, gfxconsole.y * FONT_Y, gfxconsole.front_color);
                gfxconsole.x++;
                state = NORMAL;
            }
            break;
        }
    }

    if (gfxconsole.x >= gfxconsole.columns) {
        gfxconsole.x = 0;
        gfxconsole.y++;
    }
    if (gfxconsole.y >= gfxconsole.rows) {
        // scroll up
        gfx_copyrect(gfxconsole.surface, 0, FONT_Y, gfxconsole.surface->width, gfxconsole.surface->height - FONT_Y - gfxconsole.extray, 0, 0);
        gfxconsole.y--;
        gfx_fillrect(gfxconsole.surface, 0, gfxconsole.surface->height - FONT_Y - gfxconsole.extray, gfxconsole.surface->width, FONT_Y, gfxconsole.back_color);
        gfx_flush(gfxconsole.surface);
    }
}

void gfxconsole_print_callback(print_callback_t *cb, const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        gfxconsole_putc(str[i]);
    }
}

static print_callback_t cb = {
    .entry = { 0 },
    .print = gfxconsole_print_callback,
    .context = NULL
};

/**
 * @brief  Initialize graphics console on given drawing surface.
 *
 * The graphics console subsystem is initialized, and registered as
 * an output device for debug output.
 */
void gfxconsole_start(gfx_surface *surface) {
    DEBUG_ASSERT(gfxconsole.surface == NULL);

    // set up the surface
    gfxconsole.surface = surface;

    // calculate how many rows/columns we have
    gfxconsole.rows = surface->height / FONT_Y;
    gfxconsole.columns = surface->width / FONT_X;
    gfxconsole.extray = surface->height - (gfxconsole.rows * FONT_Y);

    dprintf(SPEW, "gfxconsole: rows %d, columns %d, extray %d\n", gfxconsole.rows, gfxconsole.columns, gfxconsole.extray);

    // start in the upper left
    gfxconsole.x = 0;
    gfxconsole.y = 0;

    // colors are white and black for now
    gfxconsole.front_color = 0xffffffff;
    gfxconsole.back_color = 0;

    // register for debug callbacks
    register_print_callback(&cb);
}

/**
 * @brief  Initialize graphics console on default display
 */
void gfxconsole_start_on_display(void) {
    static bool started = false;

    if (started)
        return;

    /* pop up the console */
    struct display_framebuffer fb;
    if (display_get_framebuffer(&fb) < 0)
        return;

    gfx_surface *s = gfx_create_surface_from_display(&fb);
    gfxconsole_start(s);
    started = true;
}

static void gfxconsole_init_hook(uint level) {
    gfxconsole_start_on_display();
}

LK_INIT_HOOK(gfxconsole, &gfxconsole_init_hook, LK_INIT_LEVEL_PLATFORM);
