/*
 * Copyright (c) 2016 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <assert.h>
#include <dev/gpio.h>
#include <platform/nrf52.h>
#include <platform/gpio.h>

int gpio_config(unsigned nr, unsigned flags) {
    DEBUG_ASSERT(nr <= NRF_MAX_PIN_NUMBER);

    unsigned init;

    if (flags & GPIO_OUTPUT) {

        NRF_P0->PIN_CNF[nr] = GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos     | \
                              GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos  | \
                              GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos   | \
                              GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos;
    } else { // GPIO_INPUT
        if (flags & GPIO_PULLUP) {
            init    =   GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos;
        } else if (flags & GPIO_PULLDOWN) {
            init    =   GPIO_PIN_CNF_PULL_Pulldown << GPIO_PIN_CNF_PULL_Pos;
        } else {
            init    =   GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos;
        }
        NRF_P0->PIN_CNF[nr] = GPIO_PIN_CNF_DIR_Input      <<  GPIO_PIN_CNF_DIR_Pos    | \
                              GPIO_PIN_CNF_INPUT_Connect  <<  GPIO_PIN_CNF_INPUT_Pos  | \
                              init;
    }
    return 0;
}

void gpio_set(unsigned nr, unsigned on) {
    DEBUG_ASSERT(nr <= NRF_MAX_PIN_NUMBER);

    if (on > 0) {
        NRF_P0->OUTSET = 1 << nr;
    } else {
        NRF_P0->OUTCLR = 1 << nr;
    }
}

int gpio_get(unsigned nr) {
    DEBUG_ASSERT( nr <= NRF_MAX_PIN_NUMBER );

    if ( NRF_P0->IN & ( 1 << nr) ) {
        return 1;
    } else {
        return 0;
    }
}

