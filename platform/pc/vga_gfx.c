/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// 256 colour graphics modes on a VGA, and on a Tseng ET4000 the larger ones that need
// its banking. Drawing goes to a copy of the screen in RAM, handed out through
// display_get_framebuffer() as an RGB 3:3:2 image so lib/gfx can draw on it; the DAC is
// loaded with the matching 3:3:2 palette, and flushing copies rows out through the 64KB
// window a bank at a time.
//
// Text mode is saved on the way into graphics and put back on the way out: the
// registers, the DAC and the font in plane 2, which the graphics modes overwrite. The
// text console is suspended meanwhile and redraws itself when resumed.

#include <dev/display.h>
#include <kernel/thread.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <platform.h>
#include <platform/fb_console.h>
#include <platform/vga_console.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform_p.h"
#include "vga_p.h"

struct vga_mode {
    const char *name;
    uint width, height;
    bool et4000;    // needs the ET4000's banking
    uint memory_kb; // display memory it needs
    uint8_t crtc35; // ET4000 overflow high (interlace)
    struct vga_regs regs;
};

// From the ET4000 data book's operation mode tables (p95-96): modes 13, 2E and 30.
// Mode 13 is the standard VGA one. 800x600 uses clock select 3, which the data book
// recommends wiring to 40MHz.
#define PALETTE_IDENTITY                                                                           \
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
// clang-format off
static const struct vga_mode modes[] = {
    {
        .name = "320x200", .width = 320, .height = 200, .et4000 = false, .memory_kb = 64,
        .regs = {
            .misc = 0x63,
            .ts = { 0x03, 0x01, 0x0f, 0x00, 0x0e },
            .crtc = { 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0xbf, 0x1f, 0x00, 0x41, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x9c, 0x8e, 0x8f, 0x28, 0x40, 0x96, 0xb9, 0xa3,
                      0xff },
            .gdc = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff },
            .atc = { PALETTE_IDENTITY, 0x41, 0x00, 0x0f, 0x00, 0x00 },
        },
    },
    {
        .name = "640x480", .width = 640, .height = 480, .et4000 = true, .memory_kb = 512,
        .regs = {
            .misc = 0xe3,
            .ts = { 0x03, 0x01, 0x0f, 0x00, 0x0e },
            .crtc = { 0x5f, 0x4f, 0x50, 0x82, 0x54, 0x80, 0x0b, 0x3e, 0x00, 0x40, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0xea, 0x8c, 0xdf, 0x50, 0x60, 0xe7, 0x04, 0xab,
                      0xff },
            .gdc = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff },
            .atc = { PALETTE_IDENTITY, 0x01, 0x00, 0x0f, 0x00, 0x00 },
        },
    },
    {
        .name = "800x600", .width = 800, .height = 600, .et4000 = true, .memory_kb = 512,
        .regs = {
            .misc = 0xef,
            .ts = { 0x03, 0x01, 0x0f, 0x00, 0x0e },
            .crtc = { 0x7f, 0x63, 0x64, 0x02, 0x64, 0x17, 0x77, 0xf0, 0x00, 0x60, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x60, 0x82, 0x57, 0x64, 0x60, 0x5b, 0x75, 0xab,
                      0xff },
            .gdc = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff },
            .atc = { PALETTE_IDENTITY, 0x01, 0x00, 0x0f, 0x00, 0x00 },
        },
    },
    {
        // mode 38 interlaced, about 44.9MHz on clock select 3 per the table; the board decides
        .name = "1024x768i", .width = 1024, .height = 768, .et4000 = true, .memory_kb = 1024,
        .crtc35 = 0x80,
        .regs = {
            .misc = 0x2f,
            .ts = { 0x03, 0x01, 0x0f, 0x00, 0x0e },
            .crtc = { 0x99, 0x7f, 0x7f, 0x1d, 0x83, 0x17, 0x2f, 0xf5, 0x00, 0x60, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x08, 0x80, 0xff, 0x80, 0x60, 0xff, 0x30, 0xab,
                      0xff },
            .gdc = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff },
            .atc = { PALETTE_IDENTITY, 0x01, 0x00, 0x0f, 0x00, 0x00 },
        },
    },
    {
        // mode 38 non interlaced, needs about 65MHz
        .name = "1024x768", .width = 1024, .height = 768, .et4000 = true, .memory_kb = 1024,
        .regs = {
            .misc = 0x3f,
            .ts = { 0x03, 0x01, 0x0f, 0x00, 0x0e },
            .crtc = { 0xa1, 0x7f, 0x80, 0x04, 0x88, 0x9e, 0x26, 0xfd, 0x00, 0x60, 0x00, 0x00,
                      0x00, 0x00, 0x00, 0x00, 0x08, 0x8a, 0xff, 0x80, 0x60, 0x04, 0x22, 0xab,
                      0xff },
            .gdc = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0f, 0xff },
            .atc = { PALETTE_IDENTITY, 0x01, 0x00, 0x0f, 0x00, 0x00 },
        },
    },
};
// clang-format on

