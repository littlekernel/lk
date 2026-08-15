/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/*
 * BBC micro:bit v1, an nrf51822-QFAA (16k RAM, 256k flash) board.
 *
 * Board pinout, for reference (only the uart pins are wired up below, in
 * target/gpioconfig.h):
 *   uart to the KL26 interface chip: TX P0.24, RX P0.25
 *   button A P0.17, button B P0.26
 *   5x5 LED matrix, multiplexed as 3 rows x 9 columns:
 *     rows    P0.13 - P0.15
 *     columns P0.04 - P0.12
 * The matrix needs to be scanned rather than driven a pin at a time, so no
 * LED support here.
 *
 * qemu models this board as the 'microbit' machine, which implements the
 * uart, gpio, rng, nvmc and TIMER0-2, but no RTC and no LED matrix.
 */

#include <lk/debug.h>
#include <target.h>
#include <platform/nrf51.h>

void target_early_init(void) {
    nrf51_debug_early_init();
}

void target_init(void) {
    nrf51_debug_init();
    dprintf(SPEW, "Target: BBC micro:bit v1...\n");
}
