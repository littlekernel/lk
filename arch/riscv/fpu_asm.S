/*
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>
#include <arch/riscv.h>
#include <arch/riscv/asm.h>

// enable full use of all of the fpu instructions
#if __riscv_xlen == 32
.attribute arch, "rv32imafdc"
#elif __riscv_xlen == 64
.attribute arch, "rv64imafdc"
#else
#error unknown xlen
#endif

// conditionally use fcvt or fmv based on 32 or 64bit ISA
.macro ZERO_FPU_REG reg, width
#if __riscv_xlen == 32
    fcvt.\width\().w \reg, zero
#elif __riscv_xlen == 64
    fmv.\width\().x \reg, zero
#endif
.endm

// called just before entering user space for the first time.
// must not use the stack and is okay to be called with interrupts disabled.
FUNCTION(riscv_fpu_zero)
#if RISCV_FPU
    // zero out the fpu state
    // TODO: handle single precision implementations
    csrw fcsr, zero
    ZERO_FPU_REG f0, d
    ZERO_FPU_REG f1, d
    ZERO_FPU_REG f2, d
    ZERO_FPU_REG f3, d
    ZERO_FPU_REG f4, d
    ZERO_FPU_REG f5, d
    ZERO_FPU_REG f6, d
    ZERO_FPU_REG f7, d
    ZERO_FPU_REG f8, d
    ZERO_FPU_REG f9, d
    ZERO_FPU_REG f10, d
    ZERO_FPU_REG f11, d
    ZERO_FPU_REG f12, d
    ZERO_FPU_REG f13, d
    ZERO_FPU_REG f14, d
    ZERO_FPU_REG f15, d
    ZERO_FPU_REG f16, d
    ZERO_FPU_REG f17, d
    ZERO_FPU_REG f18, d
    ZERO_FPU_REG f19, d
    ZERO_FPU_REG f20, d
    ZERO_FPU_REG f21, d
    ZERO_FPU_REG f22, d
    ZERO_FPU_REG f23, d
    ZERO_FPU_REG f24, d
    ZERO_FPU_REG f25, d
    ZERO_FPU_REG f26, d
    ZERO_FPU_REG f27, d
    ZERO_FPU_REG f28, d
    ZERO_FPU_REG f29, d
    ZERO_FPU_REG f30, d
    ZERO_FPU_REG f31, d
#endif
    ret
END_FUNCTION(riscv_fpu_zero)

