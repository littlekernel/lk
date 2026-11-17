/*
 * Copyright (c) 2019 Travis Geiselbrecht
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
#pragma once

#include <stdint.h>

/* cmos map for PC */
#define CMOS_REG_STATUS_D    (0x0d)
#define CMOS_REG_FLOPPY      (0x10) // drive A type in the high nibble, B in the low
#define CMOS_REG_HARDDISK    (0x12) // disk 0 type in the high nibble, 1 in the low
#define CMOS_REG_EQUIPMENT   (0x14)
#define CMOS_REG_BASE_MEM_LO (0x15) // KB below 1MB, word
#define CMOS_REG_EXT_MEM_LO  (0x17) // KB above 1MB as configured, word
#define CMOS_REG_CHECKSUM_HI (0x2e) // sum of 0x10-0x2d
#define CMOS_REG_CHECKSUM_LO (0x2f)
#define CMOS_REG_POST_EXT_LO (0x30) // KB above 1MB as found by POST, word

uint8_t cmos_read(uint8_t reg);
// little endian word at reg and reg + 1
uint16_t cmos_read_word(uint8_t reg);
