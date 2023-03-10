/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>

void sifive_uart_write(int c);
int sifive_uart_read(char *c, bool wait);
void sifive_uart_early_init(void);
void sifive_uart_init(void);

void gpio_early_init(void);
void gpio_init(void);


