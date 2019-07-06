/*
 * Copyright (c) 2015 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/debug.h>
#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include <lk/reg.h>

#include <platform/lpc43xx-spifi.h>

#define CMD_PAGE_PROGRAM        0x02
#define CMD_READ_DATA           0x03
#define CMD_READ_STATUS         0x05
#define CMD_WRITE_ENABLE        0x06
#define CMD_SECTOR_ERASE        0x20

static void spifi_write_enable(void) {
    writel(CMD_FF_SERIAL | CMD_FR_OP | CMD_OPCODE(CMD_WRITE_ENABLE),
           SPIFI_CMD);
    while (readl(SPIFI_STAT) & STAT_CMD) ;
}

static void spifi_wait_busy(void) {
    while (readl(SPIFI_STAT) & STAT_CMD) ;
    writel(CMD_POLLBIT(0) | CMD_POLLCLR | CMD_POLL |
           CMD_FF_SERIAL | CMD_FR_OP | CMD_OPCODE(CMD_READ_STATUS),
           SPIFI_CMD);
    while (readl(SPIFI_STAT) & STAT_CMD) ;
    // discard matching status byte from fifo
    readb(SPIFI_DATA);
}

void spifi_page_program(u32 addr, u32 *ptr, u32 count) {
    spifi_write_enable();
    writel(addr, SPIFI_ADDR);
    writel(CMD_DATALEN(count * 4) | CMD_FF_SERIAL | CMD_FR_OP_3B |
           CMD_DOUT | CMD_OPCODE(CMD_PAGE_PROGRAM), SPIFI_CMD);
    while (count-- > 0) {
        writel(*ptr++, SPIFI_DATA);
    }
    spifi_wait_busy();
}

void spifi_sector_erase(u32 addr) {
    spifi_write_enable();
    writel(addr, SPIFI_ADDR);
    writel(CMD_FF_SERIAL | CMD_FR_OP_3B | CMD_OPCODE(CMD_SECTOR_ERASE),
           SPIFI_CMD);
    spifi_wait_busy();
}

int spifi_verify_erased(u32 addr, u32 count) {
    int err = 0;
    writel(addr, SPIFI_ADDR);
    writel(CMD_DATALEN(count * 4) | CMD_FF_SERIAL | CMD_FR_OP_3B |
           CMD_OPCODE(CMD_READ_DATA), SPIFI_CMD);
    while (count-- > 0) {
        if (readl(SPIFI_DATA) != 0xFFFFFFFF) err = -1;
    }
    while (readl(SPIFI_STAT) & STAT_CMD) ;
    return err;
}

int spifi_verify_page(u32 addr, u32 *ptr) {
    int count = 256 / 4;
    int err = 0;
    writel(addr, SPIFI_ADDR);
    writel(CMD_DATALEN(count * 4) | CMD_FF_SERIAL | CMD_FR_OP_3B |
           CMD_OPCODE(CMD_READ_DATA), SPIFI_CMD);
    while (count-- > 0) {
        if (readl(SPIFI_DATA) != *ptr++) err = -1;
    }
    while (readl(SPIFI_STAT) & STAT_CMD) ;
    return err;
}

// at reset-stop, all clocks are running from 12MHz internal osc
// todo: run SPIFI_CLK at a much higher rate
// todo: use 4bit modes
void spifi_init(void) {
    // reset spifi controller
    writel(STAT_RESET, SPIFI_STAT);
    while (readl(SPIFI_STAT) & STAT_RESET) ;
    writel(0xFFFFF, SPIFI_CTRL);
}

