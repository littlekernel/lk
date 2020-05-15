/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

// based on 32 or 64bit register widths, select the 32 or 64 bit
// wide load/stores
#if __riscv_xlen == 32
#define REGOFF(x) ((x) * 4)
#define STR       sw
#define LDR       lw
#else
#define REGOFF(x) ((x) * 8)
#define STR       sd
#define LDR       ld
#endif

#define RISCV_XLEN_BYTES (__riscv_xlen / 8)

