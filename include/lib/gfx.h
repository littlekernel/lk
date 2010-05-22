#ifndef __LIB_GFX_H
#define __LIB_GFX_H

#include <sys/types.h>

// gfx library

// different graphics formats
typedef enum {
	GFX_FORMAT_RGB_565,
	GFX_FORMAT_ARGB_8888,
	GFX_FORMAT_RGB_x888,

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

// utility routine to make a surface out of a display info
struct display_info;
gfx_surface *gfx_create_surface_from_display(struct display_info *);

// free the surface
// optionally frees the buffer if the free bit is set
void gfx_surface_destroy(struct gfx_surface *surface);

// utility routine to fill the display with a little moire pattern
void gfx_draw_pattern(void);

#endif

