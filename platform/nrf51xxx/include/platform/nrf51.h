/*
 * Copyright (c) 2015 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <platform/nrf518xx.h>

void nrf51_debug_early_init(void);
void nrf51_debug_init(void);
void nrf51_timer_early_init(void);
void nrf51_timer_init(void);
void nrf51_gpio_early_init(void);
void nrf51_flash_nor_early_init(void);
void nrf51_flash_nor_init(void);

