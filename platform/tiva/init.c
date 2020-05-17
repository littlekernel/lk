/*
 * Copyright (c) 2012 Ian McKellar
 * Copyright (c) 2013 Travis Geiselbrecht
 * Copyright (c) 2020 Himanshu Sahdev aka CunningLearner
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <stdio.h>
#include <lk/debug.h>
#include <platform.h>
#include <dev/usb.h>
#include <arch/arm/cm.h>

#include "ti_driverlib.h"


void tiva_debug_early_init(void);
void tiva_debug_init(void);

void tiva_gpio_early_init(void);
void tiva_gpio_init(void);

void tiva_usbc_early_init(void);
void tiva_usbc_init(void);

void platform_early_init(void) {
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

    tiva_gpio_early_init();

    tiva_debug_early_init();

    tiva_usbc_early_init();
}

void platform_init(void) {
    tiva_gpio_init();
    tiva_debug_init();
    tiva_usbc_init();

    // print device information
    printf("raw revision registers: 0x%x 0x%x\n", HWREG(SYSCTL_DID0), HWREG(SYSCTL_DID1));

    printf("tiva device class: ");
    if (CLASS_IS_TM4C129) printf("tm4c129");
    if (CLASS_IS_TM4C123) printf("tm4c123");

    printf("\n");

    printf("revision register: ");
    uint rev = (HWREG(SYSCTL_DID0) & SYSCTL_DID0_MAJ_M) >> 8;
    printf("%c", rev + 'A');
    printf("%d", HWREG(SYSCTL_DID0) & (SYSCTL_DID0_MIN_M));
    printf("\n");

}
