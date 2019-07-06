/*
 * Copyright (c) 2010 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <platform.h>
#include "platform_p.h"
#include <platform/armemu.h>
#include <dev/display.h>
#include <lib/gfx.h>
#include <lk/reg.h>
#include <assert.h>

#define DRAW_TEST_PATTERN 0

static int display_w, display_h;
static void *display_fb;

inline static int has_display(void) {
    return *REG32(SYSINFO_FEATURES) & SYSINFO_FEATURE_DISPLAY;
}

void platform_init_display(void) {
    if (!has_display())
        return;

    display_fb = (void *)DISPLAY_FRAMEBUFFER;
    display_w = *REG32(DISPLAY_WIDTH);
    display_h = *REG32(DISPLAY_HEIGHT);

#if DRAW_TEST_PATTERN
    gfx_draw_pattern();
#endif
}

status_t display_get_framebuffer(struct display_framebuffer *fb) {
    DEBUG_ASSERT(fb);
    if (!has_display())
        return ERR_NOT_FOUND;

    fb->image.format = IMAGE_FORMAT_RGB_x888;
    fb->image.pixels = display_fb;
    fb->image.width = display_w;
    fb->image.height = display_h;
    fb->image.stride = display_w;
    fb->image.rowbytes = display_w * 4;
    fb->flush = NULL;
    fb->format = DISPLAY_FORMAT_RGB_x888;

    return NO_ERROR;
}

status_t display_get_info(struct display_info *info) {
    DEBUG_ASSERT(info);
    if (!has_display())
        return ERR_NOT_FOUND;

    info->format = DISPLAY_FORMAT_RGB_x888;
    info->width = display_w;
    info->height = display_h;

    return NO_ERROR;
}

status_t display_present(struct display_image *image, uint starty, uint endy) {
    TRACEF("display_present - not implemented");
    DEBUG_ASSERT(false);
    return NO_ERROR;
}
