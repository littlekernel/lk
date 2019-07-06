/*
 * Copyright (c) 2013 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <dev/class/i2c.h>

status_t class_i2c_write(struct device *dev, uint8_t addr, const void *buf, size_t len) {
    struct i2c_ops *ops = device_get_driver_ops(dev, struct i2c_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->write)
        return ops->write(dev, addr, buf, len);
    else
        return ERR_NOT_SUPPORTED;
}

status_t class_i2c_read(struct device *dev, uint8_t addr, void *buf, size_t len) {
    struct i2c_ops *ops = device_get_driver_ops(dev, struct i2c_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->read)
        return ops->read(dev, addr, buf, len);
    else
        return ERR_NOT_SUPPORTED;
}

status_t class_i2c_write_reg(struct device *dev, uint8_t addr, uint8_t reg, uint8_t value) {
    struct i2c_ops *ops = device_get_driver_ops(dev, struct i2c_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->write_reg)
        return ops->write_reg(dev, addr, reg, value);
    else
        return ERR_NOT_SUPPORTED;
}

status_t class_i2c_read_reg(struct device *dev, uint8_t addr, uint8_t reg, void *value) {
    struct i2c_ops *ops = device_get_driver_ops(dev, struct i2c_ops, std);
    if (!ops)
        return ERR_NOT_CONFIGURED;

    if (ops->read_reg)
        return ops->read_reg(dev, addr, reg, value);
    else
        return ERR_NOT_SUPPORTED;
}

