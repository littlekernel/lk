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

/* qemu emulates a petalogix s3adsp1800 */
#define LMB_BRAM_SIZE       (128 * 1024)
#define FLASH_SIZE          (16 * 1024 * 1024)

#define ETHLITE_BASEADDR    0x81000000
#define INTC_BASEADDR       0x81800000
#define TIMER_BASEADDR      0x83c00000
#define UARTLITE_BASEADDR   0x84000000
#define MEMORY_BASEADDR     0x90000000
#define FLASH_BASEADDR      0xa0000000

#define TIMER_IRQ           0
#define ETHLITE_IRQ         1
#define UARTLITE_IRQ        3
#define MAX_INT             4

#define TIMER_RATE (62*1000000)

