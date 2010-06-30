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
.align 2

/* void bcopy(const void *src, void *dest, size_t n); */
FUNCTION(bcopy)
	// swap args for bcopy
	mov	r12, r0
	mov	r0, r1
	mov r1, r12

/* void *memcpy(void *dest, const void *src, size_t n); */
FUNCTION(memmove)
FUNCTION(memcpy)
	// check for zero length copy or the same pointer
	cmp		r2, #0
	cmpne	r1, r0
	bxeq	lr

	// save a few registers for use and the return code (input dst)
	stmfd	sp!, {r0, r4, r5, lr}

	// check for forwards overlap (src > dst, distance < len)
	subs	r3, r0, r1
	cmpgt	r2, r3
	bgt		.L_forwardoverlap

	// check for a short copy len.
	// 20 bytes is enough so that if a 16 byte alignment needs to happen there is at least a 
	//   wordwise copy worth of work to be done.
	cmp		r2, #(16+4)
	blt		.L_bytewise

	// see if they are similarly aligned on 4 byte boundaries
	eor		r3, r0, r1
	tst		r3, #3
	bne		.L_bytewise		// dissimilarly aligned, nothing we can do (for now)

	// check for 16 byte alignment on dst.
	// this will also catch src being not 4 byte aligned, since it is similarly 4 byte 
	//   aligned with dst at this point.
	tst		r0, #15
	bne		.L_not16bytealigned

	// check to see if we have at least 32 bytes of data to copy.
	// if not, just revert to wordwise copy
	cmp		r2, #32
	blt		.L_wordwise

.L_bigcopy:
	// copy 32 bytes at a time. src & dst need to be at least 4 byte aligned, 
	// and we need at least 32 bytes remaining to copy

	// save r6-r7 for use in the big copy
	stmfd	sp!, {r6-r7}

	sub		r2, r2, #32		// subtract an extra 32 to the len so we can avoid an extra compare

.L_bigcopy_loop:
	ldmia	r1!, {r4, r5, r6, r7}
	stmia	r0!, {r4, r5, r6, r7}
	ldmia	r1!, {r4, r5, r6, r7}
	subs	r2, r2, #32
	stmia	r0!, {r4, r5, r6, r7}
	bge		.L_bigcopy_loop

	// restore r6-r7
	ldmfd	sp!, {r6-r7}

	// see if we are done
	adds	r2, r2, #32
	beq		.L_done

	// less then 4 bytes left?
	cmp		r2, #4
	blt		.L_bytewise

.L_wordwise:
	// copy 4 bytes at a time.
	// src & dst are guaranteed to be word aligned, and at least 4 bytes are left to copy.
	subs	r2, r2, #4

.L_wordwise_loop:
	ldr		r3, [r1], #4
	subs	r2, r2, #4
	str		r3, [r0], #4
	bge		.L_wordwise_loop

	// correct the remaining len and test for completion
	adds	r2, r2, #4	
	beq		.L_done

.L_bytewise:
	// simple bytewise copy
	ldrb	r3, [r1], #1
	subs	r2, r2, #1
	strb	r3, [r0], #1
	bgt		.L_bytewise

.L_done:
	// load dst for return and restore r4,r5
#if ARM_ARCH_LEVEL >= 5
	ldmfd	sp!, {r0, r4, r5, pc}
#else
	ldmfd	sp!, {r0, r4, r5, lr}
	bx		lr
#endif

.L_not16bytealigned:
	// dst is not 16 byte aligned, so we will copy up to 15 bytes to get it aligned.
	// src is guaranteed to be similarly word aligned with dst.

	// set the condition flags based on the alignment.
	lsl		r12, r0, #28
	rsb		r12, r12, #0
	msr		CPSR_f, r12				// move into NZCV fields in CPSR

	// move as many bytes as necessary to get the dst aligned
	ldrvsb	r3, [r1], #1			// V set
	ldrcsh	r4, [r1], #2			// C set
	ldreq	r5, [r1], #4			// Z set

	strvsb	r3, [r0], #1
	strcsh	r4, [r0], #2
	streq	r5, [r0], #4

	ldmmiia	r1!, {r3-r4}			// N set
	stmmiia	r0!, {r3-r4}

	// fix the remaining len
	sub		r2, r2, r12, lsr #28

	// test to see what we should do now
	cmp		r2, #32
	bge		.L_bigcopy
	b		.L_wordwise
	
	// src and dest overlap 'forwards' or dst > src
.L_forwardoverlap:

	// do a bytewise reverse copy for now
	add		r1, r1, r2
	add		r0, r0, r2
	sub		r1, r1, #1
	sub		r0, r0, #1

.L_bytewisereverse:
	// simple bytewise reverse copy
	ldrb	r3, [r1], #-1
	subs	r2, r2, #1
	strb	r3, [r0], #-1
	bgt		.L_bytewisereverse

	b		.L_done

