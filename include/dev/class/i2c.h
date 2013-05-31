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
#ifndef __DEV_CLASS_I2C_H
#define __DEV_CLASS_I2C_H

#include <compiler.h>
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

#endif
