/*
 * Copyright (c) 2008 Brian Swetland
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

.global dcc_putc
.global dcc_getc

#if defined(ARM_ISA_ARMV6) || defined(ARM_ISA_ARMV7)
dcc_getc:
	mrc 14, 0, r0, c0, c1, 0 
	tst r0, #(1 << 30)
	moveq r0, #-1
	mrcne 14, 0, r0, c0, c5, 0
	bx lr

dcc_putc:
	mrc 14, 0, r15, c0, c1, 0 
	mcrcc 14, 0, r0, c0, c5, 0
	movcc r0, #0
	movcs r0, #-1
	bx lr
#endif