#define FONT_PLANE_SIZE VGA_WINDOW_SIZE

static bool vga_found;
static bool et4000_found;
static uint et4000_kb;

// text mode as it was before the first graphics mode
static struct {
    struct vga_regs regs;
    struct et4000_regs ext;
    uint8_t dac[256 * 3];
    uint8_t *font;
} text;

static const struct vga_mode *cur_mode;
static uint8_t *shadow;

// A VGA's graphics controller registers read back; an EGA's are write only, and on
// anything older the port floats.
static bool vga_detect(void) {
    const uint8_t old = vga_gdc_read(8);
    vga_gdc_write(8, 0x55);
    const bool r55 = vga_gdc_read(8) == 0x55;
    vga_gdc_write(8, 0xaa);
    const bool raa = vga_gdc_read(8) == 0xaa;
    vga_gdc_write(8, old);
    return r55 && raa;
}

void pc_vga_probe(void) {
    vga_found = vga_detect();
    if (!vga_found) {
        return;
    }
    et4000_found = et4000_probe();
    if (et4000_found) {
        et4000_kb = et4000_memory_kb();
    }
}

void pc_vga_report(void) {
    if (et4000_found) {
        printf("LEGACY: vga: Tseng ET4000, %uKB\n", et4000_kb);
    } else if (vga_found) {
        printf("LEGACY: vga: standard VGA\n");
    }
}

static const struct vga_mode *find_mode(const char *name) {
    for (uint i = 0; i < countof(modes); i++) {
        if (!strcmp(modes[i].name, name)) {
            return &modes[i];
        }
    }
    return NULL;
}

static bool mode_usable(const struct vga_mode *m) {
    return vga_found && (!m->et4000 || (et4000_found && et4000_kb >= m->memory_kb));
}

static void flush_rows(uint starty, uint endy) {
    if (!cur_mode || !shadow || starty > endy || starty >= cur_mode->height) {
        return;
    }
    if (endy >= cur_mode->height) {
        endy = cur_mode->height - 1;
    }

    volatile uint8_t *win = vga_window();
    size_t off = (size_t)starty * cur_mode->width;
    const size_t end = ((size_t)endy + 1) * cur_mode->width;
    while (off < end) {
        const size_t in_bank = off % VGA_WINDOW_SIZE;
        const size_t len = MIN(end - off, VGA_WINDOW_SIZE - in_bank);
        if (et4000_found) {
            et4000_set_bank(off / VGA_WINDOW_SIZE);
        }
        memcpy((void *)(win + in_bank), shadow + off, len);
        off += len;
    }
}

// RGB 3:3:2, the layout lib/gfx's GFX_FORMAT_RGB_332 writes, in 6 bit DAC components
static void load_rgb332_palette(void) {
    uint8_t dac[256 * 3];
    for (uint i = 0; i < 256; i++) {
        dac[i * 3 + 0] = (uint8_t)(((i >> 5) & 7) * 63 / 7);
        dac[i * 3 + 1] = (uint8_t)(((i >> 2) & 7) * 63 / 7);
        dac[i * 3 + 2] = (uint8_t)((i & 3) * 21);
    }
    vga_dac_write(0, 256, dac);
}

