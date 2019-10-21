/*
 * Copyright (c) 2019 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>
#include <arch/vax.h>

// a global label, but not typed as a function
#define GLABEL(x) .globl x; x:

.text

// first level exception vectors must be 4 byte aligned
.balign 4
GLABEL(vax_undefined_exception)
    halt

// for every possible exception handler, build a veneer routine that pushes
// the exception number to the stack and branches to a common routine
.balign 16
GLABEL(vax_exception_table)
.set i, 0
.rept (SCB_MAX_OFFSET / 4)

pushl   $i
jbr     interrupt_common
.balign 16

.set i, i + 1
.endr
END_DATA(vax_exception_table)

.align 4
GLABEL(interrupt_common)
    halt
END_FUNCTION(interrupt_common)

.align 4
GLABEL(_vax_mcheck)
    halt
END_FUNCTION(_vax_mcheck)

.align 4
GLABEL(_vax_invkstk)
    halt
END_FUNCTION(_vax_invkstk)

.align 4
GLABEL(_vax_interval_timer)
    pushr $0xfff

    calls $0,vax_interval_timer

    popr  $0xfff
    rei
END_FUNCTION(_vax_interval_timer)

// fill in a pre-computed SCB with a few vectors that we care about

#define SEEK(x) .org (x + SCB)
#define SCB_VECTOR(offset, funct, stack) SEEK(offset); .long funct + stack
.section .data
.balign 4
DATA(SCB)
    SCB_VECTOR(4, _vax_mcheck, SCB_FLAG_INT_STACK)
    SCB_VECTOR(8, _vax_invkstk, SCB_FLAG_INT_STACK)
    //SCB_VECTOR(40, _vax_syscall, SCB_FLAG_KERNEL_STACK)
    SCB_VECTOR(0xc0, _vax_interval_timer, SCB_FLAG_KERNEL_STACK)
    SEEK(SCB_MAX_OFFSET)
END_DATA(SCB)
