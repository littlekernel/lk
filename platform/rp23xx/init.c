// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <arch/arm/cm.h>
#include <dev/uart.h>
#include <platform.h>
#include <platform/rp23xx.h>
#include <target/debugconfig.h>

#include "usb/usbc.h"

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/resets.h>
#include <pico/runtime_init.h>

void platform_early_init(void) {
    // initialize the clock tree.
    // gets clock values from defines in SDK at
    // external/platform/pico/rp2350/hardware_regs/include/hardware/platform_defs.h
    clocks_init();

    // RP2350 defaults clk_sys to 150MHz unless overridden in platform_defs.h.
    arm_cm_systick_init(SYS_CLK_HZ);

    // unreset everything
    unreset_block_wait(RESETS_RESET_BITS);

    // Mux the debug UART pins — target-defined GPIO assignments.
    gpio_set_function(DEBUG_UART_GPIOA, GPIO_FUNC_UART);
    gpio_set_function(DEBUG_UART_GPIOB, GPIO_FUNC_UART);

    // Initialize the debug UART peripheral.
    uart_init_port(DEBUG_UART, 115200);

    // Early USB initialization (clock setup, etc.)
    rp23xx_usbc_early_init();
}

void platform_init(void) {
    // Initialize USB device controller
    rp23xx_usbc_init();
}
