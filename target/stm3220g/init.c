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
#include <stm32f2xx_usart.h>
#include <stm32f2xx_rcc.h>
#include <stm32f2xx_gpio.h>
//#include <stm32f10x_flash.h>
//#include <stm32f10x_dbgmcu.h>
#include <platform/stm32.h>
#include <platform/gpio.h>
#include <target/gpioconfig.h>

void target_early_init(void) {
    /* configure the usart3 pins */
    gpio_config(GPIO_USART3_TX, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF_USART3) | GPIO_PULLUP);
    gpio_config(GPIO_USART3_RX, GPIO_STM32_AF | GPIO_STM32_AFn(GPIO_AF_USART3) | GPIO_PULLUP);

    stm32_debug_early_init();

    /* configure some status leds */
    gpio_config(GPIO_LED0, GPIO_OUTPUT);
    gpio_config(GPIO_LED1, GPIO_OUTPUT);
    gpio_config(GPIO_LED2, GPIO_OUTPUT);
    gpio_config(GPIO_LED3, GPIO_OUTPUT);

    stm3220g_set_led_bits(1);
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
        case 2:
            gpio_set(GPIO_LED2, on);
            break;
        case 3:
            gpio_set(GPIO_LED3, on);
            break;
    }
}

void stm3220g_set_led_bits(unsigned int nr) {
    gpio_set(GPIO_LED0, nr & 1);
    gpio_set(GPIO_LED1, nr & 2);
    gpio_set(GPIO_LED2, nr & 4);
    gpio_set(GPIO_LED3, nr & 8);
}
