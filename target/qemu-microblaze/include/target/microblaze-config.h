/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
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

