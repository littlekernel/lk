/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <sys/types.h>
#include <stdint.h>

/* dcc */
typedef void (*dcc_rx_callback_t)(uint32_t val);

status_t arm_dcc_enable(dcc_rx_callback_t rx_callback);

bool arm_dcc_read_available(void);
ssize_t arm_dcc_read(uint32_t *buf, size_t len, lk_time_t timeout);
ssize_t arm_dcc_write(const uint32_t *buf, size_t len, lk_time_t timeout);

