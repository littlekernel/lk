/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>

/*
 * The plain mips target for qemu has an emulated PC style UART mapped
 * into the ISA io port apterture at 0x14000000
 */
#define ISA_IO_BASE ((volatile uint8_t *)0x14000000 + 0x80000000)
#define UART_PORT_BASE (0x3f8)

static inline void isa_write_8(uint16_t port, uint8_t val) {
    volatile uint8_t *addr = ISA_IO_BASE + port;

    *addr = val;
}

static inline uint8_t isa_read_8(uint16_t port) {
    volatile uint8_t *addr = ISA_IO_BASE + port;

    return *addr;
}

#define INT_VECTORS 8
