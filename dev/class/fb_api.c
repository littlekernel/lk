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

#include <err.h>
#include <dev/class/fb.h>

status_t class_fb_set_mode(struct device *dev, size_t width, size_t height, size_t bpp)
{
	struct fb_ops *ops = device_get_driver_ops(dev, struct fb_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->set_mode)
		return ops->set_mode(dev, width, height, bpp);
	else
		return ERR_NOT_SUPPORTED;
}

status_t class_fb_get_info(struct device *dev, struct fb_info *info)
{
	struct fb_ops *ops = device_get_driver_ops(dev, struct fb_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->get_info)
		return ops->get_info(dev, info);
	else
		return ERR_NOT_SUPPORTED;
}

status_t class_fb_update(struct device *dev)
{
	struct fb_ops *ops = device_get_driver_ops(dev, struct fb_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->update)
		return ops->update(dev);
	else
		return ERR_NOT_SUPPORTED;
}

status_t class_fb_update_region(struct device *dev, size_t x, size_t y, size_t width, size_t height)
{
	struct fb_ops *ops = device_get_driver_ops(dev, struct fb_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->update_region)
		return ops->update_region(dev, x, y, width, height);
	else
		return ERR_NOT_SUPPORTED;
}