static status_t save_text(void) {
    text.font = malloc(FONT_PLANE_SIZE);
    if (!text.font) {
        return ERR_NO_MEMORY;
    }
    vga_console_suspend();
    vga_read_regs(&text.regs);
    if (et4000_found) {
        et4000_read_regs(&text.ext);
        et4000_set_bank(0);
    }
    vga_dac_read(0, 256, text.dac);
    vga_plane_read(2, text.font, FONT_PLANE_SIZE);
    return NO_ERROR;
}

static void restore_text(void) {
    vga_write_regs(&text.regs);
    if (et4000_found) {
        et4000_write_regs(&text.ext);
    }
    vga_plane_write(2, text.font, FONT_PLANE_SIZE);
    vga_dac_write(0, 256, text.dac);
    free(text.font);
    text.font = NULL;
    vga_console_resume();
}

static void set_text(void);

static status_t set_mode(const struct vga_mode *m) {
    if (!mode_usable(m)) {
        return ERR_NOT_SUPPORTED;
    }
    if (fb_console_present() || !vga_console_present()) {
        return ERR_NOT_SUPPORTED; // only from the VGA text console
    }

    // the old mode's copy goes first: two large ones need not fit in memory together
    if (cur_mode) {
        free(shadow);
        shadow = NULL;
    }
    uint8_t *new_shadow = calloc(1, (size_t)m->width * m->height);
    if (!new_shadow) {
        set_text();
        return ERR_NO_MEMORY;
    }
    if (!cur_mode) {
        status_t err = save_text();
        if (err < 0) {
            free(new_shadow);
            return err;
        }
    }

    vga_write_regs(&m->regs);
    if (et4000_found) {
        struct et4000_regs ext = {
            .ts6 = 0x00,
            .ts7 = 0xbc,
            .crtc31 = text.ext.crtc31 & ~0x40, // clock select 3 off
            .crtc33 = 0x00,
            .crtc34 = text.ext.crtc34 & ~0x02, // clock select 2 off: Misc Output picks the clock
            .crtc35 = m->crtc35,
            .atc16 = 0x00,
            .segment = 0x00,
        };
        et4000_write_regs(&ext);
    }
    load_rgb332_palette();

    shadow = new_shadow;
    cur_mode = m;

    // clear all of display memory the mode shows
    flush_rows(0, m->height - 1);
    return NO_ERROR;
}

static void set_text(void) {
    if (!cur_mode) {
        return;
    }
    cur_mode = NULL;
    restore_text();
    free(shadow);
    shadow = NULL;
}

status_t display_get_framebuffer(struct display_framebuffer *fb) {
    if (!cur_mode) {
        return ERR_NOT_READY;
    }
    memset(fb, 0, sizeof(*fb));
    fb->format = DISPLAY_FORMAT_UNKNOWN;
    fb->image.format = IMAGE_FORMAT_RGB_332;
    fb->image.pixels = shadow;
    fb->image.width = cur_mode->width;
    fb->image.height = cur_mode->height;
    fb->image.stride = (int)cur_mode->width;
    fb->image.rowbytes = (int)cur_mode->width;
    fb->flush = flush_rows;
    return NO_ERROR;
}

static void dump_regs(void) {
    struct vga_regs r;
    vga_read_regs(&r);
    printf("misc %02x\nts  ", r.misc);
    for (uint i = 0; i < VGA_TS_COUNT; i++) {
        printf(" %02x", r.ts[i]);
    }
    printf("\ncrtc");
    for (uint i = 0; i < VGA_CRTC_COUNT; i++) {
        printf(" %02x", r.crtc[i]);
    }
    printf("\ngdc ");
    for (uint i = 0; i < VGA_GDC_COUNT; i++) {
        printf(" %02x", r.gdc[i]);
    }
    printf("\natc ");
    for (uint i = 0; i < VGA_ATC_COUNT; i++) {
        printf(" %02x", r.atc[i]);
    }
    printf("\n");
    if (et4000_found) {
        struct et4000_regs e;
        et4000_read_regs(&e);
        printf("et4000: ts6 %02x ts7 %02x atc16 %02x segment %02x, crtc 30-37", e.ts6, e.ts7,
               e.atc16, e.segment);
        for (uint i = 0x30; i <= 0x37; i++) {
            printf(" %02x", vga_crtc_read(i));
        }
        printf("\n");
    }
}

