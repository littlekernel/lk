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
    // use a REI to make sure the cpu dumps the pipeline
    moval   boot_stack_end,%sp  // set the stack temporarily in the current memory spot
    movpsl  -(%sp)
    movl    $relocated, -(%sp)
    rei

relocated:
    // switch to kernel stack
    moval   boot_stack_end,%sp  // set the interrupt stack in the relocated spot
    movpsl  -(%sp)              // push the psl on the stack
    bicl2   $(1<<26),(%sp)      // clear bit 26 (IS)
    moval   1f,-(%sp)           // push the address of the end of this routine
    rei                         // rei to it, loading the new PSL
1:

    // now we should be using the kernel stack pointer, so re-set it
    moval    boot_stack_end,%sp

    // branch into main and we're done
    calls   $0, lk_main
    halt

.section .data
// declare bootargs here to make sure it goes in the data segment, since it's
// saved before the bss is zeroed out.
.balign 4
.globl bootargs
bootargs:
    .skip 13*4
bootargs_end:

.section .bss
.balign 4
boot_stack:
    .skip 1024
boot_stack_end:
