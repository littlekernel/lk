/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>

.section .text.boot
FUNCTION(_start)
    // load the first 4 args that were pushed on whatever stack we have
    // NOTE: assumes stack is pointing at at least readable memory
    movl    %sp@(4),%d0
    movl    %sp@(8),%d1
    movl    %sp@(12),%d2
    movl    %sp@(16),%d3

#if ARCH_DO_RELOCATION
    lea     %pc@(_start),%a0 // load the current address using PC relative addressing mode
    movl    #_start,%a1      // load the same symbol absolutely
    cmpal   %a0,%a1
    beqs    bss_clear

    // load the end address for loop termination
    movl    #_end,%a2

    // copy forwards
    // NOTE: assumes the source and target do not overlap
0:
    movel   %a0@+,%a1@+
    cmpal   %a1,%a2
    bne     0b

    // branch to the new location
    movl    #bss_clear,%a0
    jmp     %a0@
#endif

    // clear bss
bss_clear:
    lea     __bss_start,%a0
    lea     __bss_end,%a1
    cmpl    %a0,%a1
    beqs    1f
    // zero 4 bytes at a time
0:
    clrl    %a0@+
    cmpal   %a1,%a0
    bne     0b
1:

    // load the initial stack pointer
    lea     _default_stack_top,%sp

    // branch into C land with 4 args off the previous stack
    movl    %d3,%sp@-
    movl    %d2,%sp@-
    movl    %d1,%sp@-
    movl    %d0,%sp@-
    jsr     lk_main

    // if we return from main just loop forever
    bra     .
END_FUNCTION(_start)

.bss
.align 4
_default_stack_base:
    .skip 4096
_default_stack_top:

