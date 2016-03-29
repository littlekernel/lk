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
	pin_config(PIN_TMS_TXEN, PIN_MODE(0) | PIN_PLAIN);

	pin_config(PIN_TDO, PIN_MODE(6) | PIN_PLAIN | PIN_INPUT | PIN_FAST);
	pin_config(PIN_TCK, PIN_MODE(6) | PIN_PLAIN | PIN_FAST);
	pin_config(PIN_TDI, PIN_MODE(6) | PIN_PLAIN | PIN_FAST);
	pin_config(PIN_TMS, PIN_MODE(6) | PIN_PLAIN | PIN_FAST);

	gpio_set(GPIO_LED, 0);
	gpio_set(GPIO_RESET, 1);
	gpio_set(GPIO_RESET_TXEN, 0);
	gpio_set(GPIO_TMS_TXEN, 1);

	gpio_config(GPIO_LED, GPIO_OUTPUT);
	gpio_config(GPIO_RESET, GPIO_OUTPUT);
	gpio_config(GPIO_RESET_TXEN, GPIO_OUTPUT);
	gpio_config(GPIO_TMS_TXEN, GPIO_OUTPUT);
}

#define POS_TDO		10
#define POS_TCK		11
#define POS_TDI		12
#define POS_TMS		14

#define BIT_TDO		(1 << POS_TDO)
#define BIT_TCK		(1 << POS_TCK)
#define BIT_TDI		(1 << POS_TDI)
#define BIT_TMS		(1 << POS_TMS)

void jtag_init(void) {
	gpio_init();

	writel(BASE_CLK_SEL(CLK_PLL1), BASE_PERIPH_CLK);
	spin(1000);

	// configure for SGPIO_OUT/OEN control
	writel(CFG_OUT_GPIO | CFG_OE_GPIO, SGPIO_OUT_CFG(POS_TDO));
	writel(CFG_OUT_GPIO | CFG_OE_GPIO, SGPIO_OUT_CFG(POS_TCK));
	writel(CFG_OUT_GPIO | CFG_OE_GPIO, SGPIO_OUT_CFG(POS_TDI));
	writel(CFG_OUT_GPIO | CFG_OE_GPIO, SGPIO_OUT_CFG(POS_TMS));

	// TCK=0 TDI=0 TMS=0 TDO=input
	writel(0, SGPIO_OUT);
	writel(BIT_TCK | BIT_TDI | BIT_TMS, SGPIO_OEN);
}

static unsigned jtag_tick(unsigned tms, unsigned tdi) {
	unsigned x = (tms << POS_TMS) | (tdi << POS_TDI);
	unsigned v;
	writel(x, SGPIO_OUT);
	writel(x, SGPIO_OUT);
	writel(x, SGPIO_OUT);
	x |= BIT_TCK;
	v = readl(SGPIO_IN);
	writel(x, SGPIO_OUT);
	writel(x, SGPIO_OUT);
	writel(x, SGPIO_OUT);
	writel(x, SGPIO_OUT);
	writel(x, SGPIO_OUT);
	writel(x, SGPIO_OUT);
	x ^= BIT_TCK;
	writel(x, SGPIO_OUT);
	return (v >> POS_TDO) & 1;
}

int jtag_io(unsigned count, unsigned tms, unsigned tdi, unsigned *tdo) {
	unsigned n = 0;
	unsigned bit = 0;
	while (count > 0) {
		n |= (jtag_tick(tms & 1, tdi & 1) << bit);
		bit++;
		count--;
		tms >>= 1;
		tdi >>= 1;
	}
	*tdo = n;
	return 0;
}
