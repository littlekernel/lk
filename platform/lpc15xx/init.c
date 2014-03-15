/*
 * Copyright (c) 2014 Travis Geiselbrecht
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
#include <stdio.h>
#include <debug.h>
#include <platform.h>
#include <platform/lpc.h>

void lpc_debug_early_init(void);
void lpc_debug_init(void);

void lpc_timer_early_init(void);
void lpc_timer_init(void);

void lpc_gpio_early_init(void);
void lpc_gpio_init(void);

void lpc_usbc_early_init(void);
void lpc_usbc_init(void);

void platform_early_init(void)
{
    lpc_timer_early_init();

    lpc_debug_early_init();

#if 0
    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
//  FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the crystal.
    //
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    lpc_gpio_early_init();

    lpc_usbc_early_init();
#endif
}

void platform_init(void)
{
    lpc_timer_init();
    lpc_debug_init();

#if 0
    lpc_gpio_init();
    lpc_usbc_init();

    // print device information
    printf("raw revision registers: 0x%lx 0x%lx\n", HWREG(SYSCTL_DID0), HWREG(SYSCTL_DID1));

    printf("lpc device class: ");
    if (CLASS_IS_SANDSTORM) printf("sandstorm");
    if (CLASS_IS_FURY) printf("fury");
    if (CLASS_IS_DUSTDEVIL) printf("dustdevil");
    if (CLASS_IS_TEMPEST) printf("tempst");
    if (CLASS_IS_FIRESTORM) printf("firestorm");
    if (CLASS_IS_BLIZZARD) printf("blizzard");
    printf("\n");

    printf("revision register: ");
    uint rev = (HWREG(SYSCTL_DID0) & SYSCTL_DID0_MAJ_M) >> 8;
    printf("%c", rev + 'A');
    printf("%ld", HWREG(SYSCTL_DID0) & (SYSCTL_DID0_MIN_M));
    printf("\n");
#endif
}

// vim: set ts=4 sw=4 expandtab:
