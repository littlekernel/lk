/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <dev/uart.h>

#define PL011_FLAG_DEBUG_UART (1u<<0)

void pl011_init_early(int port, uintptr_t base, uint32_t irq, uint32_t flag);
void pl011_init(int port);