// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <platform.h>
#include <arch/arm/cm.h>
#include <target/debugconfig.h>

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <hardware/resets.h>

void platform_early_init(void) {
    // initialize the clock tree.
    // gets clock values from defines in SDK at
    // external/platform/pico/rp2040/hardware_regs/include/hardware/platform_defs.h
    clocks_init();

    // start the systick timer
    arm_cm_systick_init(125000000);

    // unreset everything
    unreset_block_wait(RESETS_RESET_BITS);

    // target defines drive how we configure the debug uart
    uart_init(DEBUG_UART, 115200);
    gpio_set_function(DEBUG_UART_GPIOA, GPIO_FUNC_UART);
    gpio_set_function(DEBUG_UART_GPIOB, GPIO_FUNC_UART);
    uart_puts(DEBUG_UART, "Hello World!\n");
}


