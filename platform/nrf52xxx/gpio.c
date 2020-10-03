/*
 * Copyright (c) 2016 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <assert.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <dev/gpio.h>
#include <nrfx.h>
#include <hal/nrf_gpio.h>

int gpio_config(unsigned nr, unsigned flags) {
    if (!nrf_gpio_pin_present_check(nr)) {
        return ERR_INVALID_ARGS;
    }

    if (flags & GPIO_OUTPUT) {
        nrf_gpio_cfg_output(nr);
    } else { // GPIO_INPUT
        nrf_gpio_pin_pull_t pull;
        if (flags & GPIO_PULLUP) {
            pull = NRF_GPIO_PIN_PULLUP;
        } else if (flags & GPIO_PULLDOWN) {
            pull = NRF_GPIO_PIN_PULLDOWN;
        } else {
            pull = NRF_GPIO_PIN_NOPULL;
        }
        nrf_gpio_cfg_input(nr, pull);
    }
    return NO_ERROR;
}

void gpio_set(unsigned nr, unsigned on) {
    DEBUG_ASSERT(nrf_gpio_pin_present_check(nr));

    nrf_gpio_pin_write(nr, on);
}

int gpio_get(unsigned nr) {
    DEBUG_ASSERT(nrf_gpio_pin_present_check(nr));

    return (int)nrf_gpio_pin_read(nr);
}

