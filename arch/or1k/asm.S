/*
 * Copyright (c) 2015 Stefan Kristiansson
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

/* void or1k_context_switch(
    struct or1k_context_switch_frame *oldcs,
    struct or1k_context_switch_frame *newcs); */
FUNCTION(or1k_context_switch)
    /* save old context */
    l.sw    0(r3), r1
    l.sw    4(r3), r2
    l.sw    8(r3), r9
    l.sw    12(r3), r10
    l.sw    16(r3), r14
    l.sw    20(r3), r16
    l.sw    24(r3), r18
    l.sw    28(r3), r20
    l.sw    32(r3), r22
    l.sw    36(r3), r24
    l.sw    40(r3), r26
    l.sw    44(r3), r28
    l.sw    48(r3), r30

    /* restore new context */
    l.lwz   r30, 48(r4)
    l.lwz   r28, 44(r4)
    l.lwz   r26, 40(r4)
    l.lwz   r24, 36(r4)
    l.lwz   r22, 32(r4)
    l.lwz   r20, 28(r4)
    l.lwz   r18, 24(r4)
    l.lwz   r16, 20(r4)
    l.lwz   r14, 16(r4)
    l.lwz   r10, 12(r4)
    l.lwz   r9, 8(r4)
    l.lwz   r2, 4(r4)
    l.jr    r9
     l.lwz  r1, 0(r4)
