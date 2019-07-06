/*
 * Copyright (c) 2012-2014 Travis Geiselbrecht
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
#include <platform/nrf51.h>
#include <platform/system_nrf51.h>



void platform_early_init(void) {
    // Crank up the clock before initing timers.
    SystemInit();
    arm_cm_systick_init(32768);
}

void platform_init(void) {
    dprintf(SPEW, "Nordic nrf51xxx platform for lk...\n");
    dprintf(SPEW, "\tFlash: %d pages of %d bytes each (%dk bytes total)\n", \
            NRF_FICR->CODESIZE, NRF_FICR->CODEPAGESIZE, \
            (NRF_FICR->CODESIZE * NRF_FICR->CODEPAGESIZE)>>10);
    dprintf(SPEW, "\tRAM: %d blocks of %d bytes each (%dk bytes total)\n", \
            NRF_FICR->NUMRAMBLOCK, NRF_FICR->SIZERAMBLOCKS, \
            (NRF_FICR->NUMRAMBLOCK * NRF_FICR->SIZERAMBLOCKS)>>10);
    dprintf(SPEW, "\tRadio MAC address  %02x:%02x:%02x:%02x:%02x:%02x\n", \
            (NRF_FICR->DEVICEADDR[1] >> 8) & 0xFF, \
            (NRF_FICR->DEVICEADDR[1]) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >> 24) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >> 16) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >>  8) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >>  0) & 0xFF);
    dprintf(SPEW, "\tHWID: 0x%04x\n",NRF_FICR->CONFIGID & 0x0000ffff);

}
