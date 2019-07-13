/*
 * Copyright (c) 2013 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
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

