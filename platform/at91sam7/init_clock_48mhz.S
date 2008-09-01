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
.globl init_48mhz_clock

#define PMC_MCKR   0x30
#define PMC_SR     0x68

#define PMC_MCKRDY     0x08
#define PMC_PRES_DIV2  0x04
#define PMC_CSS_PLL    0x03

/* BUG?
**
** If I try to exit by bx lr, lr is corrupted somewhere in here.
** No clue why.  FIQ USB wedge not playing nice?  Am I cheating
** with my CPSR calls?
*/
init_48mhz_clock:
	ldr r1, =0xfffffc00
	mov r2, lr
		
	// turn on /2 prescaler
	mov r0, #PMC_PRES_DIV2
	str r0, [r1, #PMC_MCKR]
wait_for_clock1:
	ldr r0, [r1, #PMC_SR]
	tst r0, #PMC_MCKRDY
	beq wait_for_clock1

	// switch to pll clock
	mov r0, #(PMC_PRES_DIV2 | PMC_CSS_PLL)
	str r0, [r1, #PMC_MCKR]
wait_for_clock2:
	ldr r0, [r1, #PMC_SR]
	tst r0, #PMC_MCKRDY
	beq wait_for_clock2

	bx r2
