/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdbool.h>

void uart_init(void);

void pic_early_init(void);
void pic_init(void);
void goldfish_rtc_early_init(void);
void goldfish_rtc_init(void);
void goldfish_tty_early_init(void);
void goldfish_tty_init(void);
