/*
 * Copyright (c) 2013 Corey Tabaka
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
#ifndef __DEV_CLASS_SPI_H
#define __DEV_CLASS_SPI_H

#include <compiler.h>
#include <dev/driver.h>

/* spi transaction flags */
enum spi_flags {
	SPI_READ        = (1<<0),
	SPI_WRITE       = (1<<1),
	SPI_CS_ASSERT   = (1<<2),
	SPI_CS_DEASSERT = (1<<3),
};

/* spi transaction */
struct spi_transaction {
	enum spi_flags flags;
	void *tx_buf;
	void *rx_buf;
	size_t len;
};

/* spi interface */
struct spi_ops {
	struct driver_ops std;

	ssize_t (*transaction)(struct device *dev, struct spi_transaction *txn, size_t count);
};

__BEGIN_CDECLS

ssize_t class_spi_transaction(struct device *dev, struct spi_transaction *txn, size_t count);

__END_CDECLS

#endif

