/*
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86/mtrr.h>
#include <dev/display.h>
#include <inttypes.h>
#include <kernel/vm.h>
#include <kernel/spinlock.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <malloc.h>
#include <platform/fb_console.h>
#include <platform/pc/font9x16.h>
#include <string.h>

#define DRAW_TEST_PATTERN 0

#define FONT_WIDTH  9
#define FONT_HEIGHT 16

static uint32_t display_w, display_h, display_p;
static void *display_fb;
static void *display_fb_region;
static bool display_initialized = false;
static uint32_t foreground_color = 0x00FFFFFF;
static uint32_t background_color = 0x00000000;
static spin_lock_t fb_console_lock = SPIN_LOCK_INITIAL_VALUE;

/* Double buffering for improved scroll performance */
static void *display_backbuffer = NULL;

static unsigned int curr_x;
static unsigned int curr_y;

static unsigned int console_cols;
static unsigned int console_rows;

static struct {
    unsigned int x1, y1, x2, y2;
} view_window;

/* Helper to get the current framebuffer pointer (real FB or backbuffer) */
static inline void *get_framebuffer_ptr(void) {
    return display_backbuffer;
}

static void place(unsigned int x, unsigned int y);
static void clear_char(unsigned int x, unsigned int y);
static void clear(void);
static void window(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2);
static void scroll(void);
static void draw_char(unsigned int x, unsigned int y, char c, uint32_t fg_color,
                      uint32_t bg_color);

static struct fb_console_boot_info pending_boot_info;
static bool have_pending_boot_info = false;

static void fb_console_stage_boot_info(const struct fb_console_boot_info *boot_info) {
    if (!boot_info) {
        return;
    }

    pending_boot_info = *boot_info;
    have_pending_boot_info = true;
}

/**
 * Flush the backbuffer to the real framebuffer.
 * Only copies the specified region (for efficiency).
 */
static void flush_backbuffer_region(unsigned int start_y, unsigned int end_y) {
    size_t start_offset = (size_t)start_y * (size_t)display_p;
    uint8_t *src = (uint8_t *)display_backbuffer + start_offset;
    uint8_t *dst = (uint8_t *)display_fb + start_offset;
    size_t bytes = (size_t)(end_y - start_y) * (size_t)display_p;

    memcpy(dst, src, bytes);
}

/**
 * Flush entire backbuffer to real framebuffer.
 */
static void flush_backbuffer_full(void) {
    memcpy(display_fb, display_backbuffer, (size_t)display_h * (size_t)display_p);
}

static void fb_console_init_common(const struct fb_console_boot_info *boot_info) {
    if (!boot_info) {
        TRACEF("No multiboot framebuffer info available\n");
        return;
    }

    // The caller guarantees RGB mode; only validate bpp here.
    if (boot_info->framebuffer_bpp != 32) {
        TRACEF("Unsupported bpp: %d (only 32bpp supported)\n", boot_info->framebuffer_bpp);
        return;
    }

    uint64_t fb_size = (uint64_t)boot_info->framebuffer_height * boot_info->framebuffer_pitch;
    if (fb_size == 0) {
        TRACEF("Invalid framebuffer size: 0\n");
        return;
    }

    paddr_t fb_phys_addr = (paddr_t)boot_info->framebuffer_addr;
    paddr_t fb_map_base = ROUNDDOWN(fb_phys_addr, PAGE_SIZE);
    size_t fb_map_offset = (size_t)(fb_phys_addr - fb_map_base);
    size_t fb_map_size = PAGE_ALIGN((size_t)fb_size + fb_map_offset);

    status_t status = vmm_alloc_physical(vmm_get_kernel_aspace(), "display_fb", fb_map_size,
                                         &display_fb_region, 0, fb_map_base,
                                         0, /* vmm_flags */
                                         ARCH_MMU_FLAG_CACHED);
    if (status != NO_ERROR) {
        TRACEF("Failed to map framebuffer at %#" PRIx64 " (size %#zx): %d\n",
               boot_info->framebuffer_addr, fb_map_size, status);
        return;
    }

    display_fb = (void *)((uint8_t *)display_fb_region + fb_map_offset);
    display_w = boot_info->framebuffer_width;
    display_h = boot_info->framebuffer_height;
    display_p = boot_info->framebuffer_pitch;

    if (display_w < FONT_WIDTH || display_h < FONT_HEIGHT || display_p == 0) {
        TRACEF("Invalid framebuffer geometry: %ux%u pitch %u\n", display_w, display_h,
               display_p);
        return;
    }

    // foreground_color = ((1 << framebuffer->framebuffer_blue_mask_size) - 1) <<
    // framebuffer->framebuffer_blue_field_position;

    console_cols = display_w / FONT_WIDTH;
    console_rows = display_h / FONT_HEIGHT;

    view_window.x1 = 0;
    view_window.y1 = 0;
    if (console_cols == 0 || console_rows == 0) {
        TRACEF("Framebuffer too small for %ux%u font: %ux%u\n", FONT_WIDTH, FONT_HEIGHT,
               display_w, display_h);
        return;
    }

    view_window.x2 = console_cols - 1;
    view_window.y2 = console_rows - 1;

    /* Configure MTRR for the framebuffer to improve write throughput. */
    uint64_t fb_mtrr_phys_addr = boot_info->framebuffer_addr;

    /* Round framebuffer size up to the next power of 2 (MTRR requirement). */
    uint64_t mtrr_size = 1;
    while (mtrr_size < fb_size) {
        mtrr_size <<= 1;
    }

    if (x86_mtrr_set_framebuffer(fb_mtrr_phys_addr, mtrr_size) == NO_ERROR) {
        TRACEF("Framebuffer MTRR configured: addr=%#" PRIx64 " size=%#" PRIx64 "\n",
               fb_mtrr_phys_addr, mtrr_size);
    } else {
        TRACEF("WARNING: Could not configure MTRR for framebuffer, performance may be degraded\n");
    }

    /* Allocate backbuffer the same size as the framebuffer using VM system. */
    size_t buffer_size = (size_t)display_h * (size_t)display_p;
    status_t backbuffer_status =
        vmm_alloc(vmm_get_kernel_aspace(), "display_backbuffer", buffer_size,
                  &display_backbuffer, 0, /* align_log2: 0 for default alignment */
                  0,                      /* vmm_flags: 0 for defaults */
                  ARCH_MMU_FLAG_CACHED);

        if (backbuffer_status != NO_ERROR) {
         TRACEF("ERROR: Failed to allocate backbuffer via VM (%zu bytes): %d\n", buffer_size,
             backbuffer_status);
         return;
    }

        memcpy(display_backbuffer, display_fb, buffer_size);
        TRACEF("Display backbuffer allocated via VM: %p (%zu bytes)\n", display_backbuffer,
            buffer_size);

        curr_x = 0;
        curr_y = 0;

        clear();

        display_initialized = true;

#if DRAW_TEST_PATTERN
    gfx_draw_pattern();
#endif
}

