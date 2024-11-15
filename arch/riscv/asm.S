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
#include <arch/riscv/iframe.h>

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
END_FUNCTION(riscv_context_switch)

.macro save_regs, user
    addi   sp, sp, -RISCV_IFRAME_LEN // subtract a multiple of 16 to align the stack in 32bit
.if \user == 1
    // recover tp from the top of the stack (we saved it here before)
    STR     tp, RISCV_IFRAME_TP(sp)
    LDR     tp, (RISCV_IFRAME_LEN-__riscv_xlen / 8)(sp) // this is where the top of the stack used to be

    STR     gp, RISCV_IFRAME_GP(sp)

    // save the user stack and zero scratch register
    csrrw   gp, RISCV_CSR_XSCRATCH, zero
    STR     gp, RISCV_IFRAME_SP(sp)

    // recover gp for the kernel
.option push
.option norelax
    lla     gp, __global_pointer$
.option pop
.endif
    STR    t6, RISCV_IFRAME_T(6)(sp)
    STR    t5, RISCV_IFRAME_T(5)(sp)
    STR    t4, RISCV_IFRAME_T(4)(sp)
    STR    t3, RISCV_IFRAME_T(3)(sp)
    STR    t2, RISCV_IFRAME_T(2)(sp)
    STR    t1, RISCV_IFRAME_T(1)(sp)
    STR    t0, RISCV_IFRAME_T(0)(sp)
    STR    a7, RISCV_IFRAME_A(7)(sp)
    STR    a6, RISCV_IFRAME_A(6)(sp)
    STR    a5, RISCV_IFRAME_A(5)(sp)
    STR    a4, RISCV_IFRAME_A(4)(sp)
    STR    a3, RISCV_IFRAME_A(3)(sp)
    STR    a2, RISCV_IFRAME_A(2)(sp)
    STR    a1, RISCV_IFRAME_A(1)(sp)
    STR    a0, RISCV_IFRAME_A(0)(sp)
    STR    ra, RISCV_IFRAME_RA(sp)
    csrr   t0, RISCV_CSR_XSTATUS
    STR    t0, RISCV_IFRAME_STATUS(sp)
    csrr   a0, RISCV_CSR_XCAUSE
    csrr   a1, RISCV_CSR_XEPC
    STR    a1, RISCV_IFRAME_EPC(sp)
    mv     a2, sp
    // args are set up for a call into riscv_exception_handler()
    // a0 = xcause
    // a1 = xepc
    // a2 = sp
.endm

.macro restore_regs, user
    // put everything back
    LDR    t0, RISCV_IFRAME_EPC(sp)
    csrw   RISCV_CSR_XEPC, t0
    LDR    t0, RISCV_IFRAME_STATUS(sp)
    csrw   RISCV_CSR_XSTATUS, t0

    LDR    ra, RISCV_IFRAME_RA(sp)
    LDR    a0, RISCV_IFRAME_A(0)(sp)
    LDR    a1, RISCV_IFRAME_A(1)(sp)
    LDR    a2, RISCV_IFRAME_A(2)(sp)
    LDR    a3, RISCV_IFRAME_A(3)(sp)
    LDR    a4, RISCV_IFRAME_A(4)(sp)
    LDR    a5, RISCV_IFRAME_A(5)(sp)
    LDR    a6, RISCV_IFRAME_A(6)(sp)
    LDR    a7, RISCV_IFRAME_A(7)(sp)
    LDR    t0, RISCV_IFRAME_T(0)(sp)
    LDR    t1, RISCV_IFRAME_T(1)(sp)
    LDR    t2, RISCV_IFRAME_T(2)(sp)
    LDR    t3, RISCV_IFRAME_T(3)(sp)
    LDR    t4, RISCV_IFRAME_T(4)(sp)
    LDR    t5, RISCV_IFRAME_T(5)(sp)
    LDR    t6, RISCV_IFRAME_T(6)(sp)
.if \user == 1
    // before we run out of registers, save tp to the top of the kernel stack
    // and put the kernel stack in the scratch register
    addi   gp, sp, RISCV_IFRAME_LEN
    STR    tp, REGOFF(-1)(gp)
    csrw   RISCV_CSR_XSCRATCH, gp

    LDR    tp, RISCV_IFRAME_TP(sp)
    LDR    gp, RISCV_IFRAME_GP(sp)
    LDR    sp, RISCV_IFRAME_SP(sp)
.else
    addi   sp, sp, RISCV_IFRAME_LEN
.endif
.endm

// top level exception handler for riscv in non vectored mode
.balign 4
FUNCTION(riscv_exception_entry)
    // check to see if we came from user space
    csrrw   sp, RISCV_CSR_XSCRATCH, sp
    bnez    sp, 1f

    // put the stack back
    csrrw   sp, RISCV_CSR_XSCRATCH, sp
    j       kernel_exception_entry

1:
    // came from user space
    j       user_exception_entry
END_FUNCTION(riscv_exception_entry)

LOCAL_FUNCTION(kernel_exception_entry)
    // we came from kernel space so tp and gp are okay
    save_regs 0

    // bool kernel = true
    li     a3, 1
    call   riscv_exception_handler

    restore_regs 0

    RISCV_XRET
END_FUNCTION(kernel_exception_entry)

LOCAL_FUNCTION(user_exception_entry)
    // we came from user space, assume gp and tp have been trashed
    save_regs 1

    // bool kernel = false
    li     a3, 0
    call   riscv_exception_handler

    restore_regs 1

    RISCV_XRET
END_FUNCTION(user_exception_entry)

