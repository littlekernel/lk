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

.globl platform_early_init
platform_early_init:
	stmdb sp!, {lr}
		
/* enable the NRST reset pin */
	ldr r1, =0xfffffd08
	ldr r0, =0xa5000401
	str r0, [r1]
		
/* disable watchdog */
	ldr r1, =0xfffffd44
	ldr r0, =0x3fff8fff
	str r0, [r1]

	bl init_clock
	bl init_48mhz_clock

/* copy the .data section from ROM to RAM */
	ldr     r0, =__rodata_end
	ldr     r1, =__data_start
	ldr     r2, =__bss_start
__data_loop:
	cmp     r1, r2
	ldrlt   r3, [r0], #4
	strlt   r3, [r1], #4
	blt     __data_loop

	bl mux_init		
	bl ser_init
	bl platform_init_interrupts
		
	ldmia sp!, {lr}
	bx lr
