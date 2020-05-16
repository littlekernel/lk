/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/asm.h>
#include <arch/arm/cores.h>

.text

FUNCTION(arch_spin_trylock)
    mov     r2, r0
    mov     r1, #1
    ldrex   r0, [r2]
    cmp     r0, #0
    strexeq r0, r1, [r2]
    dmb
    bx      lr

FUNCTION(arch_spin_lock)
    mov     r1, #1
1:
    ldrex   r2, [r0]
    cmp     r2, #0
    wfene
    strexeq r2, r1, [r0]
    cmpeq   r2, #0
    bne     1b
    dmb
    bx      lr

FUNCTION(arch_spin_unlock)
    mov     r1, #0
    dmb
    str     r1, [r0]
    dsb
    sev
    bx      lr

/* void arch_idle(); */
FUNCTION(arch_idle)
#if ARM_ARCH_LEVEL >= 7
    wfi
#elif ARM_ARCH_LEVEL == 6
    mov     r0, #0
    mcr     p15, 0, r0, c7, c0, #4
#else
#error unknown cpu
#endif
    bx      lr

/* void arm_invalidate_tlb(void) */
FUNCTION(arm_invalidate_tlb)
    mov     r0, #0
    mcr     p15, 0, r0, c8, c7, 0
    bx      lr

