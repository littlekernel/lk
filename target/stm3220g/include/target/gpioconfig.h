/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __TARGET_GPIOCONFIG_H
#define __TARGET_GPIOCONFIG_H

#include <platform/gpio.h>

#define GPIO_USART3_TX GPIO(GPIO_PORT_C, 10)
#define GPIO_USART3_RX GPIO(GPIO_PORT_C, 11)

#define GPIO_LED0 GPIO(GPIO_PORT_G, 6)
#define GPIO_LED1 GPIO(GPIO_PORT_G, 8)
#define GPIO_LED2 GPIO(GPIO_PORT_I, 9)
#define GPIO_LED3 GPIO(GPIO_PORT_C, 7)

void stm3220g_set_led_bits(unsigned int nr);

#endif
