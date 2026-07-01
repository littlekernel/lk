//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <lk/asm.h>
#include <arch/sparc.h>

// Exception handlers for SPARC 32
// - One giant table, 4096 byte aligned, 4096 bytes long
// - Each vector is 0x10 bytes apart
// - At start of each exception:
//   - register window has been rotated once
//   - l1 has saved PC at the point at which it faulted
//   - l2 has saved next PC at the point at which it faulted
//   - PSR.ET = 0 (traps disabled)

.macro handler
    rd %psr, %l0
    b   sparc_shared_handler
    nop
    nop
.endm

.text
.balign 4096
FUNCTION(sparc_vectab)
// first vector, entry point
    b   _start
    nop
    nop
    nop
    handler // 0x1 instruction access exception
    handler // 0x2 illegal instruction
    handler // 0x3 privileged instruction
    handler // 0x4 floating point disabled
// 0x5 window overflow
    rd  %psr, %l0
    rd  %wim, %l3
    b   sparc_overflow
    nop
// 0x6 window underflow
    rd  %psr, %l0
    rd  %wim, %l3
    b   sparc_underflow
    nop
    handler // 0x7 memory address not aligned
    handler // 0x8 floating point exception
    handler // 0x9 data access exception
    handler // 0xa tag overflow
    handler // 0xb division by zero ?
    handler // 0xc software trap ?
    handler // 0xd breakpoint ?
    handler // 0xe reserved
    handler // 0xf reserved
    handler // 0x10 reserved

// interrupts are 0x11 - 0x1f
.rept 15
    handler
.endr

// TODO fill with illegal instructions

.balign 4096

END_FUNCTION(sparc_vectab)

FUNCTION(sparc_shared_handler)
    // read the trap number into l7
    rd      %tbr, %l7
    srl     %l7, 4, %l7
    and     %l7, 0xff, %l7

    // create a stack frame for us
    sub     %fp, 96, %sp

    // reenable traps, set PIL to 15 to mask all external exceptions
    or      %l0, (SPARC_PSR_ET | SPARC_PSR_PIL_MASK), %l4
    wr      %l4, %psr
    nop; nop; nop

    // pass some arguments
    mov     %l7, %o0            // exception number
    mov     %l1, %o1            // PC
    mov     %l2, %o2            // nPC
    mov     %l0, %o3            // PSR
    call    sparc_exception
    nop

0:  b 0b; nop


    nop
END_FUNCTION(sparc_shared_handler)

FUNCTION(sparc_overflow)
    // at entry to the overflow exception we have rotated to the window next to the one we need to save
    // entry registers:
    //  l0 - psr
    //  l1 - PC
    //  l2 - nPC
    //  l3 - wim

    // l registers are okay to use, since we're in a new window.
    // g6 is free to use for temporary storage, since the OS owns it.
    // i registers cannot be used, they are the o registers of the frame we just left.
    // o registers cannot be used, they are the i registers of the frame we're about to save (oldest frame).

    // compute what the next wim will be
    srl     %l3, 1, %l4 // wim >> 1
    sll     %l3, 7, %l5 // wim << 7
    or      %l4, %l5, %g6 // combine to get next wim
    and     %g6, 0xff, %g6 // mask to get valid wim bits

    // rotate the window so we can see the one we need to save
    save
    // we can set the new wim now
    wr      %g6, 0, %wim
    // save the window on the stack in the slot of the frame that we just rotated to
    std     %l0, [%sp + 0]
    std     %l2, [%sp + 8]
    std     %l4, [%sp + 16]
    std     %l6, [%sp + 24]
    std     %i0, [%sp + 32]
    std     %i2, [%sp + 40]
    std     %i4, [%sp + 48]
    std     %i6, [%sp + 56]
    // we can restore back to the previous window
    restore

    // zero out g6
    set     0, %g6

    // restore PSR
    wr      %l0, %0, %psr
    nop; nop; nop
    jmp     %l1
    rett    %l2

END_FUNCTION(sparc_overflow)

FUNCTION(sparc_underflow)
    // at entry to the underflow exception we have rotated to the window 2 below from the one we need to restore.
    // This is because the trap handler will have rotated the window once in the negative direction.
    // entry registers:
    //  l0 - psr
    //  l1 - PC
    //  l2 - nPC
    //  l3 - wim

    // l registers are okay to use, since we're in a new window.
    // g6 is free to use for temporary storage, since the OS owns it.
    // i registers can be used, since we're about to overwrite them with the restored window.
    // o registers cannot be used, they are the i registers of the frame we just came from.

    // start by zeroing out WIM so we can rotate the window to the one we need to restore
    wr      %g0, 0, %wim

    // rotate the window so we can see the one we need to restore
    restore
    restore

    // restore the window from the stack in the slot of the frame that we just rotated to
    ldd     [%sp + 0], %l0
    ldd     [%sp + 8], %l2
    ldd     [%sp + 16], %l4
    ldd     [%sp + 24], %l6
    ldd     [%sp + 32], %i0
    ldd     [%sp + 40], %i2
    ldd     [%sp + 48], %i4
    ldd     [%sp + 56], %i6

    // rotate back to the window the interrupt started in
    save
    save

    // compute what wim should be after we restore the window
    sll     %l3, 1, %l4 // wim << 1
    srl     %l3, 7, %l5 // wim >> 7
    or      %l4, %l5, %g4 // combine to get next wim
    and     %g4, 0xff, %g4

    // write it into wim
    wr      %g4, 0, %wim

    // restore PSR
    wr      %l0, %0, %psr
    nop; nop; nop
    jmp     %l1
    rett    %l2

0:  b 0b; nop
END_FUNCTION(sparc_underflow)
