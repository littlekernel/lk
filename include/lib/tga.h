#ifndef __LIB_TGA_H
#define __LIB_TGA_H

#include <lib/gfx.h>
#include <sys/types.h>

gfx_surface *tga_decode(const void *ptr, size_t len, gfx_format format);

#endif

