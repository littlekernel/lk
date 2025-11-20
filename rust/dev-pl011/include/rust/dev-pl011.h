/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>

// Set this flag if the UART is a debug UART, which routes input
// directly into the console buffer if present.
#define PL011_FLAG_DEBUG_UART (1u << 0)

struct pl011_config {
    uintptr_t base;
    uint32_t irq;
    uint32_t flag;
};

// pl011 specific initialization routines called from platform code.
// The driver will otherwise implement the uart_* interface.
void pl011_init_early(int port, const struct pl011_config *config);
void pl011_init(int port);