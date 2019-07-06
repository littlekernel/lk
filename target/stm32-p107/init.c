/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <target.h>
#include <lk/compiler.h>
#include <dev/gpio.h>
#include <stm32f10x_usart.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_flash.h>
#include <stm32f10x_dbgmcu.h>
#include <platform/stm32.h>
#include <platform/gpio.h>
#include <target/gpioconfig.h>

void target_early_init(void) {
    /* configure the usart3 pins */
    GPIO_PinRemapConfig(GPIO_FullRemap_USART3, ENABLE);

    gpio_config(GPIO(GPIO_PORT_D, 8), GPIO_STM32_AF);
    gpio_config(GPIO(GPIO_PORT_D, 9), GPIO_INPUT);

    stm32_debug_early_init();

    /* configure some status leds */
    gpio_set(GPIO_LED0, 0);
    gpio_set(GPIO_LED1, 0);

    gpio_config(GPIO_LED0, GPIO_OUTPUT);
    gpio_config(GPIO_LED1, GPIO_OUTPUT);
}

void target_init(void) {
    TRACE_ENTRY;

    stm32_debug_init();

    TRACE_EXIT;
}

void target_set_debug_led(unsigned int led, bool on) {
    switch (led) {
        case 0:
            gpio_set(GPIO_LED0, on);
            break;
        case 1:
            gpio_set(GPIO_LED1, on);
            break;
    }
}

