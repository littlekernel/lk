/*
 * Copyright (c) 2013 Google Inc.
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

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

