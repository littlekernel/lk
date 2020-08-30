/*
 * Copyright (c) 2016 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <nrfx.h>

void nrf52_debug_early_init(void);
void nrf52_debug_init(void);
void nrf52_timer_early_init(void);
void nrf52_timer_init(void);
void nrf52_gpio_early_init(void);
void nrf52_flash_nor_early_init(void);
void nrf52_flash_nor_init(void);

