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
#include <platform/spi.h>

#include <stdbool.h>

#include <arch/arm/cm.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <platform/dma.h>
#include <platform/rcc.h>

#define SPI_SR_FRLVL_VAL(reg)  ((reg) >> 9 & 0x3)
#define SPI_SR_FTLVL_VAL(reg)  ((reg) >> 11 & 0x3)

#define SPI_FIFO_EMPTY   0x0
#define SPI_FIFO_QUARTER 0x1
#define SPI_FIFO_HALF    0x2
#define SPI_FIFO_FULL    0x3

typedef SPI_TypeDef spi_regs_t;

uint32_t spi_dma_width_flag;
mutex_t spi_mutex;

void spi_init(spi_data_size_t data_size,
              spi_cpol_t cpol,
              spi_cpha_t cpha,
              spi_bit_order_t bit_order,
              spi_prescaler_t prescaler)
{

    spi_regs_t *regs = SPI1;
    uint16_t temp_reg;

    mutex_init(&spi_mutex);

    stm32_rcc_set_enable(STM32_RCC_CLK_SPI1, true);

    regs->CR1 = cpol | cpha | bit_order | prescaler | SPI_CR1_SSM;

    temp_reg = regs->CR2;
    temp_reg &= ~(SPI_CR2_DS | SPI_CR2_FRXTH);
    temp_reg |= data_size;
    if (data_size < SPI_DATA_SIZE_9) {
        // RXNE asserted when fifo has at least 8 bits.  Defaults to 16bits.
        temp_reg |= SPI_CR2_FRXTH;
        spi_dma_width_flag = DMA_FLAG_PERIPH_8_BIT | DMA_FLAG_MEM_8_BIT;
    } else {
        spi_dma_width_flag = DMA_FLAG_PERIPH_16_BIT | DMA_FLAG_MEM_16_BIT;
    }
    regs->CR2 = temp_reg;

    temp_reg = regs->CR1;
    temp_reg |= SPI_CR1_MSTR | SPI_CR1_SSI;
    regs->CR1 = temp_reg;
}

ssize_t spi_xfer(const void *tx_buf, void *rx_buf, size_t len)
{
    // Assure only a single transaction is ever active.
    mutex_acquire(&spi_mutex);

    // Ensure we're idle before starting.
    dma_wait(DMA_CHANNEL_2);
    dma_wait(DMA_CHANNEL_3);

    spi_regs_t *regs = SPI1;
    regs->CR1 &= ~SPI_CR1_SPE;

    // Make sure to start read DMA first.
    dma_transfer_start(DMA_CHANNEL_2,
                       (uint32_t)&regs->DR, (uint32_t)rx_buf, len,
                       DMA_FLAG_FROM_PERIPH | DMA_FLAG_MEM_INCREMENT
                       | DMA_FLAG_PRIORITY(1) | spi_dma_width_flag);
    dma_transfer_start(DMA_CHANNEL_3,
                       (uint32_t)&regs->DR, (uint32_t)tx_buf, len,
                       DMA_FLAG_FROM_MEM | DMA_FLAG_MEM_INCREMENT
                       | DMA_FLAG_PRIORITY(0) | spi_dma_width_flag);
    regs->CR2 |= SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;
    regs->CR1 |= SPI_CR1_SPE;

    // We only wait for channel 2 (RX) assuming that TX will be done first.
    dma_wait(DMA_CHANNEL_2);

    mutex_release(&spi_mutex);
    return len;
}

