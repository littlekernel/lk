/*
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86/mtrr.h>
#include <assert.h>
#include <dev/display.h>
#include <hw/multiboot.h>
#include <inttypes.h>
#include <kernel/vm.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/trace.h>
#include <malloc.h>
#include <platform/display.h>
#include <platform/pc/font9x16.h>
#include <string.h>

#define DRAW_TEST_PATTERN 0

#define FONT_WIDTH  9
#define FONT_HEIGHT 16

static uint32_t display_w, display_h, display_p;
static void *display_fb;
static bool display_initialized = false;
static uint32_t foreground_color = 0x00FFFFFF;
static uint32_t background_color = 0x00000000;

/* Double buffering for improved scroll performance */
static void *display_backbuffer = NULL;
static bool use_backbuffer = false;

static unsigned int curr_x;
static unsigned int curr_y;

static unsigned int console_cols;
static unsigned int console_rows;

static struct {
    unsigned int x1, y1, x2, y2;
} view_window;

/* Helper to get the current framebuffer pointer (real FB or backbuffer) */
static inline void *get_framebuffer_ptr(void) {
    return use_backbuffer ? display_backbuffer : display_fb;
}

static void place(unsigned int x, unsigned int y);
static void clear_char(int x, int y);
static void clear(void);
static void window(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
static void scroll(void);

/**
 * Flush the backbuffer to the real framebuffer.
 * Only copies the specified region (for efficiency).
 */
static void flush_backbuffer_region(unsigned int start_y, unsigned int end_y) {
    if (!use_backbuffer) {
        return; // Not using backbuffer, no need to flush
    }

    uint8_t *src = (uint8_t *)display_backbuffer + start_y * display_p;
    uint8_t *dst = (uint8_t *)display_fb + start_y * display_p;
    size_t bytes = (end_y - start_y) * display_p;

    memcpy(dst, src, bytes);
}

/**
 * Flush entire backbuffer to real framebuffer.
 */
static void flush_backbuffer_full(void) {
    if (!use_backbuffer) {
        return;
    }

    memcpy(display_fb, display_backbuffer, display_h * display_p);
}

/**
 * Post-VM initialization to allocate double buffer.
 * This hook is called after virtual memory is available,
 * allowing us to allocate large buffers via the VM system.
 */
static void display_init_backbuffer(uint level) {
    if (!has_display()) {
        return;
    }

    /* Allocate backbuffer the same size as the framebuffer using VM system */
    size_t buffer_size = display_h * display_p;

    status_t status = vmm_alloc(vmm_get_kernel_aspace(),
                                "display_backbuffer",
                                buffer_size,
                                &display_backbuffer,
                                0, /* align_log2: 0 for default alignment */
                                0, /* vmm_flags: 0 for defaults */
                                ARCH_MMU_FLAG_CACHED);

    if (status != NO_ERROR) {
        TRACEF("WARNING: Failed to allocate backbuffer via VM (%zu bytes), using direct rendering\n", buffer_size);
        return;
    }

    /* Initialize backbuffer with same state as framebuffer */
    memcpy(display_backbuffer, display_fb, buffer_size);
    use_backbuffer = true;

    TRACEF("Display backbuffer allocated via VM: %p (%zu bytes)\n", display_backbuffer, buffer_size);
}

void platform_init_display(struct multiboot2_tag_framebuffer *framebuffer) {
    // Check if we have multiboot framebuffer info
    if (!framebuffer) {
        TRACEF("No multiboot framebuffer info available\n");
        return;
    }

    // Only support RGB mode with 32bpp for now
    if (framebuffer->common.framebuffer_type != 1) { // 1 = RGB color
        TRACEF("Unsupported framebuffer type: %d\n", framebuffer->common.framebuffer_type);
        return;
    }

    if (framebuffer->common.framebuffer_bpp != 32) {
        TRACEF("Unsupported bpp: %d (only 32bpp supported)\n", framebuffer->common.framebuffer_bpp);
        return;
    }

    display_fb = (void *)(uintptr_t)framebuffer->common.framebuffer_addr + KERNEL_ASPACE_BASE;
    display_w = framebuffer->common.framebuffer_width;
    display_h = framebuffer->common.framebuffer_height;
    display_p = framebuffer->common.framebuffer_pitch;

    // foreground_color = ((1 << framebuffer->framebuffer_blue_mask_size) - 1) << framebuffer->framebuffer_blue_field_position;

    console_cols = display_w / FONT_WIDTH;
    console_rows = display_h / FONT_HEIGHT;

    view_window.x1 = 0;
    view_window.y1 = 0;
    view_window.x2 = console_cols - 1;
    view_window.y2 = console_rows - 1;

    curr_x = 0;
    curr_y = 0;

    clear();

    display_initialized = true;

    /* Configure MTRR for the framebuffer to improve performance on real hardware.
     * Write-Combining (WC) caches write operations to memory but combines writes into
     * larger transactions, significantly improving framebuffer write performance.
     */
    uint64_t fb_phys_addr = framebuffer->common.framebuffer_addr;
    uint64_t fb_size = (uint64_t)display_h * display_p;

    /* Round framebuffer size up to the next power of 2 (MTRR requirement) */
    uint64_t mtrr_size = 1;
    while (mtrr_size < fb_size) {
        mtrr_size <<= 1;
    }

    if (x86_mtrr_set_framebuffer(fb_phys_addr, mtrr_size) == NO_ERROR) {
        TRACEF("Framebuffer MTRR configured: addr=%#" PRIx64 " size=%#" PRIx64 "\n",
               fb_phys_addr, mtrr_size);
    } else {
        TRACEF("WARNING: Could not configure MTRR for framebuffer, performance may be degraded\n");
    }

#if DRAW_TEST_PATTERN
    gfx_draw_pattern();
#endif
}

bool has_display(void) {
    return display_initialized;
}

status_t display_get_framebuffer(struct display_framebuffer *fb) {
    // DEBUG_ASSERT(fb);
    if (!has_display()) {
        return ERR_NOT_FOUND;
    }

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
    if (!has_display()) {
        return ERR_NOT_FOUND;
    }

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

void draw_char(int x, int y, char c, uint32_t fg_color, uint32_t bg_color) {
    const uint16_t *bitmap = &font_9x16[(unsigned char)c * FONT_HEIGHT];
    void *fb = get_framebuffer_ptr();

    for (int row = 0; row < FONT_HEIGHT; row++) {
        unsigned char row_bits = bitmap[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            unsigned int pixel_x = x * FONT_WIDTH + col;
            unsigned int pixel_y = y * FONT_HEIGHT + row;

            if (pixel_x >= display_w || pixel_y >= display_h) {
                continue;
            }

            uint8_t *fb_bytes = (uint8_t *)fb;
            uint32_t *pixel = (uint32_t *)(fb_bytes + pixel_y * display_p + pixel_x * 4);

            if (row_bits & (1 << col)) {
                *pixel = fg_color;
            } else {
                *pixel = bg_color;
            }
        }
    }
}

static void clear_char(int x, int y) {
    void *fb = get_framebuffer_ptr();
    uintptr_t *fb_ptr = (uintptr_t *)fb;

    for (int row = 0; row < FONT_HEIGHT; row++) {
        for (int col = 0; col < FONT_WIDTH; col++) {
            unsigned int pixel_x = x * FONT_WIDTH + col;
            unsigned int pixel_y = y * FONT_HEIGHT + row;

            if (pixel_x >= display_w || pixel_y >= display_h) {
                continue;
            }

            uint32_t *pixel = (uint32_t *)(fb_ptr + pixel_y * display_p + pixel_x * 4);
            *pixel = background_color;
        }
    }
}

static void place(unsigned int x, unsigned int y) {
    if (x >= console_cols) {
        x = console_cols - 1;
    }
    if (y >= console_rows) {
        y = console_rows - 1;
    }

    curr_x = x;
    curr_y = y;
}

static void window(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) {
    view_window.x1 = x1;
    view_window.y1 = y1;
    view_window.x2 = x2;
    view_window.y2 = y2;

    if (curr_x < x1) {
        curr_x = x1;
    }
    if (curr_x > x2) {
        curr_x = x2;
    }
    if (curr_y < y1) {
        curr_y = y1;
    }
    if (curr_y > y2) {
        curr_y = y2;
    }
}

static void clear(void) {
    void *fb = get_framebuffer_ptr();
    uint8_t *fb_bytes = (uint8_t *)fb;
    memset(fb_bytes, 0, display_h * display_p);
}

// Scrolling with double buffering support
static void scroll(void) {
    void *fb = get_framebuffer_ptr();
    uint8_t *fb_bytes = (uint8_t *)fb;
    unsigned int scroll_pixels = FONT_HEIGHT;

    unsigned int start_y = view_window.y1 * FONT_HEIGHT;
    unsigned int end_y = (view_window.y2 + 1) * FONT_HEIGHT;
    unsigned int width_bytes = (view_window.x2 - view_window.x1 + 1) * FONT_WIDTH * 4;
    unsigned int start_x_bytes = view_window.x1 * FONT_WIDTH * 8;

    uint8_t *region_start = fb_bytes + start_y * display_p + start_x_bytes;

    // move
    memmove(region_start,
            region_start + scroll_pixels * display_p,
            (end_y - start_y - scroll_pixels) * display_p);

    // cleanup
    unsigned int clear_start_y = start_y + (end_y - start_y - scroll_pixels);
    for (unsigned int y = clear_start_y; y < end_y && y < display_h; y++) {
        memset(fb_bytes + y * display_p + start_x_bytes, background_color, width_bytes);
    }

    /* Flush the scrolled region to the real framebuffer */
    if (use_backbuffer) {
        flush_backbuffer_region(start_y, end_y);
    }

    curr_y = view_window.y2;
}

void dputc(char c) {
    if (!has_display()) {
        return;
    }
    switch (c) {
        case '\t':
            curr_x = (curr_x + 8) & ~7; // Move to next tab
            break;

        case '\r':
            curr_x = view_window.x1;
            break;

        case '\n':
            curr_y++;
            curr_x = view_window.x1;
            break;

        case '\b':
            if (curr_x > view_window.x1) {
                curr_x--;
                clear_char(curr_x, curr_y);
                /* Flush cleared character to framebuffer */
                if (use_backbuffer) {
                    unsigned int char_y = curr_y * FONT_HEIGHT;
                    flush_backbuffer_region(char_y, char_y + FONT_HEIGHT);
                }
            }
            break;

        default:
            // draw
            draw_char(curr_x, curr_y, c, foreground_color, background_color);
            /* Flush drawn character to framebuffer for immediate visual feedback */
            if (use_backbuffer) {
                unsigned int char_y = curr_y * FONT_HEIGHT;
                flush_backbuffer_region(char_y, char_y + FONT_HEIGHT);
            }
            curr_x++;
            break;
    }

    if (curr_x > view_window.x2) {
        curr_x = view_window.x1;
        curr_y++;
    }

    if (curr_y > view_window.y2) {
        scroll();
        curr_y = view_window.y2;
    }
}

/* Register post-VM initialization to allocate backbuffer */
LK_INIT_HOOK(display_init_backbuffer, display_init_backbuffer, LK_INIT_LEVEL_VM + 1);
