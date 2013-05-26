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

#include <err.h>
#include <dev/class/i2c.h>

status_t class_i2c_write(struct device *dev, uint8_t addr, const void *buf, size_t len)
{
	struct i2c_ops *ops = device_get_driver_ops(dev, struct i2c_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->write)
		return ops->write(dev, addr, buf, len);
	else
		return ERR_NOT_SUPPORTED;
}

status_t class_i2c_read(struct device *dev, uint8_t addr, void *buf, size_t len)
{
	struct i2c_ops *ops = device_get_driver_ops(dev, struct i2c_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->read)
		return ops->read(dev, addr, buf, len);
	else
		return ERR_NOT_SUPPORTED;
}

status_t class_i2c_write_reg(struct device *dev, uint8_t addr, uint8_t reg, uint8_t value)
{
	struct i2c_ops *ops = device_get_driver_ops(dev, struct i2c_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->write_reg)
		return ops->write_reg(dev, addr, reg, value);
	else
		return ERR_NOT_SUPPORTED;
}

status_t class_i2c_read_reg(struct device *dev, uint8_t addr, uint8_t reg, void *value)
{
	struct i2c_ops *ops = device_get_driver_ops(dev, struct i2c_ops, std);
	if (!ops)
		return ERR_NOT_CONFIGURED;
	
	if (ops->read_reg)
		return ops->read_reg(dev, addr, reg, value);
	else
		return ERR_NOT_SUPPORTED;
}

