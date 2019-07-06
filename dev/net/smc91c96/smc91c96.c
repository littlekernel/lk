/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <sys/types.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <stdio.h>
#include <dev/net/smc91c96.h>
#include "smc91c96_p.h"

#if !defined(SMC91C96_BASE_ADDR) || !defined(SMC91C96_IRQ)
#error need to define SMC91C96_BASE_ADDR and SMC91C96_IRQ in project
#endif

static addr_t smc91c96_base = SMC91C96_BASE_ADDR;
static uint8_t mac_addr[6];

#define SMC_REG16(reg) ((volatile uint16_t *)(smc91c96_base + (reg)))
#define SMC_REG8(reg) ((volatile uint8_t *)(smc91c96_base + (reg)))

static inline void smc_bank(int bank) {
    *SMC_REG16(SMC_BSR) = bank;
}

void smc91c96_init(void) {
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

