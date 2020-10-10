/*
 * Copyright (c) 2016 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <arch/arm/cm.h>
#include <dev/uart.h>
#include <platform.h>
#include <platform/init.h>
#include <platform/clock.h>
#include <nrfx.h>
#include <mdk/system_nrf52.h>
#include <nrfx_clock.h>

void platform_early_init(void) {

    SystemInit();
    // Initialize the clock control peripheral
    nrf52_clock_init();

    arm_cm_systick_init(32768);
}

void platform_init(void) {

    dprintf(SPEW, "Nordic nrf52xxx platform for lk...\n");

    char *variant = (char *)&NRF_FICR->FICR_INFO.VARIANT;
    dprintf(SPEW, "Part Number - %x-%c%c%c%c\n", NRF_FICR->FICR_INFO.PART,
            variant[3], variant[2], variant[1], variant[0]);
    dprintf(SPEW, "\tRam: %dk\n", NRF_FICR->FICR_INFO.RAM);
    dprintf(SPEW, "\tFlash: %d pages of %d bytes each (%dk bytes total)\n", \
            NRF_FICR->CODESIZE, NRF_FICR->CODEPAGESIZE, \
            (NRF_FICR->CODESIZE * NRF_FICR->CODEPAGESIZE)>>10);
    dprintf(SPEW, "\tRadio MAC address  %02x:%02x:%02x:%02x:%02x:%02x\n", \
            (NRF_FICR->DEVICEADDR[1] >> 8) & 0xFF, \
            (NRF_FICR->DEVICEADDR[1]) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >> 24) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >> 16) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >>  8) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >>  0) & 0xFF);
}
