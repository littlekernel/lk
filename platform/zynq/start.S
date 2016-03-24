/*
 * Copyright (c) 2014-2015 Travis Geiselbrecht
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
#include <platform/zynq.h>

/* code run at the very beginning of the system, attempting to trap the 2nd cpu */
FUNCTION(platform_reset)
    /* figure out our cpu number */
    mrc     p15, 0, r12, c0, c0, 5 /* MPIDR */

    /* mask off the bottom 8 bits to test cpu number */
    ubfx    r12, r12, #0, #8

    /* if we're the 0th cpu, continue to arm_reset */
    teq     r12, #0
    beq     arm_reset

    /* bump the cpu counter */
    adr     r12, __cpu_trapped
    mov     r11, #1
    str     r11, [r12]
    dsb

#if !WITH_SMP
0:
    /* stay trapped here forever */
    wfe
    b       0b
#else
    /* pass on through the reset vector, where the arm arch code will trap the cpu */
    b       arm_reset
#endif

DATA(__cpu_trapped)
    .word     0

#if 0
/* disabled for now */

/* this code attempts to remap sram to 0xfffc0000 - 0xffffffff and
   branch the cpu into the equivalent spot. Assumes the cpu is running
   at the initial 0 based mapping */

/* a spot of the top bank of OCM memory for us to run our code from
   needs to be below where the second cpu is running (0xffffe00-0xfffffff0) */
#define TARGET_SPOT 0xfffff800

/* first piece of code run out of the reset vector. use
   to relocate sram to the final location at 0xfffc0000
   and switch to there */
FUNCTION(platform_reset)
    /* relocate the below code to TARGET_SPOT */
    ldr     r8, =TARGET_SPOT
    adr     r9, .Lcore_reloc_start
    adr     r10, .Lcore_reloc_end

0:
    ldr     r12, [r9], #4
    str     r12, [r8], #4
    cmp     r9, r10
    bne     0b

    /* load constants we will need below */
    ldr     r8, =SLCR_BASE
    ldr     r9, =SCU_CONTROL_BASE

    /* calculate the new return address this code will need to branch to */
    adr     r12, .Ldone
    add     r12, #0xfffc0000

    ldr     r10, =TARGET_SPOT
    bx      r10

.Ldone:
    b       arm_reset

.Lcore_reloc_start:
    # use SCLR to map the sram blocks to the top of their segment
    movw    r10, #SLCR_UNLOCK_KEY
    str     r10, [r8, #SLCR_UNLOCK]

    ldr     r10, [r8, #OCM_CFG]
    orr     r10, #0xf
    str     r10, [r8, #OCM_CFG]

    movw    r10, #SLCR_LOCK_KEY
    str     r10, [r8, #SLCR_LOCK]

    # tell the SCU to not filter first 1MB
    mov     r10, #0
    str     r10, [r9, #0x40] /* SCU filter start address */
    dmb

    bx      r12
.Lcore_reloc_end:

.ltorg
#endif


