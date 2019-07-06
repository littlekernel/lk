/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <stm32f10x_usart.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_flash.h>
#include <stm32f10x_dbgmcu.h>
#include <platform/stm32.h>
#include <target/gpioconfig.h>

void target_early_init(void) {
    /* configure the usart1 pins */
    gpio_config(GPIO(GPIO_PORT_A, 9), GPIO_STM32_AF);
    gpio_config(GPIO(GPIO_PORT_A, 10), GPIO_INPUT);

    stm32_debug_early_init();

    /* configure some status leds */
    gpio_set(GPIO_LED0, 1);

    gpio_config(GPIO_LED0, GPIO_OUTPUT);
}

void target_init(void) {
    stm32_debug_init();
}

void target_set_debug_led(unsigned int led, bool on) {
    switch (led) {
        case 0:
            gpio_set(GPIO_LED0, !on);
            break;
    }
}

