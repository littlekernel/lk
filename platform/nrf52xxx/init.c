/*
 * Copyright (c) 2016 Eric Holland
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
#include <err.h>
#include <debug.h>
#include <arch/arm/cm.h>
#include <dev/uart.h>
#include <platform.h>
#include <platform/nrf52.h>
#include <platform/system_nrf52.h>



void platform_early_init(void)
{
    // Crank up the clock before initing timers.
    SystemInit();
    arm_cm_systick_init(32768);
}

void platform_init(void)
{
    dprintf(SPEW, "Nordic nrf52xxx platform for lk...\n");
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
