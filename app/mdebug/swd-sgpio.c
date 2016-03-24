/* swdp-sgpio.c
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
#include <dev/udc.h>

#include <platform.h>
#include <arch/arm.h>
#include <kernel/thread.h>

#include <platform/lpc43xx-gpio.h>
#include <platform/lpc43xx-sgpio.h>
#include <platform/lpc43xx-clocks.h>

#define PIN_LED		PIN(1,1)
#define PIN_RESET	PIN(2,5)
#define PIN_RESET_TXEN	PIN(2,6)
#define PIN_SWDIO_TXEN	PIN(1,5)	// SGPIO15=6
#define PIN_SWDIO	PIN(1,6)	// SGPIO14=6
#define PIN_SWO		PIN(1,14)	// U1_RXD=1
#define PIN_SWCLK	PIN(1,17)	// SGPIO11=6

#define GPIO_LED	GPIO(0,8)
#define GPIO_RESET	GPIO(5,5)
#define GPIO_RESET_TXEN	GPIO(5,6)
#define GPIO_SWDIO_TXEN	GPIO(1,8)
#define GPIO_SWDIO	GPIO(1,9)
#define GPIO_SWCLK	GPIO(0,12)

static unsigned sgpio_div = 31; // 6MHz

static void gpio_init(void) {
	pin_config(PIN_LED, PIN_MODE(0) | PIN_PLAIN);
	pin_config(PIN_RESET, PIN_MODE(4) | PIN_PLAIN);
	pin_config(PIN_RESET_TXEN, PIN_MODE(4) | PIN_PLAIN);
	pin_config(PIN_SWDIO_TXEN, PIN_MODE(0) | PIN_PLAIN);
	pin_config(PIN_SWDIO, PIN_MODE(0) | PIN_PLAIN | PIN_INPUT | PIN_FAST);
	pin_config(PIN_SWCLK, PIN_MODE(0) | PIN_PLAIN | PIN_FAST);
	pin_config(PIN_SWO, PIN_MODE(1) | PIN_PLAIN | PIN_INPUT | PIN_FAST);

	gpio_set(GPIO_LED, 0);
	gpio_set(GPIO_RESET, 1);
	gpio_set(GPIO_RESET_TXEN, 0);
	gpio_set(GPIO_SWDIO, 0);
	gpio_set(GPIO_SWDIO_TXEN, 1);
	gpio_set(GPIO_SWCLK, 0);

	gpio_config(GPIO_LED, GPIO_OUTPUT);
	gpio_config(GPIO_RESET, GPIO_OUTPUT);
	gpio_config(GPIO_RESET_TXEN, GPIO_OUTPUT);
	gpio_config(GPIO_SWDIO, GPIO_OUTPUT);
	gpio_config(GPIO_SWDIO_TXEN, GPIO_OUTPUT);
	gpio_config(GPIO_SWCLK, GPIO_OUTPUT);
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

static void sgpio_txn(unsigned slices) {
	// clear any previous status bits
	writel(0xFFFF, SLICE_XHG_STS_CLR);
	// kick the txn
	writel(slices, SLICE_CTRL_ENABLE);
	writel(slices, SLICE_CTRL_DISABLE);
	// wait for all slices to complete
	while ((readl(SLICE_XHG_STS) & slices) != slices) ;
	// shut down clocks
	writel(0, SLICE_CTRL_ENABLE);
	writel(0, SLICE_CTRL_DISABLE);
}

// SWDIO       SLICE_H      SLICE_P        SLICE_D      SLICE_O     SWDIO
// SGPIO14 -> [31....0] -> [31....0]      [31....0] -> [31....0] -> SGPIO14
//
//                                         SLICE_M     SWDIO_TXEN
//                                        [31....7] -> SGPIO15
//
//                                         SLICE_F     SWDIO_OE
//                                        [31....0] -> SGPIO14_OE


// configures all slices, muxes, etc
// ensures that outputs are enabled and SWDIO and SWCLK are high
static void sgpio_init(void) {
	writel(BASE_CLK_SEL(CLK_PLL1), BASE_PERIPH_CLK);

	// make sure everything's shut down
	writel(0, SLICE_CTRL_ENABLE);
	writel(0, SLICE_CTRL_DISABLE);
	writel(0xFFFF, SLICE_XHG_STS_CLR);

	// SWDIO_TXEN (SGPIO15)
	// M[31..7] -> OUT
	writel(CFG_OUT_M8B | CFG_OE_GPIO, SGPIO_OUT_CFG(15));
	writel(CLK_USE_SLICE | QUAL_ENABLE | CONCAT_SLICE, SLICE_CFG1(SLC_M));
	writel(CLK_GEN_INTERNAL | SHIFT_1BPC, SLICE_CFG2(SLC_M));

	// SWDIO (SGPIO14)
	// IN -> H[31..0] -> P[31..0]
	//       D[31..0] -> O[31..0] -> OUT
	//                   F[31..0] -> OE
	writel(CFG_OUT_M2C | CFG_OE_M1, SGPIO_OUT_CFG(14));
	writel(CLK_USE_SLICE | QUAL_ENABLE | CONCAT_PIN, SLICE_CFG1(SLC_H));
	writel(CLK_GEN_INTERNAL | SHIFT_1BPC, SLICE_CFG2(SLC_H));
	writel(CLK_USE_SLICE | QUAL_ENABLE | CONCAT_SLICE | CONCAT_2_SLICE, SLICE_CFG1(SLC_P));
	writel(CLK_GEN_INTERNAL | SHIFT_1BPC, SLICE_CFG2(SLC_P));
	writel(CLK_USE_SLICE | QUAL_ENABLE | CONCAT_SLICE, SLICE_CFG1(SLC_D));
	writel(CLK_GEN_INTERNAL | SHIFT_1BPC, SLICE_CFG2(SLC_D));
	writel(CLK_USE_SLICE | QUAL_ENABLE | CONCAT_SLICE | CONCAT_2_SLICE, SLICE_CFG1(SLC_O));
	writel(CLK_GEN_INTERNAL | SHIFT_1BPC, SLICE_CFG2(SLC_O));
	writel(CLK_USE_SLICE | QUAL_ENABLE | CONCAT_SLICE, SLICE_CFG1(SLC_F));
	writel(CLK_GEN_INTERNAL | SHIFT_1BPC, SLICE_CFG2(SLC_F));

	// SWDCLK (SGPIO11)
	// SLICE_N CLK -> OUT
	writel(CFG_OUT_CLK | CFG_OE_GPIO, SGPIO_OUT_CFG(11));
	writel(CLK_USE_SLICE | QUAL_ENABLE, SLICE_CFG1(SLC_N));
	writel(CLK_GEN_INTERNAL | SHIFT_1BPC | INV_CLK_OUT, SLICE_CFG2(SLC_N));

	// ensure output and enables idle high
	writel(1, SLICE_REG(SLC_F));
	writel(1, SLICE_REG(SLC_O));
	writel(1 << 7, SLICE_REG(SLC_M));

	// enable all outputs
	writel((1 << 11) | (1 << 14) | (1 << 15), SGPIO_OEN);

	// select SGPIOs via pin mux
	pin_config(PIN_SWDIO_TXEN, PIN_MODE(6) | PIN_PLAIN | PIN_FAST);
	pin_config(PIN_SWDIO, PIN_MODE(6) | PIN_PLAIN | PIN_INPUT | PIN_FAST);
	pin_config(PIN_SWCLK, PIN_MODE(6) | PIN_PLAIN | PIN_FAST);
}

static void sgpio_swd_clock_setup(unsigned div) {
	writel(div, SLICE_PRESET(SLC_D));
	writel(div, SLICE_PRESET(SLC_F));
	writel(div, SLICE_PRESET(SLC_H));
	writel(div, SLICE_PRESET(SLC_M));
	writel(div, SLICE_PRESET(SLC_N));
	writel(div, SLICE_PRESET(SLC_O));
	writel(div, SLICE_PRESET(SLC_P));
}

static void sgpio_swd_reset(unsigned div) {
	// shift out 64 clocks while DATA is 1
	writel(0, SLICE_COUNT(SLC_N));
	writel(POS_POS(63) | POS_RESET(63), SLICE_POS(SLC_N));
	sgpio_txn(1 << SLC_N);

	// shift out 16bit jtag->swd escape pattern
	writel(0, SLICE_COUNT(SLC_N));
	writel(POS_POS(15) | POS_RESET(15), SLICE_POS(SLC_N));

	writel(0b1110011110011110, SLICE_REG(SLC_O));
	writel(1, SLICE_SHADOW(SLC_O));
	writel(div, SLICE_COUNT(SLC_O));
	writel(POS_POS(15) | POS_RESET(15), SLICE_POS(SLC_O));
	sgpio_txn((1 << SLC_N) | (1 << SLC_O));

	// shift out 64 clocks while DATA is 1
	writel(0, SLICE_COUNT(SLC_N));
	writel(POS_POS(63) | POS_RESET(63), SLICE_POS(SLC_N));
	sgpio_txn(1 << SLC_N);
};

// shift out 8 0s then the 8bit header, then disable outputs
// and shift in the turnaround bit (ignored) and 3 bit ack
// leaves output enables low
//
// todo: make leader optional/adjustable
static int sgpio_swd_header(unsigned div, uint32_t hdr) {
	unsigned timeout = 16;
	unsigned ack;

	for (;;) {
		// 16 bits tx_en, then stop, disabling tx_en
		writel(0xFFFF << 7, SLICE_REG(SLC_M));
		writel(0, SLICE_SHADOW(SLC_M));
		writel(div, SLICE_COUNT(SLC_M));
		writel(POS_POS(15) | POS_RESET(15), SLICE_POS(SLC_M));

		// 16 bits output, then stop, disabling OE
		writel(0xFFFF, SLICE_REG(SLC_F));
		writel(0, SLICE_SHADOW(SLC_F));
		writel(div, SLICE_COUNT(SLC_F));
		writel(POS_POS(15) | POS_RESET(15), SLICE_POS(SLC_F));

		// 16 bits data out
		writel(hdr << 8, SLICE_REG(SLC_O));
		writel(1, SLICE_SHADOW(SLC_O));
		writel(div, SLICE_COUNT(SLC_O));
		writel(POS_POS(15) | POS_RESET(15), SLICE_POS(SLC_O));

		// 20 bits data in
		writel(0, SLICE_COUNT(SLC_H));
		writel(POS_POS(19) | POS_RESET(19), SLICE_POS(SLC_H));
		writel(0, SLICE_REG(SLC_H));

		// 20 bits clock
		writel(0, SLICE_COUNT(SLC_N));
		writel(POS_POS(19) | POS_RESET(19), SLICE_POS(SLC_N));

		sgpio_txn((1<<SLC_M)|(1<<SLC_F)|(1<<SLC_O)|(1<<SLC_H)|(1<<SLC_N));

		if ((ack = readl(SLICE_SHADOW(SLC_H)) >> 29) == 1) {
			// OKAY
			if (timeout < 16) printf("[%d]\n",16-timeout);
			return 0;
		}

		// re-enable oe, tx_en, and make data high
		writel(1, SLICE_REG(SLC_O));
		writel(1 << 7, SLICE_REG(SLC_M));
		writel(1, SLICE_REG(SLC_F));
	
		// technically we should do a Turn cycle here,
		// but we rely on the fact that we prefix all ops
		// with some leader 0s and can be lazy

		if (ack == 2) {
			// WAIT
			if (timeout == 0) {
				return -1;
			}
			timeout--;
		} else {
			printf("ERR %d\n", ack);
			// FAULT or invalid response
			return -1;
		}
	}
}

static int sgpio_swd_read(unsigned div, uint32_t hdr, uint32_t *_data) {
	uint32_t data, p;
	
	//printf("rd(%d,%02x)\n", div, hdr);
	if (sgpio_swd_header(div, hdr)) {
		return -1;
	}

	// 34 bits in -> H -> P
	writel(0, SLICE_COUNT(SLC_H));
	writel(POS_POS(33) | POS_RESET(33), SLICE_POS(SLC_H));
	writel(0, SLICE_REG(SLC_H));
	writel(0, SLICE_COUNT(SLC_P));
	writel(POS_POS(33) | POS_RESET(33), SLICE_POS(SLC_P));
	writel(0, SLICE_REG(SLC_P));

	writel(0, SLICE_COUNT(SLC_N));
	writel(POS_POS(33) | POS_RESET(33), SLICE_POS(SLC_N));
	sgpio_txn((1<<SLC_H)|(1<<SLC_P)|(1<<SLC_N));

	// re-enable oe, tx_en, and make data high
	writel(1, SLICE_REG(SLC_O));
	writel(1 << 7, SLICE_REG(SLC_M));
	writel(1, SLICE_REG(SLC_F));

	p = readl(SLICE_SHADOW(SLC_H));
	data = (p << 2) | (readl(SLICE_SHADOW(SLC_P)) >> 30);
	p = (p >> 30) & 1;

	//printf("RD %08x %d\n", data, p);
	if (parity(data) != p) {
		printf("parity error\n");
		return -1;
	}
	*_data = data;
	return 0;
}

static int sgpio_swd_write(unsigned div, uint32_t hdr, uint32_t data) {
	uint32_t p = parity(data);

	//printf("wr(%d,%02x,%08x) p=%d\n", div, hdr, data, p);
	if (sgpio_swd_header(div, hdr)) {
		return -1;
	}

	// 34 bits  D -> O -> out
	writel(div, SLICE_COUNT(SLC_D));
	writel(POS_POS(33) | POS_RESET(33), SLICE_POS(SLC_D));
	writel((p << 1) | (data >> 31), SLICE_REG(SLC_D));
	writel(div, SLICE_COUNT(SLC_O));
	writel(POS_POS(33) | POS_RESET(33), SLICE_POS(SLC_O));
	writel((data << 1) | 1, SLICE_REG(SLC_O));

	writel(0, SLICE_COUNT(SLC_N));
	writel(POS_POS(33) | POS_RESET(33), SLICE_POS(SLC_N));
	
	// re-enable oe, tx_en
	writel(1 << 7, SLICE_REG(SLC_M));
	writel(1, SLICE_REG(SLC_F));

	sgpio_txn((1<<SLC_D)|(1<<SLC_O)|(1<<SLC_N));

	return 0;
}

void swd_init(void) {
	gpio_init();
	sgpio_init();
}

void swd_reset(void) {
	unsigned div = sgpio_div;
	sgpio_swd_clock_setup(div);
	sgpio_swd_reset(div);
}

int swd_read(unsigned reg, unsigned *val) {
	unsigned div = sgpio_div;
	sgpio_swd_clock_setup(div);
	return sgpio_swd_read(div, reg, val);
}

int swd_write(unsigned reg, unsigned val) {
	unsigned div = sgpio_div;
	sgpio_swd_clock_setup(div);
	return sgpio_swd_write(div, reg, val);
}

unsigned swd_set_clock(unsigned khz) {
	unsigned div;
	if (khz < 2000) khz = 2000;
	if (khz > 48000) khz = 48000;
	div = 192000 / khz;
	sgpio_div = div - 1;
	return 192000 / div;
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

