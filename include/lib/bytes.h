/*
 * Copyright (c) 2013 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __BYTES_H
#define __BYTES_H

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

#endif