void fb_console_init(const struct fb_console_boot_info *boot_info) {
    if (!boot_info) {
        TRACEF("No framebuffer info available\n");
        return;
    }

    fb_console_stage_boot_info(boot_info);
}

void fb_console_init_postvm(void) {
    if (!display_initialized) {
        if (!have_pending_boot_info) {
            return;
        }

        fb_console_init_common(&pending_boot_info);
    }
}

bool fb_console_present(void) {
    return display_initialized;
}

static void draw_char(unsigned int x, unsigned int y, char c, uint32_t fg_color, uint32_t bg_color) {
    const uint16_t *bitmap = &font_9x16[(size_t)(unsigned char)c * FONT_HEIGHT];
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
            size_t pixel_offset = (size_t)pixel_y * (size_t)display_p + (size_t)pixel_x * 4u;
            uint32_t *pixel = (uint32_t *)(fb_bytes + pixel_offset);

            if (row_bits & (1 << col)) {
                *pixel = fg_color;
            } else {
                *pixel = bg_color;
            }
        }
    }
}

static void clear_char(unsigned int x, unsigned int y) {
    void *fb = get_framebuffer_ptr();
    uint8_t *fb_bytes = (uint8_t *)fb;

    for (int row = 0; row < FONT_HEIGHT; row++) {
        for (int col = 0; col < FONT_WIDTH; col++) {
            unsigned int pixel_x = x * FONT_WIDTH + col;
            unsigned int pixel_y = y * FONT_HEIGHT + row;

            if (pixel_x >= display_w || pixel_y >= display_h) {
                continue;
            }

            size_t pixel_offset = (size_t)pixel_y * (size_t)display_p + (size_t)pixel_x * 4u;
            uint32_t *pixel = (uint32_t *)(fb_bytes + pixel_offset);
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
    memset(fb_bytes, 0, (size_t)display_h * (size_t)display_p);
}

// Scrolling with double buffering support
static void scroll(void) {
    void *fb = get_framebuffer_ptr();
    uint8_t *fb_bytes = (uint8_t *)fb;
    unsigned int scroll_pixels = FONT_HEIGHT;

    unsigned int start_y = view_window.y1 * FONT_HEIGHT;
    unsigned int end_y = (view_window.y2 + 1) * FONT_HEIGHT;
    unsigned int width_bytes = (view_window.x2 - view_window.x1 + 1) * FONT_WIDTH * 4;
    unsigned int start_x_bytes = view_window.x1 * FONT_WIDTH * 4;

    size_t region_start_offset = (size_t)start_y * (size_t)display_p + (size_t)start_x_bytes;
    uint8_t *region_start = fb_bytes + region_start_offset;

    // move
    memmove(region_start,
            region_start + (size_t)scroll_pixels * (size_t)display_p,
            (size_t)(end_y - start_y - scroll_pixels) * (size_t)display_p);

    // cleanup
    unsigned int clear_start_y = start_y + (end_y - start_y - scroll_pixels);
    for (unsigned int y = clear_start_y; y < end_y && y < display_h; y++) {
        size_t line_offset = (size_t)y * (size_t)display_p + (size_t)start_x_bytes;
        memset(fb_bytes + line_offset, (int)background_color, (size_t)width_bytes);
    }

    /* Flush the scrolled region to the real framebuffer */
    flush_backbuffer_region(start_y, end_y);

    curr_y = view_window.y2;
}

void fb_console_dputc(char c) {
    if (!fb_console_present()) {
        return;
    }

    arch_interrupt_saved_state_t state = spin_lock_irqsave(&fb_console_lock);

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
                unsigned int char_y = curr_y * FONT_HEIGHT;
                flush_backbuffer_region(char_y, char_y + FONT_HEIGHT);
            }
            break;

        default:
            // draw
            draw_char(curr_x, curr_y, c, foreground_color, background_color);
            /* Flush drawn character to framebuffer for immediate visual feedback */
            unsigned int char_y = curr_y * FONT_HEIGHT;
            flush_backbuffer_region(char_y, char_y + FONT_HEIGHT);
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

    spin_unlock_irqrestore(&fb_console_lock, state);
}

