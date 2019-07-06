/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <stdio.h>
#include <lk/debug.h>
#include <platform.h>
#include <platform/lpc.h>
#include <arch/arm/cm.h>

void lpc_debug_early_init(void);
void lpc_debug_init(void);

void lpc_gpio_early_init(void);
void lpc_gpio_init(void);

void lpc_usbc_early_init(void);
void lpc_usbc_init(void);

void platform_early_init(void) {
    /* set up clocking for a board with an external oscillator */
    Chip_SetupXtalClocking();

    /* Set USB PLL input to main oscillator */
    Chip_Clock_SetUSBPLLSource(SYSCTL_PLLCLKSRC_MAINOSC);
    /* Setup USB PLL  (FCLKIN = 12MHz) * 4 = 48MHz
       MSEL = 3 (this is pre-decremented), PSEL = 1 (for P = 2)
       FCLKOUT = FCLKIN * (MSEL + 1) = 12MHz * 4 = 48MHz
       FCCO = FCLKOUT * 2 * P = 48MHz * 2 * 2 = 192MHz (within FCCO range) */
    Chip_Clock_SetupUSBPLL(3, 1);

    /* Powerup USB PLL */
    Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_USBPLL_PD);

    /* Wait for PLL to lock */
    while (!Chip_Clock_IsUSBPLLLocked()) {}

    /* Set default system tick divder to 1 */
    Chip_Clock_SetSysTickClockDiv(1);

    /* start the generic systick driver */
    arm_cm_systick_init(Chip_Clock_GetMainClockRate());

    lpc_debug_early_init();
}

void platform_init(void) {
    lpc_debug_init();
}

// vim: set ts=4 sw=4 expandtab:
