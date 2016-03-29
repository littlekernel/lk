/*
 * Copyright (c) 2012 Ian McKellar
 * Copyright (c) 2013 Travis Geiselbrecht
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
#include <dev/usb.h>
#include <arch/arm/cm.h>

#include "ti_driverlib.h"


void stellaris_debug_early_init(void);
void stellaris_debug_init(void);

void stellaris_gpio_early_init(void);
void stellaris_gpio_init(void);

void stellaris_usbc_early_init(void);
void stellaris_usbc_init(void);

void platform_early_init(void)
{
    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
//  FPULazyStackingEnable();

    //
    // Set the clocking to run directly from the crystal.
    //

    ulong config = SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN;
#if !defined(CRYSTAL_FREQ) || CRYSTAL_FREQ == 16000000
    config |= SYSCTL_XTAL_16MHZ;
#elif CRYSTAL_FREQ == 8000000
    config |= SYSCTL_XTAL_8MHZ;
#else
#error add more cases for additional frequencies
#endif

    SysCtlClockSet(config);

    // start the generic systick timer
    arm_cm_systick_init(SysCtlClockGet());

    stellaris_gpio_early_init();

    stellaris_debug_early_init();

    stellaris_usbc_early_init();
}

void platform_init(void)
{
    stellaris_gpio_init();
    stellaris_debug_init();
    stellaris_usbc_init();

    // print device information
    printf("raw revision registers: 0x%lx 0x%lx\n", HWREG(SYSCTL_DID0), HWREG(SYSCTL_DID1));

    printf("stellaris device class: ");
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

}
