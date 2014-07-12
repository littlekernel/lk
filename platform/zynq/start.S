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
#include <platform/zynq.h>

/* this code attempts to remap sram to 0xfffc0000 - 0xffffffff and
   branch the cpu into the equivalent spot. Assumes the cpu is running
   at the initial 0 based mapping */

/* disabled for now */
#if 0

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

