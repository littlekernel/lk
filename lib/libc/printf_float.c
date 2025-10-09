/*
 * Copyright (c) 2025 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

// Include the printf implementation with floating point.

#if !WITH_NO_FP

#define FLOAT_PRINTF 1

#include "printf.c.inc"
#endif
