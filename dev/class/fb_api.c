/*
 * Copyright (c) 2013 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <dev/class/fb.h>

status_t class_fb_set_mode(struct device *dev, size_t width, size_t height, size_t bpp) {
    struct fb_ops *ops = device_get_driver_ops(dev, struct fb_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->set_mode)
        return ops->set_mode(dev, width, height, bpp);
    else
        return ERR_NOT_SUPPORTED;
}

status_t class_fb_get_info(struct device *dev, struct fb_info *info) {
    struct fb_ops *ops = device_get_driver_ops(dev, struct fb_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->get_info)
        return ops->get_info(dev, info);
    else
        return ERR_NOT_SUPPORTED;
}

status_t class_fb_update(struct device *dev) {
    struct fb_ops *ops = device_get_driver_ops(dev, struct fb_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->update)
        return ops->update(dev);
    else
        return ERR_NOT_SUPPORTED;
}

status_t class_fb_update_region(struct device *dev, size_t x, size_t y, size_t width, size_t height) {
    struct fb_ops *ops = device_get_driver_ops(dev, struct fb_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->update_region)
        return ops->update_region(dev, x, y, width, height);
    else
        return ERR_NOT_SUPPORTED;
}

