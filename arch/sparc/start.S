//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <lk/asm.h>
#include <arch/sparc.h>

.section ".text.boot"
FUNCTION(_start)
    // set the top of the stack
    // XXX do we need to -96 and zero this?
    set     default_stack_top, %sp

    // clear our frame pointer to establish an end frame
    set     0, %fp

    // set up WIM and the PSR
    wr     %g0, %wim
    rd     %psr, %g1
    andn   %g1, SPARC_PSR_CWP_MASK, %g1 // CWP = 0
    andn   %g1, SPARC_PSR_ET, %g1       // disable traps
    or     %g1, SPARC_PSR_PIL_MASK, %g1 // PIL = 15
    wr     %g1, %psr
    nop; nop; nop

    // wim marked for frame 1
    set    0x2, %g2
    wr     %g2, %wim

    // enable traps
    or     %g1, SPARC_PSR_ET, %g1
    wr     %g1, %psr
    nop; nop; nop

    // clear BSS
    set     __bss_start, %g1
    set     __bss_end, %g2
.Lclear_bss:
    cmp     %g1, %g2
    bge,a   .Lcall_main
    nop
    st      %g0, [%g1]
    add     %g1, 4, %g1
    ba      .Lclear_bss
    nop

.Lcall_main:
    call    lk_main
    nop

    // Loop forever if we return
.Lloop:
    ba      .Lloop
    nop
END_FUNCTION(_start)

.bss
.balign 8
LOCAL_DATA(default_stack)
    .skip 8192
LOCAL_DATA(default_stack_top)
