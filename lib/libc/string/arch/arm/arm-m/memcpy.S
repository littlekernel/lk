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
#include <arch/arm/cores.h>

.text
.syntax unified
.thumb
.align 2

/* void bcopy(const void *src, void *dest, size_t n); */
FUNCTION(bcopy)
    // swap args for bcopy
    mov     r12, r0
    mov     r0, r1
    mov     r1, r12

/* void *memcpy(void *dest, const void *src, size_t count) */
FUNCTION(memcpy)
    push    { r0, r14 }

    // test for zero length or pointers being equivalent
    cbz     r2, .L_done
    cmp     r0, r1
    beq     .L_done

    // check for a short copy len
    cmp     r2, #16
    blt     .L_bytewise

    // check to see if the pointers are similarly dword aligned
    eors    r3, r0, r1
    ands    r3, #7
    beq     .L_prepare_dword

    // see how many bytes we need to move to align dest to word boundary
    and     r3, r0, #3
    cbz     r3, .L_prepare_wordwise
    rsb     r3, #4
    subs    r2, r3

    .align 2
.L_bytewise_align:
    // bytewise to align memcpy
    ldrb    r12, [r1], #1
    subs    r3, r3, #1
    strb    r12, [r0], #1
    bgt     .L_bytewise_align

.L_prepare_wordwise:
    // load the number of words left
    lsrs    r3, r2, #2

    .align 2
.L_wordwise:
    // wordwise copy
    ldr     r12, [r1], #4
    subs    r3, r3, #1
    str     r12, [r0], #4
    bgt     .L_wordwise

    // remaining bytes
    ands     r2, #3
    beq     .L_done

    .align 2
.L_bytewise:
    // simple bytewise copy
    ldrb    r12, [r1], #1
    subs    r2, r2, #1
    strb    r12, [r0], #1
    bgt     .L_bytewise

.L_done:
    pop     { r0, pc }

// Handle copying by dword (8 bytes at a time) increments
.L_prepare_dword:
    // see how many bytes we need to move to align dest to dword boundary
    and     r3, r0, #7
    cbz     r3, .L_prepare_dwordwise
    rsb     r3, #8
    subs    r2, r3

    .align 2
.L_bytewise_align_dword:
    // bytewise to align memcpy
    ldrb    r12, [r1], #1
    subs    r3, r3, #1
    strb    r12, [r0], #1
    bgt     .L_bytewise_align_dword

.L_prepare_dwordwise:
    // load the number of dwords left
    lsrs    r3, r2, #3

    push    { r5 }

    .align 2
.L_dwordwise:
    // dwordwise copy
    ldrd    r5, r12, [r1], #8
    subs    r3, r3, #1
    strd    r5, r12, [r0], #8
    bgt     .L_dwordwise

    pop     { r5 }

    // remaining bytes
    ands     r2, #7
    beq     .L_done

    // finish the remaining bytes and exit
    b       .L_bytewise

