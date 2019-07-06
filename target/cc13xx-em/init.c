/*
 * Copyright (c) 2016 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <target/gpioconfig.h>
#include <dev/gpio.h>

void target_early_init(void) {
    gpio_config(GPIO_LED1, GPIO_OUTPUT);
    gpio_config(GPIO_LED2, GPIO_OUTPUT);
    gpio_config(GPIO_LED3, GPIO_OUTPUT);
    gpio_config(GPIO_LED4, GPIO_OUTPUT);
    gpio_set(GPIO_LED1, 0);
    gpio_set(GPIO_LED2, 0);
    gpio_set(GPIO_LED3, 0);
    gpio_set(GPIO_LED4, 0);
}

void target_init(void) {

}
