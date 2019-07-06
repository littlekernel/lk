/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <assert.h>
#include <dev/gpio.h>
#include <platform/stm32.h>
#include <platform/gpio.h>
#include <stm32f2xx_gpio.h>
#include <stm32f2xx_rcc.h>

static GPIO_TypeDef *port_to_pointer(unsigned int port) {
    switch (port) {
        default:
        case GPIO_PORT_A:
            return GPIOA;
        case GPIO_PORT_B:
            return GPIOB;
        case GPIO_PORT_C:
            return GPIOC;
        case GPIO_PORT_D:
            return GPIOD;
        case GPIO_PORT_E:
            return GPIOE;
        case GPIO_PORT_F:
            return GPIOF;
        case GPIO_PORT_G:
            return GPIOG;
        case GPIO_PORT_H:
            return GPIOH;
        case GPIO_PORT_I:
            return GPIOI;
    }
}

static void enable_port(unsigned int port) {
    DEBUG_ASSERT(port <= GPIO_PORT_I);

    /* happens to be the RCC ids are sequential bits, so we can start from A and shift */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA << port, ENABLE);
}

void stm32_gpio_early_init(void) {
}

int gpio_config(unsigned nr, unsigned flags) {

    uint port = GPIO_PORT(nr);
    uint pin = GPIO_PIN(nr);

    enable_port(port);

    GPIO_InitTypeDef init;
    init.GPIO_Speed = GPIO_Speed_50MHz;
    init.GPIO_Pin = (1 << pin);
    init.GPIO_PuPd = GPIO_PuPd_NOPULL;

    if (flags & GPIO_INPUT) {
        init.GPIO_Mode = GPIO_Mode_IN;
    } else if  (flags & GPIO_OUTPUT) {
        init.GPIO_Mode = GPIO_Mode_OUT;
    } else if  (flags & GPIO_STM32_AF) {
        init.GPIO_Mode = GPIO_Mode_AF;
        GPIO_PinAFConfig(port_to_pointer(port), pin, GPIO_AFNUM(flags));
    }

    if (flags & GPIO_PULLUP) {
        init.GPIO_PuPd = GPIO_PuPd_UP;
    } else if (flags & GPIO_PULLDOWN) {
        init.GPIO_PuPd = GPIO_PuPd_DOWN;
    }

    if (flags & GPIO_STM32_OD) {
        init.GPIO_OType = GPIO_OType_OD;
    } else {
        init.GPIO_OType = GPIO_OType_PP;
    }

    GPIO_Init(port_to_pointer(port), &init);

    return 0;
}

void gpio_set(unsigned nr, unsigned on) {
    GPIO_WriteBit(port_to_pointer(GPIO_PORT(nr)), 1 << GPIO_PIN(nr), on);
}

int gpio_get(unsigned nr) {
    return GPIO_ReadInputDataBit(port_to_pointer(GPIO_PORT(nr)), 1 << GPIO_PIN(nr));
}

