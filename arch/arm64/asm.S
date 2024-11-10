/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>
#include <arch/asm_macros.h>

/* void arm64_context_switch(vaddr_t *old_sp, vaddr_t new_sp); */
FUNCTION(arm64_context_switch)
    /* save old frame */
    push x28, x29
    push x26, x27
    push x24, x25
    push x22, x23
    push x20, x21
    push x18, x19
    mrs  x18, tpidr_el0
    mrs  x19, tpidrro_el0
    push x18, x19
    push x30, xzr

    /* save old sp */
    mov  x15, sp
    str  x15, [x0]

    /* load new sp */
    mov  sp, x1

    /* restore new frame */
    pop  x30, xzr
    pop  x18, x19
    msr  tpidr_el0, x18
    msr  tpidrro_el0, x19
    pop  x18, x19
    pop  x20, x21
    pop  x22, x23
    pop  x24, x25
    pop  x26, x27
    pop  x28, x29

    ret

    /* drop from whatever EL we may already be in to EL1.
     * carefully avoids using x0-x3 since this is called from start.S
     * which is trying to preserve them.
     */
FUNCTION(arm64_elX_to_el1)
    mrs x4, CurrentEL

    cmp x4, #(0b01 << 2)
    bne .notEL1
    /* Already in EL1 */
    ret

.notEL1:
    cmp x4, #(0b10 << 2)
    beq .inEL2

    /* set EL2 to 64bit */
    mrs x4, scr_el3
    orr x4, x4, #(1<<10)
    msr scr_el3, x4

    /* prep this mode's ELR and SPSR to drop into EL1 */
    adr x4, .Ltarget
    msr elr_el3, x4

    mov x4, #((0b1111 << 6) | (0b0101)) /* EL1h runlevel */
    msr spsr_el3, x4
    b   .confEL1

.inEL2:
    /* prep this mode's ELR and SPSR to drop into EL1 */
    adr x4, .Ltarget
    msr elr_el2, x4
    mov x4, #((0b1111 << 6) | (0b0101)) /* EL1h runlevel */
    msr spsr_el2, x4

.confEL1:
    /* disable EL2 coprocessor traps */
    mov x4, #0x33ff
    msr cptr_el2, x4

    /* set EL1 to 64bit and disable EL2 instruction traps */
    mov x4, #(1<<31)
    msr hcr_el2, x4

    /* set up the EL1 bounce interrupt */
    mov x4, sp
    msr sp_el1, x4

    /* make sure MPIDR_EL1 and MIDR_EL1 are set with the proper values */
    mrs x4, mpidr_el1
    msr vmpidr_el2, x4
    mrs x4, midr_el1
    msr vpidr_el2, x4

    isb
    eret

.Ltarget:
    ret
