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
#ifndef __LIB_GFX_H
#define __LIB_GFX_H

#include <stdbool.h>
#include <sys/types.h>
#include <inttypes.h>
#include <compiler.h>

// gfx library

__BEGIN_CDECLS

// different graphics formats
typedef enum {
    GFX_FORMAT_NONE,
    GFX_FORMAT_RGB_565,
    GFX_FORMAT_RGB_332,
    GFX_FORMAT_RGB_2220,
    GFX_FORMAT_ARGB_8888,
    GFX_FORMAT_RGB_x888,
    GFX_FORMAT_MONO,

    GFX_FORMAT_MAX
} gfx_format;

#define MAX_ALPHA 255

/**
 * @brief  Describe a graphics drawing surface
 *
 * The gfx_surface object represents a framebuffer that can be rendered
 * to.  Elements include a pointer to the actual pixel memory, its size, its
 * layout, and pointers to basic drawing functions.
 *
 * @ingroup graphics
 */
typedef struct gfx_surface {
    void *ptr;
    bool free_on_destroy;
    gfx_format format;
    uint width;
    uint height;
    uint stride;
    uint pixelsize;
    size_t len;
    uint alpha;

    // function pointers
    uint32_t (*translate_color)(uint32_t input);
    void (*copyrect)(struct gfx_surface *, uint x, uint y, uint width, uint height, uint x2, uint y2);
    void (*fillrect)(struct gfx_surface *, uint x, uint y, uint width, uint height, uint color);
    void (*putpixel)(struct gfx_surface *, uint x, uint y, uint color);
    void (*flush)(uint starty, uint endy);
} gfx_surface;

// copy a rect from x,y with width x height to x2, y2
void gfx_copyrect(gfx_surface *surface, uint x, uint y, uint width, uint height, uint x2, uint y2);

// fill a rect within the surface with a color
void gfx_fillrect(gfx_surface *surface, uint x, uint y, uint width, uint height, uint color);

// draw a pixel at x, y in the surface
void gfx_putpixel(gfx_surface *surface, uint x, uint y, uint color);

// draw a single pixel line between x1,y1 and x2,y1
void gfx_line(gfx_surface *surface, uint x1, uint y1, uint x2, uint y2, uint color);

// clear the entire surface with a color
static inline void gfx_clear(gfx_surface *surface, uint color)
{
    surface->fillrect(surface, 0, 0, surface->width, surface->height, color);

    if (surface->flush)
        surface->flush(0, surface->height-1);
}

// blend between two surfaces
void gfx_surface_blend(struct gfx_surface *target, struct gfx_surface *source, uint destx, uint desty);

void gfx_flush(struct gfx_surface *surface);

void gfx_flush_rows(struct gfx_surface *surface, uint start, uint end);

// surface setup
gfx_surface *gfx_create_surface(void *ptr, uint width, uint height, uint stride, gfx_format format);

// utility routine to make a surface out of a display framebuffer
struct display_framebuffer;
gfx_surface *gfx_create_surface_from_display(struct display_framebuffer *) __NONNULL((1));

// free the surface
// optionally frees the buffer if the free bit is set
void gfx_surface_destroy(struct gfx_surface *surface);

// utility routine to fill the display with a little moire pattern
void gfx_draw_pattern(void);

__END_CDECLS

#endif

