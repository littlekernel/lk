/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86.h>
#include <assert.h>
#include <platform/pc.h>
#include <platform/vga_console.h>
#include <string.h>

/*
 * 80x25 text mode console.
 *
 * Under a hypervisor every access to text memory and every CRTC port write is a
 * trap, so the console never reads text memory back and keeps writes to a
 * minimum: the screen contents live in a RAM copy, scrolling moves the CRTC start
 * address down through the 32KB text window instead of copying the screen, and
 * only the newly exposed row is written. When the window runs out the screen is
 * rewritten from the RAM copy at the top of the window. The hardware cursor follows
 * at each newline and when vga_console_sync_cursor() is called, not on every
 * character.
 */

/* text memory window, in character cells */
#define VRAM       ((volatile uint16_t *)(0xB8000U + KERNEL_ASPACE_BASE))
#define VRAM_CELLS (0x8000 / 2)

#define COLS        80
#define ROWS        25
#define CELLS       (COLS * ROWS)
#define TAB_WIDTH   8
#define ATTR        0x07
#define BLANK       ((ATTR << 8) | ' ')

/* CRTC registers */
#define CRTC_START_HI  0x0C
#define CRTC_START_LO  0x0D
#define CRTC_CURSOR_HI 0x0E
#define CRTC_CURSOR_LO 0x0F

/* the RAM copy of the visible screen */
static uint16_t screen[CELLS];

/* cell offset in text memory of the top row, programmed into the CRTC start address */
static unsigned int origin;
static unsigned int curr_x;
static unsigned int curr_y;
static bool cursor_dirty;
static bool console_initialized = false;

/* the stores to text memory are unsigned long sized, which a row is a multiple of */
static_assert((COLS * sizeof(uint16_t)) % sizeof(unsigned long) == 0, "");

/* index and data in one 16 bit write, one trap instead of two */
static void crtc_write16(uint8_t reg_hi, uint8_t reg_lo, uint16_t val) {
    outpw(CGA_INDEX_REG, (uint16_t)(reg_hi | (val & 0xff00)));
    outpw(CGA_INDEX_REG, (uint16_t)(reg_lo | (val << 8)));
}

/* copy screen cells [first, first + count) to text memory, count a multiple of a row */
static void vram_copy(unsigned int first, unsigned int count) {
    volatile unsigned long *dst = (volatile unsigned long *)(VRAM + origin + first);
    const uint16_t *src = screen + first;
    for (size_t i = 0; i < count * sizeof(uint16_t) / sizeof(unsigned long); i++) {
        unsigned long w;
        memcpy(&w, src + i * (sizeof(w) / sizeof(*src)), sizeof(w));
        dst[i] = w;
    }
}

static void update_cursor(void) {
    crtc_write16(CRTC_CURSOR_HI, CRTC_CURSOR_LO, (uint16_t)(origin + curr_y * COLS + curr_x));
    cursor_dirty = false;
}

static void scroll(void) {
    memmove(screen, screen + COLS, (CELLS - COLS) * sizeof(uint16_t));
    for (unsigned int i = CELLS - COLS; i < CELLS; i++) {
        screen[i] = BLANK;
    }

    if (origin + COLS + CELLS <= VRAM_CELLS) {
        origin += COLS;
        vram_copy(CELLS - COLS, COLS);
    } else {
        /* the window is off screen at the top, so redrawing there is not visible */
        origin = 0;
        vram_copy(0, CELLS);
    }
    crtc_write16(CRTC_START_HI, CRTC_START_LO, (uint16_t)origin);
}

static void newline(void) {
    if (curr_y == ROWS - 1) {
        scroll();
    } else {
        curr_y++;
    }
}

static void put_cell(char c) {
    const unsigned int i = curr_y * COLS + curr_x;
    const uint16_t cell = (uint16_t)((ATTR << 8) | (unsigned char)c);
    screen[i] = cell;
    VRAM[origin + i] = cell;
}

void vga_console_init(void) {
    for (unsigned int i = 0; i < CELLS; i++) {
        screen[i] = BLANK;
    }
    origin = 0;
    curr_x = curr_y = 0;
    vram_copy(0, CELLS);
    crtc_write16(CRTC_START_HI, CRTC_START_LO, 0);
    update_cursor();
    console_initialized = true;
}

bool vga_console_present(void) {
    return console_initialized;
}

void vga_console_sync_cursor(void) {
    if (cursor_dirty) {
        update_cursor();
    }
}

void vga_console_putc(char c) {
    switch (c) {
        case '\t':
            curr_x = (curr_x + TAB_WIDTH) & ~(TAB_WIDTH - 1);
            if (curr_x >= COLS) {
                curr_x = 0;
                newline();
            }
            break;
        case '\r':
            curr_x = 0;
            break;
        case '\n':
            newline();
            update_cursor();
            return;
        case '\b':
            if (curr_x > 0) {
                curr_x--;
                put_cell(' ');
            }
            break;
        default:
            put_cell(c);
            if (++curr_x == COLS) {
                curr_x = 0;
                newline();
            }
            break;
    }

    cursor_dirty = true;
}
