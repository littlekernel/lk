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
#define FUNCTION(name) .align 2 ; .globl _##name ; .type _##name,@function ; _##name

FUNCTION(arch_restore_ints):
	mov.l	inverse_imask_bit_mask,r0
	stc	sr,r1			/* get the sr register */
	and	r0,r1			/* zero out the imask part */
	or	r4,r1			/* or in the passed in imask, should only contain imask bits */
	ldc	r1,sr			/* put the new status into the sr register */
	rts
	nop

FUNCTION(arch_enable_ints):
    mov.l   inverse_imask_bit_mask,r0
    stc sr,r1           /* load the sr register */
    and r0,r1           /* set the imask to 0 */
    ldc r1,sr           /* put the new status into the sr register */
    rts
    nop

.align 2
inverse_imask_bit_mask: .long   0xffffff0f

FUNCTION(arch_disable_ints):
    mov.l   imask_bit_mask,r2
    stc sr,r1           /* load the sr register */
    mov r1,r0           /* save the old sr register */
    or  r2,r1           /* or in 0xf for the imask */
    ldc r1,sr           /* set the new sr register with the interrupts masked */
    rts
    and r2,r0           /* make sure the return value contains only the imask part */

.align 2
imask_bit_mask: .long   0x000000f0

FUNCTION(atomic_add):
	mov.l	r8,@-r15
	sts.l	pr,@-r15

	/* disable interrupts */
	mov.l	disable_interrupts_addr,r1
	jsr	@r1
	nop

	/* load the value, save it, add to it, and store it back */
	mov.l	@r4,r3
	mov	r3,r8
	add	r5,r3
	mov.l	r3,@r4

	/* restore interrupts */
	mov.l	restore_interrupts_addr,r1
	jsr	@r1
	mov	r0,r4

	/* return value will be old value */
	mov	r8,r0

	/* restore the stack */
	lds.l	@r15+,pr
	rts
	mov.l	@r15+,r8

FUNCTION(atomic_and):
	mov.l	r8,@-r15
	sts.l	pr,@-r15

	/* disable interrupts */
	mov.l	disable_interrupts_addr,r1
	jsr	@r1
	nop

	/* load the value, save it, and it, and store it back */
	mov.l	@r4,r3
	mov	r3,r8
	and	r5,r3
	mov.l	r3,@r4

	/* restore interrupts */
	mov.l	restore_interrupts_addr,r1
	jsr	@r1
	mov	r0,r4

	/* return value will be old value */
	mov	r8,r0

	/* restore the stack */
	lds.l	@r15+,pr
	rts
	mov.l	@r15+,r8

FUNCTION(atomic_or):
	mov.l	r8,@-r15
	sts.l	pr,@-r15

	/* disable interrupts */
	mov.l	disable_interrupts_addr,r1
	jsr	@r1
	nop

	/* load the value, save it, or it, and store it back */
	mov.l	@r4,r3
	mov	r3,r8
	or	r5,r3
	mov.l	r3,@r4

	/* restore interrupts */
	mov.l	restore_interrupts_addr,r1
	jsr	@r1
	mov	r0,r4

	/* return value will be old value */
	mov	r8,r0

	/* restore the stack */
	lds.l	@r15+,pr
	rts
	mov.l	@r15+,r8

FUNCTION(atomic_set):
	mov.l	r8,@-r15
	sts.l	pr,@-r15

	/* disable interrupts */
	mov.l	disable_interrupts_addr,r1
	jsr	@r1
	nop

	/* load the value, save it, and store the new value */
	mov.l	@r4,r8
	mov.l	r5,@r4

	/* restore interrupts */
	mov.l	restore_interrupts_addr,r1
	jsr	@r1
	mov	r0,r4

	/* return value will be old value */
	mov	r8,r0

	/* restore the stack */
	lds.l	@r15+,pr
	rts
	mov.l	@r15+,r8

/* int test_and_set(int *val, int set_to, int test_val) */
FUNCTION(test_and_set):
	mov.l	r8,@-r15
	sts.l	pr,@-r15

	/* disable interrupts */
	mov.l	disable_interrupts_addr,r1
	jsr	@r1
	nop

	/* load the value, save it, and store the new value */
	mov.l	@r4,r8         /* load the dest, it will be the return value */
	cmp/eq	r8,r6          /* compare against the test_val */
	bf	_not_equal

	mov.l	r5,@r4         /* put the set_to value into the target */

_not_equal:
	/* restore interrupts */
	mov.l	restore_interrupts_addr,r1
	jsr	@r1
	mov	r0,r4

	/* return value will be old value */
	mov	r8,r0

	/* restore the stack */
	lds.l	@r15+,pr
	rts
	mov.l	@r15+,r8

.align 2
disable_interrupts_addr:	.long	_arch_disable_ints
restore_interrupts_addr:	.long	_arch_restore_ints

// void sh4_context_switch(unsigned int **old_esp, unsigned int *new_esp);
FUNCTION(sh4_context_switch):
	fmov.s	fr12,@-r15
	fmov.s	fr13,@-r15
	fmov.s	fr14,@-r15
	fmov.s	fr15,@-r15
	sts.l	fpscr,@-r15
	sts.l	mach,@-r15
	sts.l	macl,@-r15
	mov.l	r8,@-r15
	mov.l	r9,@-r15
	mov.l	r10,@-r15
	mov.l	r11,@-r15
	mov.l	r12,@-r15
	mov.l	r13,@-r15
	mov.l	r14,@-r15
	sts.l	pr,@-r15

	mov.l	r15,@r4
	mov	r5,r15

	lds.l	@r15+,pr
	mov.l	@r15+,r14
	mov.l	@r15+,r13
	mov.l	@r15+,r12
	mov.l	@r15+,r11
	mov.l	@r15+,r10
	mov.l	@r15+,r9
	mov.l	@r15+,r8
	lds.l	@r15+,macl
	lds.l	@r15+,mach
	lds.l	@r15+,fpscr
	fmov.s	@r15+,fr15
	fmov.s	@r15+,fr14
	fmov.s	@r15+,fr13
	fmov.s	@r15+,fr12

	rts
	nop

FUNCTION(arch_idle):
	rts
	nop

