/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <dev/uart.h>
#include <platform.h>
#include <platform/stm32.h>
#include <arch/arm/cm.h>
#include <stm32h5xx.h>
#include "system_stm32h5xx.h"

void platform_early_init(void) {
    /*
     * Put the RCC back in its reset state and pick up the resulting clock
     * rate. The part comes out of reset running from HSI with HSIDIV set to
     * 2, so this is 32MHz, not the 64MHz that SystemCoreClock statically
     * initializes to. Everything below depends on the updated value, so keep
     * this first: getting it wrong puts the systick rate and the uart divisor
     * both off by the same factor of two.
     *
     * Running the PLL up to the part's 250MHz maximum is left for later.
     */
    SystemInit();
    SystemCoreClockUpdate();

    // start the systick timer
    arm_cm_systick_init(SystemCoreClock);

    stm32_gpio_early_init();
}

void platform_init(void) {
    dprintf(INFO, "sysclk %u Hz\n", (unsigned int)SystemCoreClock);
}
