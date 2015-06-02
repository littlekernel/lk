/*
 * Copyright (c) 2015 Travis Geiselbrecht
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

/* in memory and DCC descriptors for the PDCC protocol */

/* shared outside of lk repository, be careful of modifications */
#define PDCC_VERSION 1

struct pdcc_buffer_descriptor {
    uint32_t version;

    uint32_t htod_buffer_phys;
    uint32_t htod_buffer_len;

    uint32_t dtoh_buffer_phys;
    uint32_t dtoh_buffer_len;
};

#define PDCC_VALID (1<<31)
#define PDCC_OPCODE_SHIFT (24)
#define PDCC_OPCODE(x) (((x) >> PDCC_OPCODE_SHIFT) & 0x7f)
#define PDCC_DATA(x) ((x) & 0x00ffffff);

enum {
    PDCC_OP_RESET = 0,
    PDCC_OP_BUF_HEADER,
    PDCC_OP_UPDATE_OUT_INDEX,
    PDCC_OP_CONSUMED_IN,
};

