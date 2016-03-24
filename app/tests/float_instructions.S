/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <asm.h>

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
