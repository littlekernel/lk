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

#if RISCV_FPU

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

    // put the hardware in the initial state
    // FS[1:0] == 1 set in two steps: one to set bit 0, second one to clear bit 1
    // this ensures it doesn't go through the disabled state (00)
    li  a0, (1 << 13)
    csrs RISCV_CSR_XSTATUS, a0
    li   a0, (1 << 14)
    csrc RISCV_CSR_XSTATUS, a0

    ret
END_FUNCTION(riscv_fpu_zero)

// void riscv_fpu_save(struct riscv_fpu_state *state);
FUNCTION(riscv_fpu_save)
    fsd  f0, 0*8(a0)
    fsd  f1, 1*8(a0)
    fsd  f2, 2*8(a0)
    fsd  f3, 3*8(a0)
    fsd  f4, 4*8(a0)
    fsd  f5, 5*8(a0)
    fsd  f6, 6*8(a0)
    fsd  f7, 7*8(a0)
    fsd  f8, 8*8(a0)
    fsd  f9, 9*8(a0)
    fsd  f10, 10*8(a0)
    fsd  f11, 11*8(a0)
    fsd  f12, 12*8(a0)
    fsd  f13, 13*8(a0)
    fsd  f14, 14*8(a0)
    fsd  f15, 15*8(a0)
    fsd  f16, 16*8(a0)
    fsd  f17, 17*8(a0)
    fsd  f18, 18*8(a0)
    fsd  f19, 19*8(a0)
    fsd  f20, 20*8(a0)
    fsd  f21, 21*8(a0)
    fsd  f22, 22*8(a0)
    fsd  f23, 23*8(a0)
    fsd  f24, 24*8(a0)
    fsd  f25, 25*8(a0)
    fsd  f26, 26*8(a0)
    fsd  f27, 27*8(a0)
    fsd  f28, 28*8(a0)
    fsd  f29, 29*8(a0)
    fsd  f30, 30*8(a0)
    fsd  f31, 31*8(a0)
    csrr a1, fcsr
    sw   a1, 32*8(a0)
    ret
END_FUNCTION(riscv_fpu_save)

// void riscv_fpu_restore(struct riscv_fpu_state *state);
FUNCTION(riscv_fpu_restore)
    fld  f0, 0*8(a0)
    fld  f1, 1*8(a0)
    fld  f2, 2*8(a0)
    fld  f3, 3*8(a0)
    fld  f4, 4*8(a0)
    fld  f5, 5*8(a0)
    fld  f6, 6*8(a0)
    fld  f7, 7*8(a0)
    fld  f8, 8*8(a0)
    fld  f9, 9*8(a0)
    fld  f10, 10*8(a0)
    fld  f11, 11*8(a0)
    fld  f12, 12*8(a0)
    fld  f13, 13*8(a0)
    fld  f14, 14*8(a0)
    fld  f15, 15*8(a0)
    fld  f16, 16*8(a0)
    fld  f17, 17*8(a0)
    fld  f18, 18*8(a0)
    fld  f19, 19*8(a0)
    fld  f20, 20*8(a0)
    fld  f21, 21*8(a0)
    fld  f22, 22*8(a0)
    fld  f23, 23*8(a0)
    fld  f24, 24*8(a0)
    fld  f25, 25*8(a0)
    fld  f26, 26*8(a0)
    fld  f27, 27*8(a0)
    fld  f28, 28*8(a0)
    fld  f29, 29*8(a0)
    fld  f30, 30*8(a0)
    fld  f31, 31*8(a0)
    lw   a1, 32*8(a0)
    csrw fcsr, a1
    ret
END_FUNCTION(riscv_fpu_restore)
#endif // RISCV_FPU
