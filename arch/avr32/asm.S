/*
 * Copyright (c) 2010 Travis Geiselbrecht
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

/* context switch frame:
	vaddr_t r0;
	vaddr_t r1;
	vaddr_t r2;
	vaddr_t r3;
	vaddr_t r4;
	vaddr_t r5;
	vaddr_t r6;
	vaddr_t r7;
	vaddr_t r8;
	vaddr_t r9;
	vaddr_t sr;
	vaddr_t lr;
*/

/* void avr32_context_switch(addr_t *old_sp, addr_t new_sp); */
FUNCTION(avr32_context_switch)
	/* save old state, including SR */
	mfsr	r10, 0
	pushm	r0-r10,lr

	/* save old sp */
	st.w	r12[0], sp

	/* load new one */
	mov		sp, r11

	/* restore state and exit */
	popm	r0-r10,lr
	mtsr	0, r10
	ret		r12

