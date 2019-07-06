/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>

/* include all of the ST driver library here */
#include <stm32f7xx.h>

void stm32_debug_early_init(void);
void stm32_debug_init(void);
void stm32_timer_early_init(void);
void stm32_timer_init(void);
void stm32_gpio_early_init(void);
void stm32_flash_early_init(void);
void stm32_flash_init(void);
void stm32_usbc_early_init(void);
void stm32_usbc_init(void);

int stm32_uart_getc_poll(int port);


/* unique id of device */
extern uint32_t stm32_unique_id[3];
