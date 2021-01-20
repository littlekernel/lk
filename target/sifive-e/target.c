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
#include <dev/gpio.h>

static volatile unsigned int *const prci_base = (unsigned int *)PRCI_BASE;

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

    // set up all the gpios
    for (uint i = 0; i < 32; i++) {
        switch (i) {
            // default to input
            default: gpio_config(i, GPIO_INPUT); break;

            // uart0
            case 16: gpio_config(i, GPIO_AF0); break;
            case 17: gpio_config(i, GPIO_AF0); break;

            // set the led gpios to output and default to off
            case GPIO_LED_GREEN: gpio_set(i, 0); gpio_config(i, GPIO_OUTPUT); break;
            case GPIO_LED_RED: gpio_set(i, 0); gpio_config(i, GPIO_OUTPUT); break;
            case GPIO_LED_BLUE: gpio_set(i, 0); gpio_config(i, GPIO_OUTPUT); break;
        }
    }
}

void target_set_debug_led(unsigned int led, bool on) {
    unsigned int gpio;

    switch (led) {
        default:
        case 0: gpio = GPIO_LED_GREEN; break;
        case 1: gpio = GPIO_LED_RED; break;
        case 2: gpio = GPIO_LED_BLUE; break;
    }

    gpio_set(gpio, on ? 1 : 0);
}

void target_init(void) {
}


