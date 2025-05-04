/*
 * Copyright (c) 2025 Mykola Hohsadze
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <platform/fvp-base.h>

#define GICBASE(n)  (PERIPHERAL_BASE_VIRT + 0x2c000000UL)
#define GICD_OFFSET (0x03000000)
#define GICC_OFFSET (0x0)
