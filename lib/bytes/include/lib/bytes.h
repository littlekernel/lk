/*
 * Copyright (c) 2013 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <endian.h>
#include <stdint.h>

#if BYTE_ORDER == BIG_ENDIAN
#define bytes_read_u16(x)       bytes_read_u16_be(x)
#define bytes_read_u24(x)       bytes_read_u24_be(x)
#define bytes_read_u32(x)       bytes_read_u32_be(x)
#define bytes_write_u16(x, y)   bytes_write_u16_be(x, y)
#define bytes_write_u24(x, y)   bytes_write_u24_be(x, y)
#define bytes_write_u32(x, y)   bytes_write_u32_be(x, y)
#elif BYTE_ORDER == LITTLE_ENDIAN
#define bytes_read_u16(x)       bytes_read_u16_le(x)
#define bytes_read_u24(x)       bytes_read_u24_le(x)
#define bytes_read_u32(x)       bytes_read_u32_le(x)
#define bytes_write_u16(x, y)   bytes_write_u16_le(x, y)
#define bytes_write_u24(x, y)   bytes_write_u24_le(x, y)
#define bytes_write_u32(x, y)   bytes_write_u32_le(x, y)
#else
#error "Endianness not defined!"
#endif


// Big endian interfaces
uint16_t bytes_read_u16_be(const uint8_t *buf);
uint32_t bytes_read_u24_be(const uint8_t *buf);
uint32_t bytes_read_u32_be(const uint8_t *buf);

uint8_t *bytes_write_u16_be(uint8_t *buf, uint16_t val);
uint8_t *bytes_write_u24_be(uint8_t *buf, uint32_t val);
uint8_t *bytes_write_u32_be(uint8_t *buf, uint32_t val);

// Little endian interfaces
uint16_t bytes_read_u16_le(const uint8_t *buf);
uint32_t bytes_read_u24_le(const uint8_t *buf);
uint32_t bytes_read_u32_le(const uint8_t *buf);

uint8_t *bytes_write_u16_le(uint8_t *buf, uint16_t val);
uint8_t *bytes_write_u24_le(uint8_t *buf, uint32_t val);
uint8_t *bytes_write_u32_le(uint8_t *buf, uint32_t val);

// Bit swapping interfaces
uint8_t  bytes_swap_bits_u8(uint8_t val);
uint16_t bytes_swap_bits_u16(uint16_t val);
uint32_t bytes_swap_bits_u24(uint32_t val);
uint32_t bytes_swap_bits_u32(uint32_t val);

