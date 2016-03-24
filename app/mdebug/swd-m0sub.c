/* swdp-m0sub.c
 *
 * Copyright 2015 Brian Swetland <swetland@frotz.net>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <app.h>
#include <debug.h>
#include <string.h>
#include <stdlib.h>
#include <printf.h>

#include <platform.h>
#include <arch/arm.h>
#include <kernel/thread.h>

#include <platform/lpc43xx-gpio.h>
#include <platform/lpc43xx-sgpio.h>
#include <platform/lpc43xx-clocks.h>

#include "rswdp.h"

#include "lpclink2.h"

static void gpio_init(void) {
	pin_config(PIN_LED, PIN_MODE(0) | PIN_PLAIN);
	pin_config(PIN_RESET, PIN_MODE(4) | PIN_PLAIN);
	pin_config(PIN_RESET_TXEN, PIN_MODE(4) | PIN_PLAIN);

	pin_config(PIN_SWDIO_TXEN, PIN_MODE(6) | PIN_PLAIN | PIN_FAST);
	pin_config(PIN_SWDIO, PIN_MODE(6) | PIN_PLAIN | PIN_INPUT | PIN_FAST);
	pin_config(PIN_SWCLK, PIN_MODE(6) | PIN_PLAIN | PIN_FAST);

	pin_config(PIN_SWO, PIN_MODE(1) | PIN_PLAIN | PIN_INPUT | PIN_FAST);

	gpio_set(GPIO_LED, 0);
	gpio_set(GPIO_RESET, 1);
	gpio_set(GPIO_RESET_TXEN, 0);

	gpio_config(GPIO_LED, GPIO_OUTPUT);
	gpio_config(GPIO_RESET, GPIO_OUTPUT);
	gpio_config(GPIO_RESET_TXEN, GPIO_OUTPUT);
}


/* returns 1 if the number of bits set in n is odd */
static unsigned parity(unsigned n) {
        n = (n & 0x55555555) + ((n & 0xaaaaaaaa) >> 1);
        n = (n & 0x33333333) + ((n & 0xcccccccc) >> 2);
        n = (n & 0x0f0f0f0f) + ((n & 0xf0f0f0f0) >> 4);
        n = (n & 0x00ff00ff) + ((n & 0xff00ff00) >> 8);
        n = (n & 0x0000ffff) + ((n & 0xffff0000) >> 16);
        return n & 1;
}

#include "fw-m0sub.h"

#define M0SUB_ZEROMAP		0x40043308
#define M0SUB_TXEV		0x40043314 // write 0 to clear
#define M4_TXEV			0x40043130 // write 0 to clear

#define RESET_CTRL0		0x40053100
#define M0_SUB_RST		(1 << 12)

#define COMM_CMD		0x18004000
#define COMM_ARG1		0x18004004
#define COMM_ARG2		0x18004008
#define COMM_RESP		0x1800400C

#define M0_CMD_ERR		0
#define M0_CMD_NOP		1
#define M0_CMD_READ		2
#define M0_CMD_WRITE		3
#define M0_CMD_RESET		4
#define M0_CMD_SETCLOCK		5

#define RSP_BUSY	0xFFFFFFFF

void swd_init(void) {
	gpio_init();

	writel(BASE_CLK_SEL(CLK_PLL1), BASE_PERIPH_CLK);
	spin(1000);

	// SGPIO15 SWDIO_TXEN
	writel(CFG_OUT_GPIO | CFG_OE_GPIO, SGPIO_OUT_CFG(15));
	// SGPIO14 SWDIO
	writel(CFG_OUT_GPIO | CFG_OE_GPIO, SGPIO_OUT_CFG(14));
	// SGPIO11 SWCLK
	writel(CFG_OUT_GPIO | CFG_OE_GPIO, SGPIO_OUT_CFG(11));

	// all outputs enabled and high
	writel((1 << 11) | (1 << 14) | (1 << 15), SGPIO_OUT);
	writel((1 << 11) | (1 << 14) | (1 << 15), SGPIO_OEN);

	writel(0, M4_TXEV);
	writel(M0_SUB_RST, RESET_CTRL0);
	writel(0x18000000, M0SUB_ZEROMAP);
	writel(0xffffffff, 0x18004000);
	memcpy((void*) 0x18000000, zero_bin, sizeof(zero_bin));
	DSB;
	writel(0, RESET_CTRL0);
}

int swd_write(unsigned hdr, unsigned data) {
	unsigned n;
	unsigned p = parity(data);
	writel(M0_CMD_WRITE, COMM_CMD);
	writel((hdr << 8) | (p << 16), COMM_ARG1);
	writel(data, COMM_ARG2);
	writel(RSP_BUSY, COMM_RESP);
	DSB;
	asm("sev");
	while ((n = readl(COMM_RESP)) == RSP_BUSY) ;
	//printf("wr s=%d\n", n);
	return n;
}

int swd_read(unsigned hdr, unsigned *val) {
	unsigned n, data, p;
	writel(M0_CMD_READ, COMM_CMD);
	writel(hdr << 8, COMM_ARG1);
	writel(RSP_BUSY, COMM_RESP);
	DSB;
	asm("sev");
	while ((n = readl(COMM_RESP)) == RSP_BUSY) ;
	if (n) {
		return n;
	}
	data = readl(COMM_ARG1);
	p = readl(COMM_ARG2);
	if (p != parity(data)) {
		return ERR_PARITY;
	}
	//printf("rd s=%d p=%d d=%08x\n", n, p, data);
	*val = data;
	return 0;
}

void swd_reset(void) {
	unsigned n;
	writel(M0_CMD_RESET, COMM_CMD);
	writel(RSP_BUSY, COMM_RESP);
	DSB;
	asm("sev");
	while ((n = readl(COMM_RESP)) == RSP_BUSY) ;
}

unsigned swd_set_clock(unsigned khz) {
	unsigned n;
	if (khz > 8000) {
		khz = 8000;
	}
	writel(M0_CMD_SETCLOCK, COMM_CMD);
	writel(khz/1000, COMM_ARG1);
	writel(RSP_BUSY, COMM_RESP);
	DSB;
	asm("sev");
	while ((n = readl(COMM_RESP)) == RSP_BUSY) ;

	// todo: accurate value
	return khz;
}

void swd_hw_reset(int assert) {
	if (assert) {
		gpio_set(GPIO_RESET, 0);
		gpio_set(GPIO_RESET_TXEN, 1);
	} else {
		gpio_set(GPIO_RESET, 1);
		gpio_set(GPIO_RESET_TXEN, 0);
	}
}
