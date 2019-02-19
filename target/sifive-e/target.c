/*
 * Copyright (c) 2019 Travis Geiselbrecht
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


