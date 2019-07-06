/*
 * Copyright (c) 2013 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <dev/class/block.h>

ssize_t class_block_get_size(struct device *dev) {
    struct block_ops *ops = device_get_driver_ops(dev, struct block_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->get_block_size)
        return ops->get_block_size(dev);
    else
        return ERR_NOT_SUPPORTED;
}

ssize_t class_block_get_count(struct device *dev) {
    struct block_ops *ops = device_get_driver_ops(dev, struct block_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->get_block_count)
        return ops->get_block_count(dev);
    else
        return ERR_NOT_SUPPORTED;
}

ssize_t class_block_write(struct device *dev, off_t offset, const void *buf, size_t count) {
    struct block_ops *ops = device_get_driver_ops(dev, struct block_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->write)
        return ops->write(dev, offset, buf, count);
    else
        return ERR_NOT_SUPPORTED;
}

ssize_t class_block_read(struct device *dev, off_t offset, void *buf, size_t count) {
    struct block_ops *ops = device_get_driver_ops(dev, struct block_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->read)
        return ops->read(dev, offset, buf, count);
    else
        return ERR_NOT_SUPPORTED;
}

status_t class_block_flush(struct device *dev) {
    struct block_ops *ops = device_get_driver_ops(dev, struct block_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->flush)
        return ops->flush(dev);
    else
        return ERR_NOT_SUPPORTED;
}

