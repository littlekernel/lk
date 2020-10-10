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
#include <nrfx_usbd.h>
#include <dev/gpio.h>
#include <dev/i2c.h>
#include <platform/init.h>
#include <target/gpioconfig.h>

void target_early_init(void) {
    gpio_config(GPIO_LED1, GPIO_OUTPUT);
    gpio_config(GPIO_LED2, GPIO_OUTPUT);
    gpio_config(GPIO_LED3, GPIO_OUTPUT);
    gpio_config(GPIO_LED4, GPIO_OUTPUT);

    LED1_OFF;
    LED2_OFF;
    LED3_OFF;
    LED4_OFF;

    nrf52_debug_early_init();
    // Note: i2c_init will only instantiate an i2c device if proper defines
    //   are set.  See comments at top of i2c_master.c(platform driver) for more info.
    i2c_init();
}


static void target_usb_init(void) {


}


void target_init(void) {
    nrf52_debug_init();
    dprintf(SPEW,"Target: PCA10056 DK...\n");
}


