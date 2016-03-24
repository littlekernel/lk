/*
 * Copyright (c) 2013 Google Inc.
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
#ifndef __GPIO_I2C__
#define __GPIO_I2C__

#include <stdint.h>
#include <dev/i2c.h>

typedef struct gpio_i2c_info {
    const int sda;
    const int scl;
    uint32_t  hcd;  /* 1/2 I2C clock delay in cpu cycles */
    uint32_t  qcd;  /* 1/4 I2C clock delay in cpu cycles */
} gpio_i2c_info_t;

#define DEFINE_GPIO_I2C(_name,                 \
                        _sda_gpio_id,          \
                        _scl_gpio_id,          \
                        _clk_ticks)            \
static const gpio_i2c_info_t _name = {         \
    .sda            = _sda_gpio_id,            \
    .scl            = _scl_gpio_id,            \
    .hcd            = ((_clk_ticks + 1) >> 1), \
    .qcd            = ((_clk_ticks + 3) >> 2), \
}

void gpio_i2c_add_bus(uint32_t bus_id, const gpio_i2c_info_t *info);

void gpio_i2c_init_early(void);
void gpio_i2c_init(void);
status_t gpio_i2c_transmit(int, uint8_t, const void *, size_t);
status_t gpio_i2c_receive(int, uint8_t, void *, size_t);
status_t gpio_i2c_write_reg_bytes(int, uint8_t, uint8_t, const uint8_t *, size_t);
status_t gpio_i2c_read_reg_bytes(int, uint8_t, uint8_t, uint8_t *, size_t);

#endif  // __GPIO_I2C__
