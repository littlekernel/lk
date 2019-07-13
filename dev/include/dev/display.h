/*
 * Copyright (c) 2008-2010 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <inttypes.h>
#include <lk/compiler.h>

__BEGIN_CDECLS

int display_init(void *framebuffer);
int display_enable(bool enable);
void display_pre_freq_change(void);
void display_post_freq_change(void);

enum display_format {
    DISPLAY_FORMAT_UNKNOWN,
    DISPLAY_FORMAT_MONO_1,
    DISPLAY_FORMAT_RGB_111,
    DISPLAY_FORMAT_RGB_565,
    DISPLAY_FORMAT_RGB_x888,
    DISPLAY_FORMAT_ARGB_8888,
};

enum image_format {
    IMAGE_FORMAT_UNKNOWN,
    IMAGE_FORMAT_MONO_1,
    IMAGE_FORMAT_MONO_8,
    IMAGE_FORMAT_RGB_x111,
    IMAGE_FORMAT_RGB_332,
    IMAGE_FORMAT_RGB_565,
    IMAGE_FORMAT_RGB_2220,
    IMAGE_FORMAT_RGB_x888,
    IMAGE_FORMAT_ARGB_8888,
};

struct display_info {
    enum display_format format;
    uint width;
    uint height;
};

status_t display_get_info(struct display_info *info) __NONNULL((1));

struct display_image {
    enum image_format format;
    void *pixels;
    uint width;
    uint height;
    int stride; // row length in pixels
    int rowbytes; // row length in bytes
};

status_t display_present(struct display_image *image, uint starty, uint endy)
__NONNULL((1));

struct display_framebuffer {
    enum display_format format;
    struct display_image image;
    // Update function
    void (*flush)(uint starty, uint endy);
};

status_t display_get_framebuffer(struct display_framebuffer *fb)
__NONNULL((1));

__END_CDECLS

