/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <asm.h>
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
    str  x30, [sp,#-8]!

    /* save old sp */
    mov  x15, sp
    str  x15, [x0]

    /* load new sp */
    mov  sp, x1

    /* restore new frame */
    ldr  x30, [sp], #8
    pop  x18, x19
    pop  x20, x21
    pop  x22, x23
    pop  x24, x25
    pop  x26, x27
    pop  x28, x29

    ret

FUNCTION(arm64_el3_to_el1)
    /* set EL2 to 64bit */
    mrs x0, scr_el3
    orr x0, x0, #(1<<10)
    msr scr_el3, x0

    /* set EL1 to 64bit */
    mov x0, #(1<<31)
    msr hcr_el2, x0

    /* disable EL2 coprocessor traps */
    mov x0, #0x33ff
    msr cptr_el2, x0

    /* disable EL1 FPU traps */
    mov x0, #(0b11<<20)
    msr cpacr_el1, x0

    /* set up the EL1 bounce interrupt */
    mov x0, sp
    msr sp_el1, x0

    adr x0, .Ltarget
    msr elr_el3, x0

    mov x0, #((0b1111 << 6) | (0b0101)) /* EL1h runlevel */
    msr spsr_el3, x0
    isb

    eret

.Ltarget:
    ret
