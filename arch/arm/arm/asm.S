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


	/* context switch frame is as follows:
	 * ulr
	 * usp
	 * lr
	 * r11
	 * r10
	 * r9
	 * r8
	 * r7
	 * r6
	 * r5
	 * r4
	 */
/* arm_context_switch(addr_t *old_sp, addr_t new_sp) */
FUNCTION(arm_context_switch)
	/* save all the usual registers + user regs */
	/* the spsr is saved and restored in the iframe by exceptions.S */
	sub		r3, sp, #(11*4)		/* can't use sp in user mode stm */
	mov		r12, lr
	stmia	r3, { r4-r11, r12, r13, r14 }^
	
	/* save old sp */
	str		r3, [r0] 

	/* clear any exlusive locks that the old thread holds */
#if ARM_ISA_ARMV7
	/* can clear it directly */
	.word	0xf57ff01f // clrex
#elif ARM_ISA_ARMV6
	/* have to do a fake strex to clear it */
	ldr		r0, =strex_spot
	strex	r3, r2, [r0]
#endif

	/* load new regs */
	ldmia	r1, { r4-r11, r12, r13, r14 }^
	mov		lr, r12				/* restore lr */
	add		sp, r1, #(11*4)     /* restore sp */
	bx		lr

.ltorg

FUNCTION(arm_save_mode_regs)
	mrs		r1, cpsr

#if ARM_ISA_ARMv6
	cps		#0x11			/* fiq */
	str		r13, [r0], #4
	str		r14, [r0], #4
	cps		#0x12			/* irq */
	str		r13, [r0], #4
	str		r14, [r0], #4
	cps		#0x13			/* svc */
	str		r13, [r0], #4
	str		r14, [r0], #4
	cps		#0x17			/* abt */
	str		r13, [r0], #4
	str		r14, [r0], #4
	cps		#0x1b			/* und */
	str		r13, [r0], #4
	str		r14, [r0], #4
	cps		#0x1f			/* sys */
	str		r13, [r0], #4
	str		r14, [r0], #4
#else
	// XXX implement
	b		.
#endif
	
	msr		cpsr_c, r1

	bx		lr

.data
strex_spot:
	.word	0
	

