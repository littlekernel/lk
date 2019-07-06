/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __PLATFORM_P_H
#define __PLATFORM_P_H

void platform_init_interrupts(void);
void platform_init_timer(uint32_t freq);
void zynq_gpio_init(void);

#endif

