/*
 * Copyright (c) 2014-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <platform/qemu-virt.h>

#define GICBASE(n)  (CPUPRIV_BASE_VIRT)
#define GICD_OFFSET (0x00000)
#define GICC_OFFSET (0x10000)

