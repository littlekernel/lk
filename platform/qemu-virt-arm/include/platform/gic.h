/*
 * Copyright (c) 2014-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

// For MAX_INT.
// TODO: read the max interrupt number from the GIC itself and dynamically allocate
// the interrupt table.
#include <platform/qemu-virt.h>

// residual stub for platforms that do not need to define the GIC base addresses
