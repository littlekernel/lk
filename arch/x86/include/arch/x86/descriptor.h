/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2014 Intel Corporation
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

/*
 * System Selectors
 */
#define NULL_SELECTOR       0x00

/********* x86 selectors *********/
#define CODE_SELECTOR       0x08
#define DATA_SELECTOR       0x10
#define USER_CODE_32_SELECTOR   0x18
#define USER_DATA_32_SELECTOR   0x20

/******* x86-64 selectors ********/
#define CODE_64_SELECTOR    0x28
#define STACK_64_SELECTOR   0x30
#define USER_CODE_64_SELECTOR   0x38
#define USER_DATA_64_SELECTOR   0x40

#define TSS_SELECTOR        0x48

/*
 * Descriptor Types
 */
#define SEG_TYPE_TSS        0x9
#define SEG_TYPE_TSS_BUSY   0xb
#define SEG_TYPE_TASK_GATE  0x5
#define SEG_TYPE_INT_GATE   0xe     // 32 bit
#define SEG_TYPE_DATA_RW    0x2
#define SEG_TYPE_CODE_RW    0xa

#ifndef ASSEMBLY

#include <sys/types.h>

typedef uint16_t seg_sel_t;

void set_global_desc(seg_sel_t sel, void *base, uint32_t limit,
                     uint8_t present, uint8_t ring, uint8_t sys, uint8_t type, uint8_t gran, uint8_t bits);

#endif
