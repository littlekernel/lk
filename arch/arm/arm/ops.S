/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <arch/arm/cores.h>

.text

/* void _arch_enable_ints(void); */
FUNCTION(_arch_enable_ints)
	mrs	r0, cpsr
	bic	r0, r0, #(1<<7)		/* clear the I bit */
	msr	cpsr_c, r0
	bx	lr

/* void _arch_disable_ints(void); */
FUNCTION(_arch_disable_ints)
	mrs	r0, cpsr
	orr	r0, r0, #(1<<7)
	msr	cpsr_c, r0
	bx	lr

/* int _atomic_swap(int *ptr, int val); */
FUNCTION(_atomic_swap)
.L_loop_swap:
	ldrex	r12, [r0]
	strex	r2, r1, [r0]
	cmp		r2, #0
	bne 	.L_loop_swap

	/* save old value */
	mov	r0, r12
	bx	lr

/* int _atomic_add(int *ptr, int val); */
FUNCTION(_atomic_add)
	/* use load/store exclusive */
.L_loop_add:
	ldrex 	r12, [r0]
	add		r2, r12, r1
	strex 	r3, r2, [r0]
	cmp		r3, #0
	bne 	.L_loop_add
	
	/* save old value */
	mov		r0, r12
	bx		lr
	
/* int _atomic_and(int *ptr, int val); */
FUNCTION(_atomic_and)
	/* use load/store exclusive */
.L_loop_and:
	ldrex 	r12, [r0]
	and		r2, r12, r1
	strex 	r3, r2, [r0]
	cmp		r3, #0
	bne 	.L_loop_and
	
	/* save old value */
	mov		r0, r12
	bx		lr
	
/* int _atomic_or(int *ptr, int val); */
FUNCTION(_atomic_or)
	/* use load/store exclusive */
.L_loop_or:
	ldrex 	r12, [r0]
	orr		r2, r12, r1
	strex 	r3, r2, [r0]
	cmp		r3, #0
	bne 	.L_loop_or
	
	/* save old value */
	mov		r0, r12
	bx		lr

FUNCTION(arch_spin_trylock)
	mov	r2, r0
	mov	r1, #1
	ldrex	r0, [r2]
	cmp	r0, #0
	strexeq	r0, r1, [r2]
	dmb
	bx	lr

FUNCTION(arch_spin_lock)
	mov	r1, #1
1:
	ldrex	r2, [r0]
	cmp	r2, #0
	wfene
	strexeq	r2, r1, [r0]
	cmpeq	r2, #0
	bne	1b
	dmb
	bx	lr

FUNCTION(arch_spin_unlock)
	mov	r1, #0
	dmb
	str	r1, [r0]
	dsb
	sev
	bx	lr

/* void arch_idle(); */
FUNCTION(arch_idle)
#if ARM_ARCH_LEVEL >= 7
	wfi
#elif ARM_ARCH_LEVEL == 6
	mov	r0, #0
	mcr	p15, 0, r0, c7, c0, #4
#else
#error unknown cpu
#endif
	bx	lr

/* void arm_invalidate_tlb(void) */
FUNCTION(arm_invalidate_tlb)
	mov		r0, #0
	mcr 	p15, 0, r0, c8, c7, 0
	bx		lr

