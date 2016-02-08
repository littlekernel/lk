/*
 * Copyright (c) 2016 Erik Gilling
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
#ifndef __PLATFORM_STM32_DMA_H
#define __PLATFORM_STM32_DMA_H

#include <stdint.h>

typedef enum {
    DMA_CHANNEL_1 = 1,
    DMA_CHANNEL_2 = 2,
    DMA_CHANNEL_3 = 3,
    DMA_CHANNEL_4 = 4,
    DMA_CHANNEL_5 = 5,
    DMA_CHANNEL_6 = 6,
    DMA_CHANNEL_7 = 7,
} dma_channel_t;

#define DMA_CHANNELS 7

#define DMA_FLAG_FROM_PERIPH      (0 << 4)
#define DMA_FLAG_FROM_MEM         (1 << 4)
#define DMA_FLAG_PERIPH_INCREMENT (1 << 6)
#define DMA_FLAG_MEM_INCREMENT    (1 << 7)
#define DMA_FLAG_PERIPH_8_BIT     (0 << 8)
#define DMA_FLAG_PERIPH_16_BIT    (1 << 8)
#define DMA_FLAG_PERIPH_32_BIT    (2 << 8)
#define DMA_FLAG_MEM_8_BIT        (0 << 10)
#define DMA_FLAG_MEM_16_BIT       (1 << 10)
#define DMA_FLAG_MEM_32_BIT       (2 << 10)
#define DMA_FLAG_PRIORITY(x)      (((x) & 0x3) << 12)

/**
 * dmi_init
 *
 * Initialize the DMA peripheral.
 */
void dma_init(void);

/**
 * dma_transfer_start
 *
 * Initiate a DMA transfer.
 *
 * @param[in] chan        DMA channel.
 * @param[in] periph_addr Address of the peripheral register for the transfer.
 * @param[in] mem_addr    Address of the memory of the transfer.
 * @param[in] count       Number of cycles to transfer.
 * @param[in] flags       Flags to control the transfer (see DMA_FLAG_*.)
 */

void dma_transfer_start(dma_channel_t chan,
                        uint32_t periph_addr,
                        uint32_t mem_addr,
                        uint16_t count,
                        uint32_t flags);

/**
 * dma_wait
 *
 * Wait for a DMA transaction to complete.
 * TODO(konkers): Add timeout support.
 * TODO(konkers): Add error reporting.
 *
 * @param[in] chan DMA channel.
 */
void dma_wait(dma_channel_t chan);

#endif  // __PLATFORM_STM32_DMA_H
