/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <target.h>
#include <arch/arch_ops.h>
#include <platform/sifive.h>

static volatile unsigned int *const prci_base = (unsigned int *)PRCI_BASE;
static volatile unsigned int *const gpio_base = (unsigned int *)GPIO_BASE;

#define GPIO_LED_GREEN 19
#define GPIO_LED_BLUE  21
#define GPIO_LED_RED   22

void target_early_init(void) {
    // enable the external 16Mhz crystal
    prci_base[1] = (1<<30); // hfxosc enable
    while ((prci_base[1] & (1<<31)) == 0) // wait for hfxosc ready
        ;

    // program the pll bypass, we should be running at 16Mhz now
    prci_base[2] = 0x00070df1;

    // lfclock is a 32768Hz crystal, strapped externally

    // io function enable for pin 16/17, no IOF for all others
    gpio_base[14] = (3<<16);

    // turn our LED gpios off
    gpio_base[GPIO_REG_PORT] |= (1u << GPIO_LED_GREEN) | (1u << GPIO_LED_BLUE) | (1u << GPIO_LED_RED);

    // set the led gpios to output
    gpio_base[GPIO_REG_OUTPUT_EN] |= (1u << GPIO_LED_GREEN) | (1u << GPIO_LED_BLUE) | (1u << GPIO_LED_RED);
}

void target_set_debug_led(unsigned int led, bool on) {
    uint val = 0;
    if (led == 0) {
        val = 1u << GPIO_LED_GREEN;
    } else if (led == 1) {
        val = 1u << GPIO_LED_RED;
    } else if (led == 2) {
        val = 1u << GPIO_LED_BLUE;
    }

    // set and clear the LED gpios using atomic instructions
    // polarity is inverted
    if (on) {
        __atomic_fetch_and((int *)&gpio_base[GPIO_REG_PORT], ~val, __ATOMIC_RELAXED);
    } else {
        __atomic_fetch_or((int *)&gpio_base[GPIO_REG_PORT], val, __ATOMIC_RELAXED);
    }
}

void target_init(void) {
}


