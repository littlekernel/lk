/*
 * Copyright (c) 2008-2013 Travis Geiselbrecht
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

/* exception handling glue.
 * NOTE: only usable on armv6+ cores
 */

.macro save, offset
	/* save spsr and r14 onto the svc stack */
	sub		lr, \offset
	srsdb	#0x13!

	/* switch to svc mode, interrupts disabled */
	cpsid	i,#0x13

	/* save callee trashed regs and lr */
	push	{ r0-r3, r12, lr }

	/* save user space sp/lr */
	sub		sp, #8
	stmia	sp, { r13, r14 }^
.endm

.macro restore
	/* restore user space sp/lr */
	ldmia	sp, { r13, r14 }^
	add		sp, #8

	pop		{ r0-r3, r12, lr }

	/* return to whence we came from */
	rfeia	sp!
.endm

.macro saveall, mode
	/* save spsr and r14 onto our stack */
	srsdb	\mode!

	/* save all regs and lr to keep the stack aligned */
	push	{ r0-r12, r14 }

	/* save user space sp/lr */
	sub		sp, #8
	stmia	sp, { r13, r14 }^
.endm

.macro saveall_offset, offset, mode
	sub		lr, \offset
	saveall	\mode
.endm

.macro restoreall
	/* restore user space sp/lr */
	ldmia	sp, { r13, r14 }^
	add		sp, #8

	pop		{ r0-r12, r14 }

	/* return to whence we came from */
	rfeia	sp!
.endm

FUNCTION(arm_undefined)
	saveall #0x1b

	mov		r0, sp
	bl		arm_undefined_handler

	restoreall

FUNCTION(arm_syscall)
	saveall #0x13

	mov		r0, sp
	bl		arm_syscall_handler

	restoreall

FUNCTION(arm_prefetch_abort)
	saveall_offset #4, #0x17

	mov		r0, sp
	bl		arm_prefetch_abort_handler

	restoreall

FUNCTION(arm_data_abort)
	saveall_offset #8, #0x17

	mov		r0, sp
	bl		arm_data_abort_handler

	restoreall

FUNCTION(arm_reserved)
	b	.

FUNCTION(arm_irq)
	save    #4

	/* increment the global critical section count */
	ldr     r1, =critical_section_count
	ldr     r0, [r1]
	add     r0, r0, #1
	str     r0, [r1]

	/* call into higher level code */
	mov	r0, sp /* iframe */
	bl	platform_irq

	/* reschedule if the handler returns nonzero */
	cmp     r0, #0
	blne    thread_preempt

	/* decrement the global critical section count */
	ldr     r1, =critical_section_count
	ldr     r0, [r1]
	sub     r0, r0, #1
	str     r0, [r1]

	restore

FUNCTION(arm_fiq)
	saveall_offset #4, #0x17

	mov		r0, sp
	bl	platform_fiq

	restoreall

.ltorg

/* vim: set ts=4 sw=4 noexpandtab: */
