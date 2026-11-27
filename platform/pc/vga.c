/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// The standard VGA register set: Misc Output, sequencer (TS), CRT controller, graphics
// controller (GDC), attribute controller (ATC) and the DAC.

#include <arch/x86.h>
#include <kernel/vm.h>
#include <string.h>

#include "vga_p.h"

#define VGA_ATC_INDEX  0x3c0 // index and data writes alternate through a flip-flop
#define VGA_ATC_READ   0x3c1
#define VGA_MISC_WRITE 0x3c2
#define VGA_TS_INDEX   0x3c4
#define VGA_DAC_MASK   0x3c6
#define VGA_DAC_RINDEX 0x3c7
#define VGA_DAC_WINDEX 0x3c8
#define VGA_DAC_DATA   0x3c9
#define VGA_MISC_READ  0x3cc
#define VGA_GDC_INDEX  0x3ce

#define ATC_PAS 0x20 // palette address source: set, the display runs; clear, the palette is open

volatile uint8_t *vga_window(void) {
    return (volatile uint8_t *)paddr_to_kvaddr(0xa0000);
}

uint16_t vga_crtc_port(void) {
    return (inp(VGA_MISC_READ) & 1) ? 0x3d4 : 0x3b4;
}

uint8_t vga_ts_read(uint8_t index) {
    outp(VGA_TS_INDEX, index);
    return inp(VGA_TS_INDEX + 1);
}

void vga_ts_write(uint8_t index, uint8_t val) {
    outp(VGA_TS_INDEX, index);
    outp(VGA_TS_INDEX + 1, val);
}

uint8_t vga_crtc_read(uint8_t index) {
    const uint16_t port = vga_crtc_port();
    outp(port, index);
    return inp(port + 1);
}

void vga_crtc_write(uint8_t index, uint8_t val) {
    const uint16_t port = vga_crtc_port();
    outp(port, index);
    outp(port + 1, val);
}

uint8_t vga_gdc_read(uint8_t index) {
    outp(VGA_GDC_INDEX, index);
    return inp(VGA_GDC_INDEX + 1);
}

void vga_gdc_write(uint8_t index, uint8_t val) {
    outp(VGA_GDC_INDEX, index);
    outp(VGA_GDC_INDEX + 1, val);
}

// reading Input Status 1 puts the ATC flip-flop back to expecting an index
static void atc_reset_flipflop(void) {
    (void)inp(vga_crtc_port() + 6);
}

uint8_t vga_atc_read(uint8_t index) {
    atc_reset_flipflop();
    outp(VGA_ATC_INDEX, index);
    const uint8_t val = inp(VGA_ATC_READ);
    atc_reset_flipflop();
    outp(VGA_ATC_INDEX, ATC_PAS);
    return val;
}

void vga_atc_write(uint8_t index, uint8_t val) {
    atc_reset_flipflop();
    outp(VGA_ATC_INDEX, index);
    outp(VGA_ATC_INDEX, val);
    atc_reset_flipflop();
    outp(VGA_ATC_INDEX, ATC_PAS);
}

void vga_read_regs(struct vga_regs *regs) {
    regs->misc = inp(VGA_MISC_READ);
    for (uint i = 0; i < VGA_TS_COUNT; i++) {
        regs->ts[i] = vga_ts_read(i);
    }
    for (uint i = 0; i < VGA_CRTC_COUNT; i++) {
        regs->crtc[i] = vga_crtc_read(i);
    }
    for (uint i = 0; i < VGA_GDC_COUNT; i++) {
        regs->gdc[i] = vga_gdc_read(i);
    }
    atc_reset_flipflop();
    for (uint i = 0; i < VGA_ATC_COUNT; i++) {
        outp(VGA_ATC_INDEX, i);
        regs->atc[i] = inp(VGA_ATC_READ);
        atc_reset_flipflop();
    }
    outp(VGA_ATC_INDEX, ATC_PAS);
}

void vga_write_regs(const struct vga_regs *regs) {
    // Misc Output picks the clock, so it and the sequencer go in under a synchronous reset
    vga_ts_write(0, 0x01);
    outp(VGA_MISC_WRITE, regs->misc);
    for (uint i = 1; i < VGA_TS_COUNT; i++) {
        vga_ts_write(i, regs->ts[i]);
    }
    vga_ts_write(0, regs->ts[0]);

    // CRTC 0x11 bit 7 write protects CRTC 0-7
    vga_crtc_write(0x11, regs->crtc[0x11] & 0x7f);
    for (uint i = 0; i < VGA_CRTC_COUNT; i++) {
        if (i != 0x11) {
            vga_crtc_write(i, regs->crtc[i]);
        }
    }
    vga_crtc_write(0x11, regs->crtc[0x11]);

    for (uint i = 0; i < VGA_GDC_COUNT; i++) {
        vga_gdc_write(i, regs->gdc[i]);
    }

    atc_reset_flipflop();
    for (uint i = 0; i < VGA_ATC_COUNT; i++) {
        outp(VGA_ATC_INDEX, i);
        outp(VGA_ATC_INDEX, regs->atc[i]);
    }
    atc_reset_flipflop();
    outp(VGA_ATC_INDEX, ATC_PAS);
}

