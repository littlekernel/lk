/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// Device discovery for CPU=legacy (pre PCI) PCs, done the way the original AT did it:
// the BIOS data area and the CMOS say what the firmware believes is fitted, and a probe
// of each well known port decides what is really there. The hints are only reported;
// a device counts as found when its probe answers.

#include <kernel/vm.h>
#include <lk/compiler.h>
#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <platform/cmos.h>
#include <platform/pc.h>
#include <stdio.h>

#include "platform_p.h"

// BIOS data area, at physical 0x400
#define BDA_PHYS          0x400
#define BDA_COM_BASE      0x00 // 4 words, 0 for none
#define BDA_LPT_BASE      0x08 // 3 words, 0 for none
#define BDA_EQUIPMENT     0x10 // word
#define BDA_BASE_MEM_KB   0x13 // word
#define BDA_VIDEO_MODE    0x49
#define BDA_CRTC_BASE     0x63 // word, 0x3d4 colour or 0x3b4 mono
#define BDA_HARDDISK_COUNT 0x75

#define COM_COUNT 4
#define LPT_COUNT 3

static const uint16_t com_std_ports[COM_COUNT] = { COM1_REG, COM2_REG, COM3_REG, COM4_REG };

struct legacy_hints {
    uint16_t bda_com[COM_COUNT];
    uint16_t bda_lpt[LPT_COUNT];
    uint16_t bda_equipment;
    uint16_t bda_base_mem_kb;
    uint8_t bda_video_mode;
    uint16_t bda_crtc;
    uint8_t bda_hd_count;

    uint8_t cmos_floppy;
    uint8_t cmos_hd;
    uint8_t cmos_equipment;
    uint16_t cmos_base_kb;
    uint16_t cmos_ext_kb;
    bool cmos_checksum_ok;
};

static struct legacy_hints hints;

static struct {
    uint16_t port;
    enum pc_uart_type type;
} uarts[COM_COUNT];

static uint16_t bda_read16(const uint8_t *bda, uint off) {
    return (uint16_t)(bda[off] | (bda[off + 1] << 8));
}

// standard checksum over 0x10-0x2d, stored big endian at 0x2e
static bool cmos_checksum_valid(void) {
    uint16_t sum = 0;
    for (uint8_t reg = 0x10; reg <= 0x2d; reg++) {
        sum += cmos_read(reg);
    }
    return sum == (uint16_t)((cmos_read(CMOS_REG_CHECKSUM_HI) << 8) | cmos_read(CMOS_REG_CHECKSUM_LO));
}

static void read_hints(void) {
    const uint8_t *bda = paddr_to_kvaddr(BDA_PHYS);
    if (bda) {
        for (uint i = 0; i < COM_COUNT; i++) {
            hints.bda_com[i] = bda_read16(bda, BDA_COM_BASE + i * 2);
        }
        for (uint i = 0; i < LPT_COUNT; i++) {
            hints.bda_lpt[i] = bda_read16(bda, BDA_LPT_BASE + i * 2);
        }
        hints.bda_equipment = bda_read16(bda, BDA_EQUIPMENT);
        hints.bda_base_mem_kb = bda_read16(bda, BDA_BASE_MEM_KB);
        hints.bda_video_mode = bda[BDA_VIDEO_MODE];
        hints.bda_crtc = bda_read16(bda, BDA_CRTC_BASE);
        hints.bda_hd_count = bda[BDA_HARDDISK_COUNT];
    }

    hints.cmos_floppy = cmos_read(CMOS_REG_FLOPPY);
    hints.cmos_hd = cmos_read(CMOS_REG_HARDDISK);
    hints.cmos_equipment = cmos_read(CMOS_REG_EQUIPMENT);
    hints.cmos_base_kb = cmos_read_word(CMOS_REG_BASE_MEM_LO);
    hints.cmos_ext_kb = cmos_read_word(CMOS_REG_EXT_MEM_LO);
    hints.cmos_checksum_ok = cmos_checksum_valid();
}

static const char *floppy_type_name(uint8_t type) {
    switch (type) {
        case 0: return "none";
        case 1: return "360K";
        case 2: return "1.2M";
        case 3: return "720K";
        case 4: return "1.44M";
        case 5: return "2.88M";
    }
    return "?";
}

// The standard four ports, plus any non standard one the BIOS found. The console is in
// use and keeps the identity it was given at boot rather than being probed again.
static void probe_uarts(void) {
    enum pc_uart_type console_type;
    const uint16_t console_port = pc_console_uart(&console_type);

    for (uint i = 0; i < COM_COUNT; i++) {
        uint16_t port = com_std_ports[i];
        const uint16_t bda_port = hints.bda_com[i];
        if (bda_port && bda_port != port) {
            port = bda_port;
        }
        uarts[i].port = port;
        uarts[i].type = (port == console_port) ? console_type : pc_uart_probe(port);
    }
}

static void print_report(void) {
    enum pc_uart_type console_type;
    const uint16_t console_port = pc_console_uart(&console_type);

    for (uint i = 0; i < COM_COUNT; i++) {
        if (uarts[i].type == PC_UART_NONE) {
            continue;
        }
        printf("LEGACY: com%u at %#x: %s%s\n", i + 1, uarts[i].port,
               pc_uart_type_name(uarts[i].type),
               uarts[i].port == console_port ? " (console)" : "");
    }

    printf("LEGACY: cmos floppy A: %s B: %s, hard disk types %#x/%#x, checksum %s\n",
           floppy_type_name(hints.cmos_floppy >> 4), floppy_type_name(hints.cmos_floppy & 0xf),
           hints.cmos_hd >> 4, hints.cmos_hd & 0xf, hints.cmos_checksum_ok ? "ok" : "BAD");
    printf("LEGACY: cmos memory base %uK extended %uK\n", hints.cmos_base_kb, hints.cmos_ext_kb);
}

static void print_hints(void) {
    const uint16_t eq = hints.bda_equipment;
    printf("BDA: com %#x %#x %#x %#x, lpt %#x %#x %#x\n", hints.bda_com[0], hints.bda_com[1],
           hints.bda_com[2], hints.bda_com[3], hints.bda_lpt[0], hints.bda_lpt[1],
           hints.bda_lpt[2]);
    printf("BDA: equipment %#06x: %u floppies, x87 %s, video mode bits %u, %u serial, %u parallel\n",
           eq, (eq & 1) ? ((eq >> 6) & 3) + 1 : 0, (eq & 2) ? "yes" : "no", (eq >> 4) & 3,
           (eq >> 9) & 7, (eq >> 14) & 3);
    printf("BDA: base memory %uK, video mode %#x, crtc %#x, %u hard disks\n",
           hints.bda_base_mem_kb, hints.bda_video_mode, hints.bda_crtc, hints.bda_hd_count);
    printf("CMOS: floppy %#04x, hard disk %#04x, equipment %#04x\n", hints.cmos_floppy,
           hints.cmos_hd, hints.cmos_equipment);
}

void pc_legacy_probe(void) {
    read_hints();
    probe_uarts();
    print_report();
}

static int cmd_legacy(int argc, const console_cmd_args *argv) {
    print_hints();
    print_report();
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("legacy", "legacy PC device probe report: BDA, CMOS and what answered", &cmd_legacy)
STATIC_COMMAND_END(legacy);
