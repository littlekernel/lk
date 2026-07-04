//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <lk/asm.h>

/* void sparc_context_switch(
    struct sparc_context_switch_frame *oldcs,
    struct sparc_context_switch_frame *newcs); */
FUNCTION(sparc_context_switch)
    // leaf function, operating directly in arch_context_switch's window.
    // o0 = oldcs
    // o1 = newcs
    // o6 = %sp (old thread stack pointer)
    // o7 = return address to arch_context_switch's caller

    // save the context we came from into the oldcs frame
    st      %o6, [%o0 + 0] // save the old stack
    st      %o7, [%o0 + 4] // save from pc

    // force the overflow handler to spill out 6 frames (NWINDOWS - 2)
    // this pushes the older windows to the old thread's stack
    save
    save
    save
    save
    save
    save
    restore
    restore
    restore
    restore
    restore
    restore

    // manually save arch_context_switch's l and i registers to its stack frame
    // doing this avoids a costly underflow trap when the old thread resumes later
    std     %l0, [%o6 + 0]
    std     %l2, [%o6 + 8]
    std     %l4, [%o6 + 16]
    std     %l6, [%o6 + 24]
    std     %i0, [%o6 + 32]
    std     %i2, [%o6 + 40]
    std     %i4, [%o6 + 48]
    std     %i6, [%o6 + 56]

    // switch to the new thread
    ld      [%o1 + 0], %o6 // restore new thread's stack pointer
    ld      [%o1 + 4], %o7 // restore new thread's pc

    // manually restore the new thread's arch_context_switch l and i registers
    ldd     [%o6 + 0], %l0
    ldd     [%o6 + 8], %l2
    ldd     [%o6 + 16], %l4
    ldd     [%o6 + 24], %l6
    ldd     [%o6 + 32], %i0
    ldd     [%o6 + 40], %i2
    ldd     [%o6 + 48], %i4
    ldd     [%o6 + 56], %i6

    // invalidate all other windows so that the new thread is forced
    // to reload its older frames from the stack on every restore.
    // We do this by setting the firewall to the window we are returning to (CWP + 1)
    rd      %psr, %g1
    and     %g1, 0x1f, %g1
    add     %g1, 1, %g1
    and     %g1, 7, %g1
    mov     1, %g2
    sll     %g2, %g1, %g2
    wr      %g2, 0, %wim
    nop; nop; nop

    // go back to the caller's frame
    retl
    nop
END_FUNCTION(sparc_context_switch)
