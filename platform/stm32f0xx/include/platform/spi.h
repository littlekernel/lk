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
#ifndef __PLATFORM_STM32_SPI_H
#define __PLATFORM_STM32_SPI_H

#include <sys/types.h>

#include <stm32f0xx.h>

typedef enum {
    SPI_PRESCALER_2 =   0 << 3,
    SPI_PRESCALER_4 =   1 << 3,
    SPI_PRESCALER_8 =   2 << 3,
    SPI_PRESCALER_16 =  3 << 3,
    SPI_PRESCALER_32 =  4 << 3,
    SPI_PRESCALER_64 =  5 << 3,
    SPI_PRESCALER_128 = 6 << 3,
    SPI_PRESCALER_256 = 7 << 3,
} spi_prescaler_t;

#define SPI_DATA_SIZE(x) ((((x) - 1) & 0xf) << 8)
typedef enum {
    SPI_DATA_SIZE_4 = SPI_DATA_SIZE(4),
    SPI_DATA_SIZE_5 = SPI_DATA_SIZE(5),
    SPI_DATA_SIZE_6 = SPI_DATA_SIZE(6),
    SPI_DATA_SIZE_7 = SPI_DATA_SIZE(7),
    SPI_DATA_SIZE_8 = SPI_DATA_SIZE(8),
    SPI_DATA_SIZE_9 = SPI_DATA_SIZE(9),
    SPI_DATA_SIZE_10 = SPI_DATA_SIZE(10),
    SPI_DATA_SIZE_11 = SPI_DATA_SIZE(11),
    SPI_DATA_SIZE_12 = SPI_DATA_SIZE(12),
    SPI_DATA_SIZE_13 = SPI_DATA_SIZE(13),
    SPI_DATA_SIZE_14 = SPI_DATA_SIZE(14),
    SPI_DATA_SIZE_15 = SPI_DATA_SIZE(15),
    SPI_DATA_SIZE_16 = SPI_DATA_SIZE(16),
} spi_data_size_t;

typedef enum {
    SPI_CPOL_LOW =  0x0,
    SPI_CPOL_HIGH = SPI_CR1_CPOL,
} spi_cpol_t;

typedef enum {
    SPI_CPHA_0 = 0x0,  // Data capture on first clock transition.
    SPI_CPHA_1 = SPI_CR1_CPHA,  // Data capture on second clock transition.
} spi_cpha_t;

typedef enum {
    SPI_BIT_ORDER_MSB = 0,
    SPI_BIT_ORDER_LSB = SPI_CR1_LSBFIRST,
} spi_bit_order_t;

/**
 * spi_init
 *
 * Initialize the SPI peripheral.  NOTE: ONLY SPI1 IS SUPPORTED.
 *
 * @param[in] data_size Size of SPI frame.
 * @param[in] cpol      Clock polarity.
 * @param[in] cpha      Clock phase.
 * @param[in] bit_order Which order to transmit the bits on the data lines.
 * @param[in] prescaler Divides the main peripheral clock to generate SCK.
 */
void spi_init(spi_data_size_t data_size,
              spi_cpol_t cpol,
              spi_cpha_t cpha,
              spi_bit_order_t bit_order,
              spi_prescaler_t prescaler);

/**
 * spi_xfer
 *
 * Preform a SPI transfer.  Blocks until finished.
 * TODO(konkers): Support write-only and read-only transactions.
 * TODO(konkers): Add timeout support.
 * TODO(konkers): Report errors.
 *
 * @param[in] tx_buf Buffer to transmit.
 * @param[in] rx_buf Buffer to store received data.
 * @param[in] len    Number of SPI frames to transmit.
 *
 * @return Number of frames transmitted on success, negative error code on
 * failure.
 */
ssize_t spi_xfer(const void *tx_buf, void *rx_buf, size_t len);

#endif  // __PLATFORM_STM32_SPI_H
