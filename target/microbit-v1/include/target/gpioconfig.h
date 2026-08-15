/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <platform/gpio.h>

/* UART to the on board KL26 interface chip, which bridges it to USB CDC */
#define UART0_TX_PIN    24
#define UART0_RX_PIN    25
