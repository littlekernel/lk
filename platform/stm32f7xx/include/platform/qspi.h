/*
 * Copyright (c) 2015 Gurjant Kalsi <me@gurjantkalsi.com>
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stm32f7xx.h>

// Initialize the QSPI Flash device.
status_t qspi_flash_init(size_t flash_size);

