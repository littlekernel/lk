/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <sys/types.h>
#include <debug.h>
#include <printf.h>
#include <dev/net/smc91c96.h>
#include "smc91c96_p.h"

#if !defined(SMC91C96_BASE_ADDR) || !defined(SMC91C96_IRQ)
#error need to define SMC91C96_BASE_ADDR and SMC91C96_IRQ in project
#endif

static addr_t smc91c96_base = SMC91C96_BASE_ADDR;
static uint8_t mac_addr[6];

#define SMC_REG16(reg) ((volatile uint16_t *)(smc91c96_base + (reg)))
#define SMC_REG8(reg) ((volatile uint8_t *)(smc91c96_base + (reg)))

static inline void smc_bank(int bank)
{
	*SMC_REG16(SMC_BSR) = bank;
}

void smc91c96_init(void)
{
	int i;

	TRACE;

	// try to detect it
	if ((*SMC_REG16(SMC_BSR) & 0xff00) != 0x3300) {
		TRACEF("didn't see smc91c96 chip at 0x%x\n", (unsigned int)smc91c96_base);
	}

	// read revision
	smc_bank(3);
	TRACEF("detected, revision 0x%x\n", *SMC_REG16(SMC_REV));

	// read in the mac address
	smc_bank(1);
	for (i=0; i < 6; i++) {
		mac_addr[i] = *SMC_REG8(SMC_IAR0 + i);
	}
	TRACEF("mac address %02x:%02x:%02x:%02x:%02x:%02x\n", 
		mac_addr[0], mac_addr[1], mac_addr[2],
		mac_addr[3], mac_addr[4], mac_addr[5]);

	smc_bank(0);
}

