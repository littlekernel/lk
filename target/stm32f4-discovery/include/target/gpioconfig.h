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

#define GPIO_USART2_TX GPIO(GPIO_PORT_A, 2)
#define GPIO_USART2_RX GPIO(GPIO_PORT_A, 3)

#define GPIO_LED0 GPIO(GPIO_PORT_D, 13) // LD3 - Orange
#define GPIO_LED1 GPIO(GPIO_PORT_D, 12) // LD4 - Green
#define GPIO_LED2 GPIO(GPIO_PORT_D, 14) // LD5 - Red
#define GPIO_LED3 GPIO(GPIO_PORT_D, 15) // LD6 - Blue

#endif
