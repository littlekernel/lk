/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>
#include <arch/arm/cores.h>

.text
.align 2

/* void bzero(void *s, size_t n); */
FUNCTION(bzero)
    mov     r2, r1
    mov     r1, #0

/* void *memset(void *s, int c, size_t n); */
FUNCTION(memset)
    // check for zero length
    cmp     r2, #0
    bxeq    lr

    // save the original pointer
    mov     r12, r0

    // short memsets aren't worth optimizing
    cmp     r2, #(32 + 16)
    blt     .L_bytewise

    // fill a 32 bit register with the 8 bit value
    and     r1, r1, #0xff
    orr     r1, r1, r1, lsl #8
    orr     r1, r1, r1, lsl #16

    // check for 16 byte alignment
    tst     r0, #15
    bne     .L_not16bytealigned

.L_bigset:
    // dump some registers to make space for our values
    stmfd   sp!, { r4-r5 }

    // fill a bunch of registers with the set value
    mov     r3, r1
    mov     r4, r1
    mov     r5, r1

    // prepare the count register so we can avoid an extra compare
    sub     r2, r2, #32

    // 32 bytes at a time
.L_bigset_loop:
    stmia   r0!, { r1, r3, r4, r5 }
    subs    r2, r2, #32
    stmia   r0!, { r1, r3, r4, r5 }
    bge     .L_bigset_loop

    // restore our dumped registers
    ldmfd   sp!, { r4-r5 }

    // see if we're done
    adds    r2, r2, #32
    beq     .L_done

.L_bytewise:
    // bytewise memset
    subs    r2, r2, #1
    strb    r1, [r0], #1
    bgt     .L_bytewise

.L_done:
    // restore the base pointer as return value
    mov     r0, r12
    bx      lr

.L_not16bytealigned:
    // dst is not 16 byte aligned, so we will set up to 15 bytes to get it aligned.

    // set the condition flags based on the alignment.
    lsl     r3, r0, #28
    rsb     r3, r3, #0
    msr     CPSR_f, r3             // move into NZCV fields in CPSR

    // move as many bytes as necessary to get the dst aligned
    strvsb  r1, [r0], #1            // V set
    strcsh  r1, [r0], #2            // C set
    streq   r1, [r0], #4            // Z set
    strmi   r1, [r0], #4            // N set
    strmi   r1, [r0], #4            // N set

    // fix the remaining len
    sub     r2, r2, r3, lsr #28

    // do the large memset
    b       .L_bigset

