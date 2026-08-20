/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <platform/gpio.h>

#define GPIO_LED0 GPIO(GPIO_PORT_B, 0)  /* LD1, green */
#define GPIO_LED1 GPIO(GPIO_PORT_F, 4)  /* LD2, yellow */
#define GPIO_LED2 GPIO(GPIO_PORT_G, 4)  /* LD3, red */

#define GPIO_BUTTON GPIO(GPIO_PORT_C, 13)  /* USER_PB */
