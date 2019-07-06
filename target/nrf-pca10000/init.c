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
#include <kernel/timer.h>
#include <platform/gpio.h>
#include <platform/nrf51.h>
#include <target/gpioconfig.h>

void target_early_init(void) {
    NRF_CLOCK->XTALFREQ = CLOCK_XTALFREQ_XTALFREQ_16MHz;

    /* configure the usart1 pins */
    gpio_config(GPIO_LED1, GPIO_OUTPUT);
    gpio_config(GPIO_LED2, GPIO_OUTPUT);
    gpio_config(GPIO_LED3, GPIO_OUTPUT);

    gpio_set(GPIO_LED1,1);
    gpio_set(GPIO_LED2,1);
    gpio_set(GPIO_LED3,1);

    gpio_config(UART0_RTS_PIN, GPIO_OUTPUT);
    gpio_set(UART0_RTS_PIN,0);              //placate flow control requirements of pca10000

    nrf51_debug_early_init();
}

void target_init(void) {
    nrf51_debug_init();
    dprintf(SPEW,"Target: PCA10000 DK...\n");
}
