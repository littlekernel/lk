/*
 * Copyright (c) 2013 Corey Tabaka
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
#ifndef __DEV_CLASS_FB_H
#define __DEV_CLASS_FB_H

#include <compiler.h>
#include <dev/driver.h>

struct fb_info {
	void *addr;
	size_t line_width;
	size_t width;
	size_t height;
	size_t bpp;
};

/* fb interface */
struct fb_ops {
	struct driver_ops std;

	status_t (*set_mode)(struct device *dev, size_t width, size_t height, size_t bpp);
	status_t (*get_info)(struct device *dev, struct fb_info *info);
	status_t (*update)(struct device *dev);
	status_t (*update_region)(struct device *dev, size_t x, size_t y, size_t width, size_t height);
};

__BEGIN_CDECLS

status_t class_fb_set_mode(struct device *dev, size_t width, size_t height, size_t bpp);
status_t class_fb_get_info(struct device *dev, struct fb_info *info);
status_t class_fb_update(struct device *dev);
status_t class_fb_update_region(struct device *dev, size_t x, size_t y, size_t width, size_t height);

__END_CDECLS

#endif


