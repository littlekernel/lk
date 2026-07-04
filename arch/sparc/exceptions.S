//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <lk/asm.h>
#include <arch/sparc.h>

// Exception handlers for SPARC v8
// - One giant table, 4096 byte aligned, 4096 bytes long
// - Each vector is 0x10 bytes apart
// - At start of each exception:
//   - register window has been rotated once
//   - l1 has saved PC at the point at which it faulted
//   - l2 has saved next PC at the point at which it faulted
//   - PSR.ET = 0 (traps disabled)
//   - i and g registers are live
//   - we own l and o registers (from our new frame)

.macro handler
    rd  %psr, %l0
    b   sparc_shared_handler
    rd  %wim, %l3
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
    b   sparc_overflow
    rd  %wim, %l3
    nop
// 0x6 window underflow
    rd  %psr, %l0
    b   sparc_underflow
    rd  %wim, %l3
    nop
    handler // 0x7 memory address not aligned
    handler // 0x8 floating point exception
    handler // 0x9 data access exception
    handler // 0xa tag overflow
    handler // 0xb watchpoint detected
    handler // 0xc reserved
    handler // 0xd reserved
    handler // 0xe reserved
    handler // 0xf reserved
    handler // 0x10 reserved

// interrupts are 0x11 - 0x1f
.rept 15
    handler
.endr

.rept 0x80 - 0x20
    handler
.endr

    // 0x80 - software exceptions
.rept 0x80
    handler
.endr

END_FUNCTION(sparc_vectab)

FUNCTION(sparc_shared_handler)
    // at this point
    //  l0 - PSR
    //  l1 - PC
    //  l2 - nPC
    //  l3 - WIM
    // we have been rotated one window -1, which may or may not be the
    // 'firewall' window that the WIM would have marked as out of bounds.

    // create a stack frame for us.
    // the exception already rotated the window, but we didn't
    // set up the local o6 stack pointer, so we need to do that now.
    sub     %fp, 96 + 40, %sp

    // have we rotated into a window that should have triggered an overflow?
    and    %l0, SPARC_PSR_CWP_MASK, %l4
    set    1, %l5
    sll    %l5, %l4, %l4
    cmp    %l4, %l3
    bne    .Lclean_window
    nop

    // yes we are, we need to save the window below us and mark it as the new firewall
    save

    // deal with it
    // save the window on the stack in the slot of the frame that we just rotated to
    std     %l0, [%sp + 0]
    std     %l2, [%sp + 8]
    std     %l4, [%sp + 16]
    std     %l6, [%sp + 24]
    std     %i0, [%sp + 32]
    std     %i2, [%sp + 40]
    std     %i4, [%sp + 48]
    std     %i6, [%sp + 56]

    // compute the new wim that would mark the window we just saved as the new firewall
    rd      %psr, %l0
    and     %l0, SPARC_PSR_CWP_MASK, %l0
    mov     1, %l1
    sll     %l1, %l0, %l1
    wr      %l1, 0, %wim
    nop; nop; nop  // XXX do we need this?

    // rotate to the original trap window
    restore

    // now the firewall has been pushed one window down to the one we just saved.

.Lclean_window:
    // read the trap number into l7
    rd      %tbr, %l7
    srl     %l7, 4, %l7
    and     %l7, 0xff, %l7

    // save Y and FSR on the stack
    rd      %y, %l4
    # rd      %fsr, %l5
    std     %l4, [%sp + 96 + 0]

    // save all of the g registers on the stack
    std     %g0, [%sp + 96 + 8]
    std     %g2, [%sp + 96 + 16]
    std     %g4, [%sp + 96 + 24]
    std     %g6, [%sp + 96 + 32]

    // reenable traps, set PIL to 15 to mask all external exceptions
    or      %l0, (SPARC_PSR_ET | SPARC_PSR_PIL_MASK), %l4
    wr      %l4, %psr
    nop; nop; nop

    // pass some arguments
    mov     %l7, %o0            // exception number
    mov     %l1, %o1            // PC
    mov     %l2, %o2            // nPC
    call    sparc_exception
    mov     %l0, %o3            // PSR

    // pop the g registers off the stack
    ldd     [%sp + 96 + 8], %g0
    ldd     [%sp + 96 + 16], %g2
    ldd     [%sp + 96 + 24], %g4
    ldd     [%sp + 96 + 32], %g6

    // pop y and FSR off the stack
    ldd     [%sp + 96 + 0], %l4
    wr      %l4, %y
    # wr      %l5, %fsr

    // see if we'e about to window underflow in the rett instruction, which is catastrophic
    rd      %wim, %l3       // read a new wim, since we may have context switched and moved it
    rd      %psr, %l4       // read current PSR
    and     %l4, SPARC_PSR_CWP_MASK, %l4 // extract CWP
    add     %l4, 1, %l5     // CWP + 1
    and     %l5, 7, %l5  // PSR.CWD % 8 (assuming 8 windows)
    set     1, %l6
    sll     %l6, %l5, %l6   // (1 << (PSR.CWD + 1))
    cmp     %l6, %l3
    beq     .Lrestore_window_and_ret
    nop

.Lout_restore_psr:
    // restore PSR which should disable traps and restore the PIL to what it was before the exception
    // the saved PSR may have the wrong CWD if we had context switched
    rd      %psr, %l4
    and     %l4, SPARC_PSR_CWP_MASK, %l4
    andn    %l0, SPARC_PSR_CWP_MASK, %l0
    or      %l0, %l4, %l7
    wr      %l7, %psr
    nop; nop; nop

# 0: b 0b; nop

    // nope, regular return
    jmp     %l1
    rett    %l2

.Lrestore_window_and_ret:

# 0: b 0b; nop


    // start by zeroing out WIM so we can rotate the window to the one we need to restore
    wr      0, %wim
    nop; nop; nop

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

    save

    // compute what wim should be after we restore the window
    // l3 has the old wim
    sll     %l3, 1, %l4 // wim << 1
    srl     %l3, 7, %l5 // wim >> 7
    or      %l4, %l5, %l6 // combine to get next wim
    and     %l6, 0xff, %l6

    wr      %l6, %wim
    nop; nop; nop

    // branch to the common exit path to restore PSR
    b       .Lout_restore_psr
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
    // we have to set the wim have to set it inside this window,
    // so that we can rotate back without triggering  another overflow exception,i
    // which would be fatal when PSR.ET = 0
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

    // return from exception
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
    or      %l4, %l5, %l6 // combine to get next wim
    and     %l6, 0xff, %l6

    // write it into wim
    wr      %l6, %wim

    // we dont need to restore the PSR here because we haven't touched any condition bits
    // and we should be on the same window we were when we started.

    // return from exception
    jmp     %l1
    rett    %l2

0:  b 0b; nop
END_FUNCTION(sparc_underflow)

.bss
.align 8
.global dummy_window_dump
dummy_window_dump:
.skip 96

