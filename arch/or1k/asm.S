/*
 * Copyright (c) 2015 Stefan Kristiansson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

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
