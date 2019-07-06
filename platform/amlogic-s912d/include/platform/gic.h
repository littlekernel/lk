/*
 * Copyright (c) 2018 The Fuchsia Authors
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <platform/s912d.h>

#define GICBASE(n) (GIC_BASE)
#define GICD_OFFSET (0x1000)
#define GICC_OFFSET (0x2000)

#define MAX_INT 255