void vga_dac_write(uint first, uint count, const uint8_t *rgb) {
    outp(VGA_DAC_MASK, 0xff);
    outp(VGA_DAC_WINDEX, first);
    for (uint i = 0; i < count * 3; i++) {
        outp(VGA_DAC_DATA, rgb[i]);
    }
}

void vga_dac_read(uint first, uint count, uint8_t *rgb) {
    outp(VGA_DAC_RINDEX, first);
    for (uint i = 0; i < count * 3; i++) {
        rgb[i] = inp(VGA_DAC_DATA);
    }
}

static void dac_to_pel(void) {
    (void)inp(VGA_DAC_WINDEX);
}

static void dac_to_command(void) {
    dac_to_pel();
    for (int i = 0; i < 4; i++) {
        (void)inp(VGA_DAC_MASK);
    }
}

bool vga_dac_hicolor_probe(uint8_t *command) {
    dac_to_pel();
    const uint8_t mask = inp(VGA_DAC_MASK);
    dac_to_command();
    const uint8_t cmd = inp(VGA_DAC_MASK);
    dac_to_pel();

    // A plain DAC hands back the PEL mask however many times it is read. A mask that
    // can't be written gives no answer, and counts as plain.
    const uint8_t not_cmd = (uint8_t)~cmd;
    outp(VGA_DAC_MASK, not_cmd);
    dac_to_pel();
    const bool mask_writable = inp(VGA_DAC_MASK) == not_cmd;
    dac_to_command();
    bool hidden = mask_writable && inp(VGA_DAC_MASK) != not_cmd;

    // and the hidden register holds what is written to it: flip the HiColor bit and back
    if (hidden) {
        dac_to_command();
        outp(VGA_DAC_MASK, cmd ^ 0x80);
        dac_to_command();
        hidden = inp(VGA_DAC_MASK) == (cmd ^ 0x80);
        dac_to_command();
        outp(VGA_DAC_MASK, cmd);
    }

    dac_to_pel();
    outp(VGA_DAC_MASK, mask);
    if (hidden) {
        *command = cmd;
    }
    return hidden;
}

// Planar access: one plane through the whole 64KB window, no odd/even or chain 4, write
// mode 0 with the data going straight through.
struct planar_saved {
    uint8_t ts2, ts4;
    uint8_t gdc[VGA_GDC_COUNT];
};

static void planar_begin(struct planar_saved *s, uint8_t ts2, uint8_t gdc4) {
    s->ts2 = vga_ts_read(2);
    s->ts4 = vga_ts_read(4);
    for (uint i = 0; i < VGA_GDC_COUNT; i++) {
        s->gdc[i] = vga_gdc_read(i);
    }
    vga_ts_write(2, ts2);
    vga_ts_write(4, 0x06);  // extended memory, odd/even off, chain 4 off
    vga_gdc_write(1, 0x00); // no set/reset
    vga_gdc_write(3, 0x00); // no rotate, no logic op
    vga_gdc_write(4, gdc4); // read map
    vga_gdc_write(5, 0x00); // read and write mode 0, odd/even off
    vga_gdc_write(6, 0x04); // 64KB at 0xa0000, chain odd/even off, alphanumeric
    vga_gdc_write(8, 0xff); // every bit from the cpu
}

static void planar_end(const struct planar_saved *s) {
    vga_ts_write(2, s->ts2);
    vga_ts_write(4, s->ts4);
    for (uint i = 0; i < VGA_GDC_COUNT; i++) {
        vga_gdc_write(i, s->gdc[i]);
    }
}

void vga_plane_read(uint plane, void *buf, size_t len) {
    struct planar_saved s;
    planar_begin(&s, 0x00, (uint8_t)plane);
    const volatile uint8_t *win = vga_window();
    uint8_t *out = buf;
    for (size_t i = 0; i < len && i < VGA_WINDOW_SIZE; i++) {
        out[i] = win[i];
    }
    planar_end(&s);
}

void vga_plane_write(uint plane, const void *buf, size_t len) {
    struct planar_saved s;
    planar_begin(&s, (uint8_t)(1u << plane), 0);
    volatile uint8_t *win = vga_window();
    const uint8_t *in = buf;
    for (size_t i = 0; i < len && i < VGA_WINDOW_SIZE; i++) {
        win[i] = in[i];
    }
    planar_end(&s);
}
