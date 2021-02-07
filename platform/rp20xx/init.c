// Copyright (c) 2020 Brian Swetland
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <platform.h>
#include <arch/arm/cm.h>

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/uart.h>
#include <hardware/resets.h>

extern void* vectab;

void platform_early_init(void) {
    clocks_init();

    unreset_block_wait(RESETS_RESET_BITS);

    uart_init(uart0, 1000000);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
    uart_puts(uart0, "Hello World!\n");

    // arch/arm/arm-m/arch.c does this but only for M3 and above...
    SCB->VTOR = (uint32_t) &vectab;

    arm_cm_systick_init(133000000);
}

void platform_init(void) {
}


bool running_on_fpga(void) {
	return false;
}
