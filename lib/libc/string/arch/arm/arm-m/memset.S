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

/* void bzero(void *s, size_t n); */
FUNCTION(bzero)
    mov     r2, r1
    movs    r1, #0

/* void *memset(void *s, int c, size_t n); */
FUNCTION(memset)
    // save the original pointer
    push    { r0, lr }

    // check for zero length
    cbz     r2, .L_done

    // short memsets aren't worth optimizing and make sure we have
    // enough headroom to try to do dwordwise move optimization
    cmp     r2, #16
    blt     .L_bytewise

    // see how many bytes we need to move to align to dword boundaries
    and     r3, r0, #7
    cbz     r3, .L_prepare_dwordwise
    rsb     r3, #8
    subs    r2, r3

.L_bytewise_align:
    // bytewise to align memset
    subs    r3, r3, #1
    strb    r1, [r0], #1
    bgt     .L_bytewise_align

.L_prepare_dwordwise:
    // fill a pair of 32 bit registers with the 8 bit value
    uxtb    r1, r1
    orr     r1, r1, r1, lsl #8
    orr     r1, r1, r1, lsl #16
    mov     r12, r1

    // load the number of dwords left
    lsrs    r3, r2, #3

.L_dwordwise:
    // dwordwise memset
    subs    r3, r3, #1
    strd    r1, r12, [r0], #8
    bgt     .L_dwordwise

    // remaining bytes
    ands     r2, #7
    beq     .L_done

.L_bytewise:
    // bytewise memset
    subs    r2, r2, #1
    strb    r1, [r0], #1
    bgt     .L_bytewise

.L_done:
    // restore the base pointer as return value
    pop     { r0, pc }

