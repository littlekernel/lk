/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>
#include <arch/riscv.h>
#include <arch/riscv/asm.h>

/* void riscv_context_switch(
    struct riscv_context_switch_frame *oldcs,
    struct riscv_context_switch_frame *newcs); */
FUNCTION(riscv_context_switch)
    # a0 = oldcs
    # a1 = newcs
    STR    ra, REGOFF(0)(a0)
    STR    sp, REGOFF(1)(a0)
    STR    s0, REGOFF(2)(a0)
    STR    s1, REGOFF(3)(a0)
    STR    s2, REGOFF(4)(a0)
    STR    s3, REGOFF(5)(a0)
    STR    s4, REGOFF(6)(a0)
    STR    s5, REGOFF(7)(a0)
    STR    s6, REGOFF(8)(a0)
    STR    s7, REGOFF(9)(a0)
    STR    s8, REGOFF(10)(a0)
    STR    s9, REGOFF(11)(a0)
    STR    s10, REGOFF(12)(a0)
    STR    s11, REGOFF(13)(a0)

    LDR    s11, REGOFF(13)(a1)
    LDR    s10, REGOFF(12)(a1)
    LDR    s9, REGOFF(11)(a1)
    LDR    s8, REGOFF(10)(a1)
    LDR    s7, REGOFF(9)(a1)
    LDR    s6, REGOFF(8)(a1)
    LDR    s5, REGOFF(7)(a1)
    LDR    s4, REGOFF(6)(a1)
    LDR    s3, REGOFF(5)(a1)
    LDR    s2, REGOFF(4)(a1)
    LDR    s1, REGOFF(3)(a1)
    LDR    s0, REGOFF(2)(a1)
    LDR    sp, REGOFF(1)(a1)
    LDR    ra, REGOFF(0)(a1)

    ret

/* top level exception handler for riscv in non vectored mode */
.balign 4
FUNCTION(riscv_exception_entry)
    /* dump all the callee trashed regs on the stack */
    addi   sp, sp, -REGOFF(20) // subtract a multiple of 16 to align the stack in 32bit
    STR    t6, REGOFF(17)(sp)
    STR    t5, REGOFF(16)(sp)
    STR    t4, REGOFF(15)(sp)
    STR    t3, REGOFF(14)(sp)
    STR    t2, REGOFF(13)(sp)
    STR    t1, REGOFF(12)(sp)
    STR    t0, REGOFF(11)(sp)
    STR    a7, REGOFF(10)(sp)
    STR    a6, REGOFF(9)(sp)
    STR    a5, REGOFF(8)(sp)
    STR    a4, REGOFF(7)(sp)
    STR    a3, REGOFF(6)(sp)
    STR    a2, REGOFF(5)(sp)
    STR    a1, REGOFF(4)(sp)
    STR    a0, REGOFF(3)(sp)
    STR    ra, REGOFF(2)(sp)
    csrr   t0, RISCV_CSR_XSTATUS
    STR    t0, REGOFF(1)(sp)
    csrr   a0, RISCV_CSR_XCAUSE
    csrr   a1, RISCV_CSR_XEPC
    STR    a1, REGOFF(0)(sp)
    mv     a2, sp

    jal    riscv_exception_handler

    /* put everything back */
    LDR    t0, REGOFF(0)(sp)
    csrw   RISCV_CSR_XEPC, t0
    LDR    t0, REGOFF(1)(sp)
    csrw   RISCV_CSR_XSTATUS, t0

    LDR    ra, REGOFF(2)(sp)
    LDR    a0, REGOFF(3)(sp)
    LDR    a1, REGOFF(4)(sp)
    LDR    a2, REGOFF(5)(sp)
    LDR    a3, REGOFF(6)(sp)
    LDR    a4, REGOFF(7)(sp)
    LDR    a5, REGOFF(8)(sp)
    LDR    a6, REGOFF(9)(sp)
    LDR    a7, REGOFF(10)(sp)
    LDR    t0, REGOFF(11)(sp)
    LDR    t1, REGOFF(12)(sp)
    LDR    t2, REGOFF(13)(sp)
    LDR    t3, REGOFF(14)(sp)
    LDR    t4, REGOFF(15)(sp)
    LDR    t5, REGOFF(16)(sp)
    LDR    t6, REGOFF(17)(sp)
    addi   sp, sp, REGOFF(20)

    RISCV_XRET