// read, or write then read back, one indexed register
static int reg_command(const char *group, uint index, bool write, uint8_t val) {
    uint8_t (*rd)(uint8_t);
    void (*wr)(uint8_t, uint8_t);
    if (!strcmp(group, "ts")) {
        rd = vga_ts_read, wr = vga_ts_write;
    } else if (!strcmp(group, "crtc")) {
        rd = vga_crtc_read, wr = vga_crtc_write;
    } else if (!strcmp(group, "gdc")) {
        rd = vga_gdc_read, wr = vga_gdc_write;
    } else if (!strcmp(group, "atc")) {
        rd = vga_atc_read, wr = vga_atc_write;
    } else {
        printf("register group is ts, crtc, gdc or atc\n");
        return -1;
    }
    if (write) {
        wr((uint8_t)index, val);
    }
    printf("%s %#x = %#04x\n", group, index, rd((uint8_t)index));
    return 0;
}

// Rising edges of vertical retrace over ms milliseconds. The timer interrupt can hide a
// short retrace from the polling, so the caller stretches the sync pulse first.
static uint count_vsyncs(uint ms) {
    const uint16_t status = vga_crtc_port() + 6;
    const lk_time_t end = current_time() + ms;
    bool prev = inp(status) & 8;
    uint n = 0;
    while (current_time() < end) {
        const bool now = inp(status) & 8;
        if (now && !prev) {
            n++;
        }
        prev = now;
    }
    return n;
}

// Dots per frame of the timings in the CRTC now, for turning a frame rate into a clock.
static uint64_t dots_per_frame(void) {
    const uint8_t c0 = vga_crtc_read(0x00), c6 = vga_crtc_read(0x06), c7 = vga_crtc_read(0x07);
    uint dots = (vga_ts_read(1) & 1) ? 8 : 9;
    if (vga_ts_read(1) & 0x08) {
        dots *= 2; // dot clock halved
    }
    uint vt = c6 | (c7 & 0x01) << 8 | (c7 & 0x20) << 4;
    if (et4000_found && (vga_crtc_read(0x35) & 0x02)) {
        vt |= 1u << 10;
    }
    return (uint64_t)(c0 + 5) * dots * (vt + 2);
}

// Measure what each clock select runs at, using the current mode's timings.
static void measure_clocks(uint ms) {
    const uint orig = et4000_get_clock();
    const uint64_t dots = dots_per_frame();

    // a 16 line sync pulse, long enough to outlast the timer interrupt
    const uint8_t c11 = vga_crtc_read(0x11);
    vga_crtc_write(0x11, (uint8_t)((c11 & 0xf0) | ((vga_crtc_read(0x10) + 15) & 0x0f)));

    printf("%llu dots per frame, %u ms per clock\n", (unsigned long long)dots, ms);
    for (uint sel = 0; sel < 16; sel++) {
        et4000_set_clock(sel);
        thread_sleep(100);
        const uint frames = count_vsyncs(ms);
        const uint64_t hz = (uint64_t)frames * dots * 1000 / ms;
        printf("clock %2u: %3u frames, %3llu.%03llu MHz\n", sel, frames,
               (unsigned long long)(hz / 1000000), (unsigned long long)(hz / 1000 % 1000));
    }

    et4000_set_clock(orig);
    vga_crtc_write(0x11, c11);
}

