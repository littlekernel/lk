/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <arch/arm/cores.h>

#if ARM_ARCH_LEVEL >= 7
#define LOADCONST(reg, c) \
    movw reg, #:lower16: c; \
    movt reg, #:upper16: c
#else
#define LOADCONST(reg, c) ldr   reg, =##c
#endif

