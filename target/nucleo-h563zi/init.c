/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <target.h>
#include <lk/compiler.h>
#include <dev/gpio.h>
#include <platform/gpio.h>
#include <platform/stm32.h>
#include <target/gpioconfig.h>

void target_early_init(void) {
    /* configure the usart3 pins, which run to the ST-LINK virtual com port */
    gpio_config(GPIO(GPIO_PORT_D, 8), GPIO_STM32_AF | GPIO_STM32_AFn(7));
    gpio_config(GPIO(GPIO_PORT_D, 9), GPIO_STM32_AF | GPIO_STM32_AFn(7));

    stm32_debug_early_init();

    /* configure some status leds */
    gpio_set(GPIO_LED0, 0);
    gpio_set(GPIO_LED1, 0);
    gpio_set(GPIO_LED2, 0);
    gpio_config(GPIO_LED0, GPIO_OUTPUT);
    gpio_config(GPIO_LED1, GPIO_OUTPUT);
    gpio_config(GPIO_LED2, GPIO_OUTPUT);

    gpio_config(GPIO_BUTTON, GPIO_INPUT);
}

void target_init(void) {
    stm32_debug_init();
}

void target_set_debug_led(unsigned int led, bool on) {
    switch (led) {
        case 0:
            gpio_set(GPIO_LED0, on);
            break;
        case 1:
            gpio_set(GPIO_LED1, on);
            break;
        case 2:
            gpio_set(GPIO_LED2, on);
            break;
    }
}
