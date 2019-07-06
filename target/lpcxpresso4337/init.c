/*
 * Copyright (c) 2015 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/reg.h>
#include <lk/debug.h>

#include <platform/lpc43xx-gpio.h>

void target_early_init(void) {
    // UART1 on P6.4 (TX) and P2.1 (RX)
    // LpcXpresso4337 P4 FTDI header
    pin_config(PIN(6,4), PIN_MODE(2) | PIN_PLAIN);
    pin_config(PIN(2,1), PIN_MODE(1) | PIN_PLAIN | PIN_INPUT);
}

void target_init(void) {
}