// every colour in a 16x16 grid, a white frame and the diagonals, drawn a row at a time
static void draw_test(void) {
    const uint w = cur_mode->width, h = cur_mode->height;
    const uint cw = (w - 2) / 16, ch = (h - 2) / 16;

    lk_time_t t = current_time();
    for (uint y = 0; y < h; y++) {
        uint8_t *row = shadow + (size_t)y * w;
        if (y == 0 || y == h - 1) {
            memset(row, 0xff, w);
            continue;
        }
        const uint gy = MIN((y - 1) / ch, 15u);
        for (uint gx = 0; gx < 16; gx++) {
            const uint x = 1 + gx * cw;
            const uint len = (gx == 15) ? w - 1 - x : cw;
            memset(row + x, (int)(gy * 16 + gx), len);
        }
        row[0] = row[w - 1] = 0xff;
        const uint d = y * w / h;
        row[d] = row[w - 1 - d] = 0xff;
    }
    const lk_time_t drawn = current_time();
    flush_rows(0, h - 1);
    const lk_time_t flushed = current_time();
    printf("drew in %u ms, flushed %u bytes in %u ms\n", (uint)(drawn - t), w * h,
           (uint)(flushed - drawn));
}

static int cmd_vga(int argc, const console_cmd_args *argv) {
    if (argc < 2) {
usage:
        printf("usage:\n");
        printf("%s info\n", argv[0].str);
        printf("%s mode <name>  : switch to a graphics mode (see info)\n", argv[0].str);
        printf("%s test         : draw a test pattern in the current graphics mode\n", argv[0].str);
        printf("%s text         : back to text mode\n", argv[0].str);
        printf("%s dump         : print the registers\n", argv[0].str);
        printf("%s clocks [ms]  : measure every clock select, in a graphics mode\n", argv[0].str);
        printf("%s clock [sel]  : show or set the clock select (0-15)\n", argv[0].str);
        printf("%s reg <ts|crtc|gdc|atc> <index> [value] : read or write one register\n",
               argv[0].str);
        return -1;
    }

    if (!strcmp(argv[1].str, "info")) {
        if (et4000_found) {
            printf("Tseng ET4000, %uKB\n", et4000_kb);
        } else {
            printf("%s\n", vga_found ? "standard VGA" : "no VGA");
        }
        if (vga_found) {
            uint8_t cmd;
            if (vga_dac_hicolor_probe(&cmd)) {
                printf("DAC: HiColor, command register 0x%02x\n", cmd);
            } else {
                printf("DAC: plain\n");
            }
        }
        printf("current mode: %s\n", cur_mode ? cur_mode->name : "text");
        for (uint i = 0; i < countof(modes); i++) {
            printf("  %-8s%s%s\n", modes[i].name, modes[i].et4000 ? " (ET4000)" : "",
                   mode_usable(&modes[i]) ? "" : " not available");
        }
    } else if (!strcmp(argv[1].str, "mode") && argc >= 3) {
        const struct vga_mode *m = find_mode(argv[2].str);
        if (!m) {
            printf("no mode %s\n", argv[2].str);
            return -1;
        }
        status_t err = set_mode(m);
        if (err == ERR_NOT_SUPPORTED) {
            printf("%s is not available here\n", m->name);
            return err;
        } else if (err < 0) {
            printf("cannot switch to %s: %d\n", m->name, err);
            return err;
        }
    } else if (!strcmp(argv[1].str, "test")) {
        if (!cur_mode) {
            printf("not in a graphics mode\n");
            return -1;
        }
        draw_test();
    } else if (!strcmp(argv[1].str, "text")) {
        set_text();
    } else if (!strcmp(argv[1].str, "clocks")) {
        if (!cur_mode || !et4000_found) {
            printf("needs an ET4000 in a graphics mode\n");
            return -1;
        }
        measure_clocks(argc >= 3 ? argv[2].u : 1000);
    } else if (!strcmp(argv[1].str, "clock")) {
        if (!et4000_found) {
            printf("needs an ET4000\n");
            return -1;
        }
        if (argc >= 3) {
            et4000_set_clock(argv[2].u);
        }
        printf("clock select %u\n", et4000_get_clock());
    } else if (!strcmp(argv[1].str, "dump")) {
        if (!vga_found) {
            printf("no VGA\n");
            return -1;
        }
        dump_regs();
    } else if (!strcmp(argv[1].str, "reg") && argc >= 4) {
        const bool write = argc >= 5;
        return reg_command(argv[2].str, argv[3].u, write, write ? (uint8_t)argv[4].u : 0);
    } else {
        goto usage;
    }
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("vga", "VGA and Tseng ET4000 graphics modes", &cmd_vga)
STATIC_COMMAND_END(vga);
