/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

#if ARM_WITH_VFP

.fpu neon
.syntax unified

.macro disable, scratchreg
    vmrs  \scratchreg, fpexc
    bic   \scratchreg, #(1<<30)
    vmsr  fpexc, \scratchreg
.endm

.macro vfp_instructions
    disable r12
    vadd.f32 s0, s0, s0

    disable r12
    vadd.f64 d0, d0, d0

    disable r12
    ldr     r0, =float_test_scratch
    vldr    s0, [r0]
.endm

.macro neon_instructions
    disable r12
    vadd.f32 q0, q0, q0

    disable r12
    ldr     r0, =float_test_scratch
    vld1.f32 { q0 }, [r0]

    disable r12
    vmov    s0, r0
.endm

#if !ARM_ONLY_THUMB
.arm

FUNCTION(float_vfp_arm_instruction_test)
    vfp_instructions
    bx  lr

FUNCTION(float_neon_arm_instruction_test)
    neon_instructions
    bx  lr
#endif

.thumb

FUNCTION(float_vfp_thumb_instruction_test)
    vfp_instructions
    bx  lr

FUNCTION(float_neon_thumb_instruction_test)
    neon_instructions
    bx  lr

.data
LOCAL_DATA(float_test_scratch)
    .word 0
    .word 0

#endif // ARM_WITH_VFP
