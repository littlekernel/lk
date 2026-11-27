/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// Tseng Labs ET4000: detection, memory size, banking and the extended registers a mode
// switch touches. Page numbers are the 1990 ET4000 data book's printed ones.

#include <arch/x86.h>

#include "vga_p.h"

#define ET4000_HERCULES_COMPAT 0x3bf
#define ET4000_SEGMENT         0x3cd // bits 3:0 write bank, 7:4 read bank, 64KB each

// CRTC extended registers
#define CRTC_GENERAL   0x31 // bit 6 is clock select 3
#define CRTC_EXT_START 0x33 // display start and cursor address bits 17:16
#define CRTC_COMPAT    0x34 // bit 1 is clock select 2
#define CRTC_OVERFLOW  0x35 // bit 7 interlace; write protected by CRTC 0x11 bit 7
#define CRTC_VSCONF2   0x37 // bits 1:0 bus width, bit 3 chip depth

static uint current_bank = ~0u;

// The KEY (p102): opens the CRTC registers above 0x18 (0x33 and 0x35 are always open), TS 6
// and 7, ATC 0x16 and the segment register. A synchronous reset closes the segment
// register again, so this is sent after every mode switch.
static void et4000_key(void) {
    outp(ET4000_HERCULES_COMPAT, 0x03);
    outp(vga_crtc_port() + 4, 0xa0); // mode control, 0x3d8 or 0x3b8
}

// whether the bits in mask hold both all zeros and all ones
static bool port_bits_rw(uint16_t port, uint8_t mask) {
    const uint8_t old = inp(port);
    outp(port, old & ~mask);
    const bool zeros = (inp(port) & mask) == 0;
    outp(port, old | mask);
    const bool ones = (inp(port) & mask) == mask;
    outp(port, old);
    return zeros && ones;
}

static bool crtc_bits_rw(uint8_t index, uint8_t mask) {
    const uint8_t old = vga_crtc_read(index);
    vga_crtc_write(index, old & ~mask);
    const bool zeros = (vga_crtc_read(index) & mask) == 0;
    vga_crtc_write(index, old | mask);
    const bool ones = (vga_crtc_read(index) & mask) == mask;
    vga_crtc_write(index, old);
    return zeros && ones;
}

// The vgadoc test: the segment register holds its bits once unlocked, and CRTC 0x33,
// which an ET3000 lacks, holds its low four.
bool et4000_probe(void) {
    et4000_key();
    if (!port_bits_rw(ET4000_SEGMENT, 0x3f)) {
        return false;
    }
    return crtc_bits_rw(CRTC_EXT_START, 0x0f);
}

// CRTC 0x37 bits 1:0 bus width and bit 3 chip depth, as the BIOS set them at power on
uint et4000_memory_kb(void) {
    switch (vga_crtc_read(CRTC_VSCONF2) & 0x0b) {
        case 0x03:
        case 0x09:
            return 256;
        case 0x0a:
            return 512;
        case 0x0b:
            return 1024;
        default:
            return 0;
    }
}

void et4000_set_bank(uint bank) {
    if (bank != current_bank) {
        outp(ET4000_SEGMENT, (uint8_t)((bank & 0xf) | (bank & 0xf) << 4));
        current_bank = bank;
    }
}

uint et4000_get_clock(void) {
    et4000_key();
    uint sel = (inp(0x3cc) >> 2) & 3;
    sel |= (vga_crtc_read(CRTC_COMPAT) & 0x02) << 1;
    sel |= (vga_crtc_read(CRTC_GENERAL) & 0x40) >> 3;
    return sel;
}

void et4000_set_clock(uint sel) {
    et4000_key();
    const uint8_t ts0 = vga_ts_read(0);
    vga_ts_write(0, 0x01);
    outp(0x3c2, (uint8_t)((inp(0x3cc) & ~0x0c) | (sel & 3) << 2));
    vga_crtc_write(CRTC_COMPAT, (uint8_t)((vga_crtc_read(CRTC_COMPAT) & ~0x02) | (sel & 4) >> 1));
    vga_crtc_write(CRTC_GENERAL, (uint8_t)((vga_crtc_read(CRTC_GENERAL) & ~0x40) | (sel & 8) << 3));
    vga_ts_write(0, ts0);
    et4000_key();
}

void et4000_read_regs(struct et4000_regs *regs) {
    et4000_key();
    regs->ts6 = vga_ts_read(6);
    regs->ts7 = vga_ts_read(7);
    regs->crtc31 = vga_crtc_read(CRTC_GENERAL);
    regs->crtc33 = vga_crtc_read(CRTC_EXT_START);
    regs->crtc34 = vga_crtc_read(CRTC_COMPAT);
    regs->crtc35 = vga_crtc_read(CRTC_OVERFLOW);
    regs->atc16 = vga_atc_read(0x16);
    regs->segment = inp(ET4000_SEGMENT);
}

// After vga_write_regs(), whose synchronous reset closed the segment register.
void et4000_write_regs(const struct et4000_regs *regs) {
    et4000_key();

    // TS 7 holds clock dividers, so it changes under a synchronous reset (p139)
    const uint8_t ts0 = vga_ts_read(0);
    vga_ts_write(0, 0x01);
    vga_ts_write(6, regs->ts6);
    vga_ts_write(7, regs->ts7);
    vga_ts_write(0, ts0);
    et4000_key();

    const uint8_t crtc11 = vga_crtc_read(0x11);
    vga_crtc_write(0x11, crtc11 & 0x7f);
    vga_crtc_write(CRTC_OVERFLOW, regs->crtc35);
    vga_crtc_write(0x11, crtc11);
    vga_crtc_write(CRTC_GENERAL, regs->crtc31);
    vga_crtc_write(CRTC_EXT_START, regs->crtc33);
    vga_crtc_write(CRTC_COMPAT, regs->crtc34);
    vga_atc_write(0x16, regs->atc16);

    outp(ET4000_SEGMENT, regs->segment);
    current_bank = ~0u;
}
