/*
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <trace.h>
#include <stdio.h>
#include <string.h>
#include <lib/watchdog.h>
#include <platform.h>
#include <platform/zynq.h>

/* Driver for the system wide watchdog timer for Zynq */

#define LOCAL_TRACE 0

/* three magic values for the MODE, CONTROL, and RESTART registers */
#define SWDT_MODE_ZKEY      (0xabc << 12)
#define SWDT_CONTROL_CKEY   (0x248 << 14)
#define SWDT_RESTART_RSTKEY (0x1999)

/* reserved field in the MODE register */
#define SWDT_MODE_RESERVED  (0x4 << 4)

/* routines called from lib/watchdog */
status_t platform_watchdog_init(lk_time_t  target_timeout,
                                lk_time_t* recommended_pet_period)
{
    LTRACEF("target_timeout %u\n", (uint32_t)target_timeout);

    /* make sure the swdt is stopped */
    SWDT->MODE = SWDT_MODE_ZKEY | SWDT_MODE_RESERVED;

    /* make sure swdt has the proper clock */
    SLCR->WDT_CLK_SEL = 0; // cpu 1x

    uint32_t swdt_clock = zynq_get_swdt_freq();

    /* assuming a prescalar of / 4096, figure out the restart value */
    uint32_t restart = ((swdt_clock / 4096) * target_timeout) / 1000;

    /* make sure the restart value is <= 24 bits */
    if (restart > 0x00ffffff)
        restart = 0x00ffffff;

    LTRACEF("restart value %u\n", restart);

    /* the bottom 12 bits of restart are set to 0xfff by hardware */
    restart |= 0xfff;

    /* pet period is / 2 the computed restart value */
    if (recommended_pet_period)
        *recommended_pet_period = ((restart * 1000) / (swdt_clock / 4096)) / 2;

    LTRACEF("recommended pet period %u\n", (uint32_t)*recommended_pet_period);

    /* set up the swdt */

    /* load counter restart (top 12 bits of restart count), pclk / 4096 */
    SWDT->CONTROL = SWDT_CONTROL_CKEY | ((restart >> 12) << 2) | 3;

    /* zero it out */
    SWDT->RESTART = SWDT_RESTART_RSTKEY;

    DMB;

    return NO_ERROR;
}

void platform_watchdog_set_enabled(bool enabled)
{
    LTRACEF("enabled %u\n", enabled);

    if (enabled) {
        /* wide irq length, reset enable on counter zero, enable */
        SWDT->MODE = SWDT_MODE_ZKEY | SWDT_MODE_RESERVED | (3 << 7) | (1<<1) | 1;

        /* start it by petting it once, in case it already latched */
        SWDT->RESTART = SWDT_RESTART_RSTKEY;
    } else {
        /* disable everything */
        SWDT->MODE = SWDT_MODE_ZKEY | SWDT_MODE_RESERVED;
    }
    DMB;
}

void platform_watchdog_pet(void)
{
    //LTRACEF("pet\n");
    SWDT->RESTART = SWDT_RESTART_RSTKEY;
    DMB;
}

