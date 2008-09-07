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

.text

/* void arch_enable_ints(void); */
FUNCTION(arch_enable_ints)
	mrs	r0, cpsr
	bic	r0, r0, #(1<<7)		/* clear the I bit */
	msr	cpsr_c, r0
	bx	lr

/* void arch_disable_ints(void); */
FUNCTION(arch_disable_ints)
	mrs	r0, cpsr
	orr	r0, r0, #(1<<7)
	msr	cpsr_c, r0
	bx	lr

/* int atomic_swap(int *ptr, int val); */
FUNCTION(atomic_swap)
	swp	r0, r2, [r1]
	bx	lr

/* int atomic_add(int *ptr, int val); */
FUNCTION(atomic_add)
#if ARM_ISA_ARMV6 || ARM_ISA_ARMV7
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
#else
	/* disable interrupts, do the add, and reenable */
	mrs	r2, cpsr
	mov	r12, r2
	orr	r2, r2, #(3<<6)
	msr	cpsr_c, r2

	/* ints disabled, old cpsr state in r12 */
	
	/* do the add, leave the previous value in r0 */
	mov	r3, r0
	ldr	r0, [r3]
	add	r2, r0, r1
	str	r2, [r3]

	/* restore interrupts and exit */
	msr	cpsr_c, r12
	bx	lr
#endif
	
/* int atomic_and(int *ptr, int val); */
FUNCTION(atomic_and)
#if ARM_ISA_ARMV6 || ARM_ISA_ARMV7
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
#else
	/* disable interrupts, do the and, and reenable */
	mrs	r2, cpsr
	mov	r12, r2
	orr	r2, r2, #(3<<6)
	msr	cpsr_c, r2

	/* ints disabled, old cpsr state in r12 */
	
	/* do the and, leave the previous value in r0 */
	mov	r3, r0
	ldr	r0, [r3]
	and	r2, r0, r1
	str	r2, [r3]

	/* restore interrupts and exit */
	msr	cpsr_c, r12
	bx	lr
#endif
	
/* int atomic_or(int *ptr, int val); */
FUNCTION(atomic_or)
#if ARM_ISA_ARMV6 || ARM_ISA_ARMV7
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
#else
	/* disable interrupts, do the or, and reenable */
	mrs	r2, cpsr
	mov	r12, r2
	orr	r2, r2, #(3<<6)
	msr	cpsr_c, r2

	/* ints disabled, old cpsr state in r12 */
	
	/* do the or, leave the previous value in r0 */
	mov	r3, r0
	ldr	r0, [r3]
	orr	r2, r0, r1
	str	r2, [r3]

	/* restore interrupts and exit */
	msr	cpsr_c, r12
	bx	lr
#endif

/* void arch_idle(); */
FUNCTION(arch_idle)
#if ARM_CPU_CORTEX_A8
	.word 0xe320f003 /* wfi */
#elif PLATFORM_MSM7K
	/* TODO: safely handle wfi */
#elif ARM_CPU_ARM1136 || ARM_CPU_ARM926
	mov	r0, #0
	mcr	p15, 0, r0, c7, c0, #4
#elif ARM_CPU_ARM7
	/* nothing to do here */
#else
#error unknown cpu
#endif
	bx	lr

/* uint32_t arm_read_cr1(void) */
FUNCTION(arm_read_cr1)
	mrc		p15, 0, r0, c1, c0, 0
	bx		lr

/* void arm_write_cr1(uint32_t val) */
FUNCTION(arm_write_cr1)
	mcr		p15, 0, r0, c1, c0, 0
	bx		lr

/* uint32_t arm_read_cr1_aux(void) */
FUNCTION(arm_read_cr1_aux)
	mrc		p15, 0, r0, c1, c0, 1
	bx		lr

/* void arm_write_cr1_aux(uint32_t val) */
FUNCTION(arm_write_cr1_aux)
	mcr		p15, 0, r0, c1, c0, 1
	bx		lr

/* void arm_write_ttbr(uint32_t val) */
FUNCTION(arm_write_ttbr)
	mcr 	p15, 0, r0, c2, c0, 0
	bx		lr

/* void arm_write_dacr(uint32_t val) */
FUNCTION(arm_write_dacr)
	mcr 	p15, 0, r0, c3, c0, 0
	bx		lr

/* void arm_invalidate_tlb(void) */
FUNCTION(arm_invalidate_tlb)
	mov		r0, #0
	mcr 	p15, 0, r0, c8, c7, 0
	bx		lr

/* void arch_switch_stacks_and_call(addr_t call, addr_t stack) */
FUNCTION(arch_switch_stacks_and_call)
	mov		sp, r1
	bx		r0	
