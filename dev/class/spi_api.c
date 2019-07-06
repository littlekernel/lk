/*
 * Copyright (c) 2013 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <dev/class/spi.h>

ssize_t class_spi_transaction(struct device *dev, struct spi_transaction *txn, size_t count) {
    struct spi_ops *ops = device_get_driver_ops(dev, struct spi_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->transaction)
        return ops->transaction(dev, txn, count);
    else
        return ERR_NOT_SUPPORTED;
}

