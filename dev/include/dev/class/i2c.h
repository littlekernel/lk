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

/* i2c interface */
struct i2c_ops {
    struct driver_ops std;

    status_t (*write)(struct device *dev, uint8_t addr, const void *buf, size_t len);
    status_t (*read)(struct device *dev, uint8_t addr, void *buf, size_t len);

    status_t (*write_reg)(struct device *dev, uint8_t addr, uint8_t reg, uint8_t value);
    status_t (*read_reg)(struct device *dev, uint8_t addr, uint8_t reg, void *value);
};

__BEGIN_CDECLS

status_t class_i2c_write(struct device *dev, uint8_t addr, const void *buf, size_t len);
status_t class_i2c_read(struct device *dev, uint8_t addr, void *buf, size_t len);
status_t class_i2c_write_reg(struct device *dev, uint8_t addr, uint8_t reg, uint8_t value);
status_t class_i2c_read_reg(struct device *dev, uint8_t addr, uint8_t reg, void *value);

__END_CDECLS

