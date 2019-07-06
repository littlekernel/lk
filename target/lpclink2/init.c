/*
 * Copyright (c) 2015 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/reg.h>
#include <lk/debug.h>
#include <printf.h>
#include <kernel/thread.h>

#include <platform/lpc43xx-gpio.h>

void target_early_init(void) {
    // UART2 on P2.10 (TX) and P2.11 (RX)
    pin_config(PIN(2,10), PIN_MODE(2) | PIN_PLAIN);
    pin_config(PIN(2,11), PIN_MODE(2) | PIN_PLAIN | PIN_INPUT);

    // SPIFI
    pin_config(PIN(3,3), PIN_MODE(3) | PIN_PLAIN); // SPIFI_SCK
    pin_config(PIN(3,4), PIN_MODE(3) | PIN_PLAIN | PIN_INPUT); // SPIFI_SIO3
    pin_config(PIN(3,5), PIN_MODE(3) | PIN_PLAIN | PIN_INPUT); // SPIFI_SIO2
    pin_config(PIN(3,6), PIN_MODE(3) | PIN_PLAIN | PIN_INPUT); // SPIFI_MISO
    pin_config(PIN(3,7), PIN_MODE(3) | PIN_PLAIN | PIN_INPUT); // SPIFI_MOSI
    pin_config(PIN(3,8), PIN_MODE(3) | PIN_PLAIN); // SPIFI_CS
}

void target_init(void) {
}

