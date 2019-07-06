/*
 * Copyright (c) 2015 Gurjant Kalsi <me@gurjantkalsi.com>
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#ifndef __TARGET_DARTUINOP0_MEMORY_LCD_H_
#define __TARGET_DARTUINOP0_MEMORY_LCD_H_

#include <stm32f7xx.h>

// Initialize the QSPI Flash device.
status_t memory_lcd_init(void);

#endif  // __TARGET_DARTUINOP0_MEMORY_LCD_H_
