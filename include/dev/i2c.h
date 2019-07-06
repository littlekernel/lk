/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>

#include <stddef.h>     // size_t
#include <sys/types.h>  // status_t

void i2c_init(void);
void i2c_init_early(void);

/* Notes about I2C addresses
 * + Currently, there is no API support for 10-bit I2C addresses.  7-bit only.
 * + Addresses should be supplied as the actual 7-bit address of the device.
 *   The platform driver is responsible for left shifting this address by 1 and
 *   suppling the appropriate value for the R/W bit.
 */

/* send and receive blocks of data */
status_t i2c_transmit(int bus, uint8_t address, const void *buf, size_t count);
status_t i2c_receive(int bus, uint8_t address, void *buf, size_t count);

/* A few convenience routines based on the usual way of accessing registers on
 * i2c slave devices.
 */
status_t i2c_write_reg_bytes(int bus, uint8_t address, uint8_t reg, const uint8_t *val, size_t cnt);
status_t i2c_read_reg_bytes(int bus, uint8_t address, uint8_t reg, uint8_t *val, size_t cnt);

static inline status_t i2c_write_reg(int bus, uint8_t address, uint8_t reg, uint8_t val) {
    return i2c_write_reg_bytes(bus, address, reg, &val, 1);
}

static inline status_t i2c_read_reg(int bus, uint8_t address, uint8_t reg, uint8_t *val) {
    return i2c_read_reg_bytes(bus, address, reg, val, 1);
}
