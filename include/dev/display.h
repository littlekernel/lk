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
#ifndef __DEV_DISPLAY_H
#define __DEV_DISPLAY_H

#include <stdbool.h>
#include <sys/types.h>
#include <inttypes.h>
#include <compiler.h>

__BEGIN_CDECLS

int display_init(void *framebuffer);
int display_enable(bool enable);
void display_pre_freq_change(void);
void display_post_freq_change(void);

#define DISPLAY_FORMAT_NONE         (-1)
#define DISPLAY_FORMAT_RGB_565      (0)
#define DISPLAY_FORMAT_RGB_332      (1)
#define DISPLAY_FORMAT_RGB_2220     (2)
#define DISPLAY_FORMAT_ARGB_8888    (3)
#define DISPLAY_FORMAT_RGB_x888     (4)
#define DISPLAY_FORMAT_RGB_x111     (5)
#define DISPLAY_FORMAT_MONO_1       (6)
#define DISPLAY_FORMAT_MONO_8       (7)

struct display_info {
    void *framebuffer;
    int format;
    uint width;
    uint height;
    uint stride;

    // Update function
    void (*flush)(uint starty, uint endy);
};

status_t display_get_info(struct display_info *info);

__END_CDECLS

#endif

