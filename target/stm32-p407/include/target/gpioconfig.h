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

#define GPIO_USART3_TX GPIO(GPIO_PORT_D, 8)
#define GPIO_USART3_RX GPIO(GPIO_PORT_D, 9)

#define GPIO_LED0 GPIO(GPIO_PORT_F, 6)
#define GPIO_LED1 GPIO(GPIO_PORT_F, 7)
#define GPIO_LED2 GPIO(GPIO_PORT_F, 8)
#define GPIO_LED3 GPIO(GPIO_PORT_F, 9)

#endif
