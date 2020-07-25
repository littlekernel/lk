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
#include <dev/i2c.h>
#include <platform/gpio.h>
#include <platform/stm32.h>
#include <platform/usbc.h>
#include <target/gpioconfig.h>
#include <target/usb.h>
#include <lk/console_cmd.h>

void target_early_init(void) {
    /* configure the usart2 pins */
    gpio_config(GPIO(GPIO_PORT_A, 2), GPIO_STM32_AF | GPIO_STM32_AFn(1));
    gpio_config(GPIO(GPIO_PORT_A, 3), GPIO_STM32_AF | GPIO_STM32_AFn(1));

    /* configure i2c pins */
    gpio_config(GPIO(GPIO_PORT_B, 8), GPIO_PULLUP | GPIO_STM32_AF | GPIO_STM32_AFn(1));  // SCL
    gpio_config(GPIO(GPIO_PORT_B, 9), GPIO_PULLUP | GPIO_STM32_AF | GPIO_STM32_AFn(1));  // SDA


    stm32_debug_early_init();

    /* configure some status leds */
    gpio_set(GPIO_LED0, 1);
    gpio_config(GPIO_LED0, GPIO_OUTPUT);

    i2c_init_early();
}

void target_init(void) {
    stm32_debug_init();
    i2c_init();
    stm32_usbc_init();
    target_usb_setup();
}

void target_set_debug_led(unsigned int led, bool on) {
    switch (led) {
        case 0:
            gpio_set(GPIO_LED0, on);
            break;
    }
}

static int cmd_i2c_rb(int argc, const console_cmd_args *argv) {

    if (argc != 3) {
        printf("usage: i2c_rb <addr> <reg>\n");
        return 1;
    }

    uint8_t addr = argv[1].u;
    uint8_t reg = argv[2].u;
    uint8_t data;

    status_t ret = i2c_read_reg_bytes(1, addr, reg, &data, 1);
    if (ret != NO_ERROR) {
        printf("error: %d\n", ret);
        return 1;
    }

    printf("%02x[%02x]: %02x\n", addr, reg, data);
    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("i2c_rb", "i2c_rb", &cmd_i2c_rb)
STATIC_COMMAND_END(nucleo);
