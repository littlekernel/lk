/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <platform/zynq.h>

/* configuration for the PL310 L2 cache controller */
#define PL310_BASE L2CACHE_BASE
#define PL310_TAG_RAM_LATENCY ((1 << 8) | (1 << 4) | (1 << 0))
#define PL310_DATA_RAM_LATENCY ((1 << 8) | (2 << 4) | (1 << 0))

