/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

// VGA register access and graphics modes, CPU=legacy only. vga.c is the standard
// register set, vga_gfx.c the modes and the framebuffer, et4000.c the Tseng ET4000's
// extended modes and banking.

#include <lk/compiler.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

__BEGIN_CDECLS

#define VGA_TS_COUNT   5
#define VGA_CRTC_COUNT 25
#define VGA_GDC_COUNT  9
#define VGA_ATC_COUNT  21

// The standard register file, enough to switch between any two standard modes.
struct vga_regs {
    uint8_t misc;
    uint8_t ts[VGA_TS_COUNT];
    uint8_t crtc[VGA_CRTC_COUNT];
    uint8_t gdc[VGA_GDC_COUNT];
    uint8_t atc[VGA_ATC_COUNT];
};

// the 64KB graphics window at physical 0xa0000
#define VGA_WINDOW_SIZE 0x10000u
volatile uint8_t *vga_window(void);

// CRTC index port, 0x3d4 or 0x3b4 per Misc Output bit 0
uint16_t vga_crtc_port(void);

uint8_t vga_ts_read(uint8_t index);
void vga_ts_write(uint8_t index, uint8_t val);
uint8_t vga_crtc_read(uint8_t index);
void vga_crtc_write(uint8_t index, uint8_t val);
uint8_t vga_gdc_read(uint8_t index);
void vga_gdc_write(uint8_t index, uint8_t val);
uint8_t vga_atc_read(uint8_t index);
void vga_atc_write(uint8_t index, uint8_t val);

void vga_read_regs(struct vga_regs *regs);

// Program every standard register. Misc Output and the sequencer are written under a
// synchronous reset, CRTC 0x11's write protect is lifted for CRTC 0-7 and written last,
// and the attribute controller is left with the palette locked and the display on.
void vga_write_regs(const struct vga_regs *regs);

// DAC palette, 3 bytes (6 bit red, green, blue) per entry
void vga_dac_write(uint first, uint count, const uint8_t *rgb);
void vga_dac_read(uint first, uint count, uint8_t *rgb);

// A HiColor (Sierra style) DAC hides a command register behind four reads of the PEL
// mask. Returns false for a plain DAC; otherwise the command register's value.
bool vga_dac_hicolor_probe(uint8_t *command);

// Copy between memory and one bit plane through the window, for the text font in plane 2.
// The sequencer and graphics controller are put back as they were afterwards.
void vga_plane_read(uint plane, void *buf, size_t len);
void vga_plane_write(uint plane, const void *buf, size_t len);

// A Tseng ET4000. et4000_probe() unlocks the extended registers and decides whether the
// chip is there; the rest assume it is.
bool et4000_probe(void);
uint et4000_memory_kb(void);
void et4000_set_bank(uint bank);

// Clock select, 0-15: bits 1:0 are Misc Output bits 3:2, bit 2 CRTC 0x34 bit 1 and bit 3
// CRTC 0x31 bit 6. Which oscillator each one picks depends on the board.
uint et4000_get_clock(void);
void et4000_set_clock(uint sel);

// The extended registers a mode switch touches, saved from text mode and put back.
struct et4000_regs {
    uint8_t ts6, ts7;
    uint8_t crtc31, crtc33, crtc34, crtc35;
    uint8_t atc16;
    uint8_t segment;
};
void et4000_read_regs(struct et4000_regs *regs);
void et4000_write_regs(const struct et4000_regs *regs);

__END_CDECLS
