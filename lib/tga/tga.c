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
 * @brief  Parse tga format files
 *
 * @ingroup graphics
 */

#include <debug.h>
#include <compiler.h>
#include <lib/tga.h>

#define LOCAL_TRACE 0

struct tga_header {
	uint8_t  idlength;
	uint8_t  colormaptype;
	uint8_t  datatypecode;
	uint16_t colormaporigin;
	uint16_t colormaplength;
	uint8_t  colormapdepth;
	uint16_t x_origin;
	uint16_t y_origin;
	uint16_t width;
	uint16_t height;
	uint8_t  bitsperpixel;
	uint8_t  imagedescriptor;
} __PACKED;

static void print_tga_info(const struct tga_header *header)
{
	LTRACEF("idlength %hhd\n", header->idlength);	
	LTRACEF("colormaptype %hhd\n", header->colormaptype);	
	LTRACEF("datatypecode %hhd\n", header->datatypecode);	
	LTRACEF("colormaporigin %hd\n", header->colormaporigin);	
	LTRACEF("colormaplength %hd\n", header->colormaplength);	
	LTRACEF("colormapdepth %hhd\n", header->colormapdepth);	
	LTRACEF("x_origin %hd\n", header->x_origin);	
	LTRACEF("y_origin %hd\n", header->y_origin);	
	LTRACEF("width %hd\n", header->width);	
	LTRACEF("height %hd\n", header->height);	
	LTRACEF("bitsperpixel %hhd\n", header->bitsperpixel);	
	LTRACEF("imagedescriptor %hhd\n", header->imagedescriptor);	

}

static void decode_2byte(gfx_surface *surface, uint x, uint y, const void *input)
{
	const uint8_t *in = (const uint8_t *)input;

//	printf("in 0x%hhx 0x%hhx\n", in[0], in[1]);
	uint r,g,b;

	b = (in[0] & 0x1f) << 3;
	g = (((in[0] >> 5) & 0x7) | ((in[1] & 0x3) << 3)) << 3;
	r = ((in[1] >> 2) & 0x1f) << 3;

	gfx_putpixel(surface, x, y, 0xff000000 | r << 16 | g << 8 | b);
}

static void decode_3byte(gfx_surface *surface, uint x, uint y, const void *input)
{
	const uint8_t *in = (const uint8_t *)input;

//	printf("in 0x%hhx 0x%hhx\n", in[0], in[1]);

	gfx_putpixel(surface, x, y, 0xff000000 | in[2] << 16 | in[1] << 8 | in[0]);
}

static void decode_4byte(gfx_surface *surface, uint x, uint y, const void *input)
{
	const uint8_t *in = (const uint8_t *)input;

//	printf("in 0x%hhx 0x%hhx 0x%hhx 0x%hhx\n", in[0], in[1], in[2], in[3]);

	if (in[3] == 0)
		gfx_putpixel(surface, x, y, 0);
	else
		gfx_putpixel(surface, x, y, in[3] << 24 | in[2] << 16 | in[1] << 8 | in[0]);
}

/**
 * @brief  Decode a tga image
 *
 * @param  ptr  Pointer to tga data in memory
 * @param  len  Length of tga data
 * @param  format  Desired format of returned graphics surface
 *
 * @return Graphics surface or NULL on error.
 *
 * @ingroup graphics
 */
gfx_surface *tga_decode(const void *ptr, size_t len, gfx_format format)
{
	const struct tga_header *header = (const struct tga_header *)ptr;

	LTRACEF("ptr %p, len %zu\n", ptr, len);

#if LOCAL_TRACE > 0
	print_tga_info(header);
#endif

	/* do some sanity checks */
	if (header->datatypecode != 2 && header->datatypecode != 10) {
		dprintf(INFO, "tga_decode: unknown data type %d\n", header->datatypecode);
		return NULL;
	}
	if (header->bitsperpixel != 16 && header->bitsperpixel != 24 && header->bitsperpixel != 32) {
		dprintf(INFO, "tga_decode: unsupported bits per pixel %d\n", header->bitsperpixel);
		return NULL;
	}
	if (header->colormaptype != 0) {
		dprintf(INFO, "tga_decode: has colormap, can't handle\n");
		return NULL;
	}

	const void *imagestart = ((const uint8_t *)ptr + sizeof(struct tga_header) + header->idlength);

	/* create a surface to hold the decoded bits */
	gfx_surface *surface = gfx_create_surface(NULL, header->width, header->height, header->width, format);
	DEBUG_ASSERT(surface);

	/* copy the bits out */
	void (*decodefunc)(gfx_surface *, uint x, uint y, const void *) = NULL;

	uint step = 1;
	if (header->bitsperpixel == 16) {
		step = 2;
		decodefunc = decode_2byte;
	} else if (header->bitsperpixel == 24) {
		step = 3;
		decodefunc = decode_3byte;
	} else if (header->bitsperpixel == 32) {
		step = 4;
		decodefunc = decode_4byte;
	}

	if (header->datatypecode == 2) {
		/* no RLE */
		uint pos = 0;
		uint x, y;
		uint surfacey;

		for (y = 0; y < header->height; y++) {

			if ((header->imagedescriptor & (1 << 5)) == 0)
				surfacey = (surface->height - 1) - y;
			else
				surfacey = y;

			for (x = 0; x < header->width; x++) {
				decodefunc(surface, x, surfacey, (const uint8_t *)imagestart + pos);
				pos += step;
			}
		}
	} else if (header->datatypecode == 10) {
		/* RLE compression */
		uint pos = 0;
		uint count = 0;
		uint x, y;

		x = 0;
		if ((header->imagedescriptor & (1 << 5)) == 0)
			y = header->height - 1;
		else
			y = 0;

		while (count < (uint)header->height * (uint)header->width) {
			uint runpos;

			uint8_t run = *((const uint8_t *)imagestart + pos);
			bool repeat_run = (run & 0x80);
			uint runlen = (run & 0x7f) + 1;

//			printf("pos 0x%x count %u run 0x%hhx runtype %d runlen %u\n", pos, count, run, run & 0x80, runlen);

			/* consume the run byte */
			pos++;

			/* start of a run */
			for (runpos = 0; runpos < runlen; runpos++) {
				decodefunc(surface, x, y, (const uint8_t *)imagestart + pos);
				count++;

				x++;
				if (x == surface->width) {
					if ((header->imagedescriptor & (1 << 5)) == 0)
						y--;
					else
						y++;
					x = 0;
				}

				/* if a run of raw pixels, consume an input pixel */
				if (!repeat_run)
					pos += step;
			}
			/* if this was a run of repeated pixels, consume the one input pixel we repeated */
			if (repeat_run)
				pos += step;

		}
//		printf("done with RLE: x %d, y %d, pos %d, count %d\n", x, y, pos, count);
	}
	
	return surface;
}

