/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <platform/alterasoc.h>

#define GICBASE(n)  (CPUPRIV_BASE)
#define GICC_OFFSET (0x0100)
#define GICD_OFFSET (0x1000)

