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
#include <err.h>
#include <debug.h>
#include <platform.h>
#include "platform_p.h"
#include <platform/armemu.h>
#include <dev/display.h>
#include <lib/gfx.h>
#include <reg.h>

static int display_w, display_h;
static void *display_fb;

inline static int has_display(void)
{
	return *REG32(SYSINFO_FEATURES) & SYSINFO_FEATURE_DISPLAY;
}

void platform_init_display(void)
{
	if (!has_display())
		return;

	display_fb = (void *)DISPLAY_FRAMEBUFFER;
	display_w = *REG32(DISPLAY_WIDTH);
	display_h = *REG32(DISPLAY_HEIGHT);

	gfx_draw_pattern();
}

void display_get_info(struct display_info *info)
{
	if (!has_display())
		return;

	info->framebuffer = display_fb;
	info->format = GFX_FORMAT_RGB_x888;
	info->width = display_w;
	info->height = display_h;
	info->stride = display_w;
	info->flush = NULL;
}

