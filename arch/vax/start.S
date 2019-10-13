/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
.section .text.boot

.globl _start
_start:
    nop
    nop

    // save our bootargs
    moval   bootargs_end,%sp
    pushr   $0x1fff

    // see if we need to be relocated
    movab   _start, %r0     // where we are
    movl    $_start, %r1     // where we want to be
    cmpl    %r0,%r1
    beql    relocated

    // compute the copy length
    subl3   %r1, $__data_end, %r2

    // copy us down to the final location
1:
    movb    (%r0)+,(%r1)+
    sobgtr  %r2,1b

    // zero bss
    subl3   $_end, $__data_end, %r2
1:
    movb    $0,(%r1)+
    sobgtr  %r2,1b

    // branch to our new spot
    movl    $relocated, %r0
    jmp     (%r0)

relocated:
    // set the stack to the end of us
    moval    stack_end,%sp

    // branch into main and we're done
    calls   $0, lk_main
    halt

.section .data
.align 4
.globl bootargs
bootargs:
    .skip 13*4
bootargs_end:

.section .bss
.align 4
stack:
    .skip 1024
stack_end:
