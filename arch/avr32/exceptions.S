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

.text
.align 9
DATA(avr32_exception_base)
	rjmp	unhandled_exception 	/* unrecoverable exception */
	rjmp	unhandled_exception		/* tlb multiple hit */
	rjmp	unhandled_exception		/* bus error data fetch */
	rjmp	unhandled_exception		/* bus error instruction fetch */
	rjmp	unhandled_exception		/* nmi */
	rjmp	unhandled_exception		/* instrction address? */
	rjmp	unhandled_exception		/* itlb protection */
	rjmp	unhandled_exception		/* breakpoint */
	rjmp	unhandled_exception		/* illegal opcode */
	rjmp	unhandled_exception		/* unimplemented opcode */
	rjmp	unhandled_exception		/* priviledge violation */
	rjmp	unhandled_exception		/* floating-point */
	rjmp	unhandled_exception		/* coprocessor absent */
	rjmp	unhandled_exception		/* data address(read) */
	rjmp	unhandled_exception		/* data address(write) */
	rjmp	unhandled_exception		/* dtlb protection (read) */
	rjmp	unhandled_exception		/* dtlb protection (write) */
	rjmp	unhandled_exception		/* dtlb modified */

	// 0x50
	.skip	(0x50 - (. - avr32_exception_base))
	rjmp	unhandled_exception		/* itlb miss */

	// 0x60
	.skip	(0x60 - (. - avr32_exception_base))
	rjmp	unhandled_exception		/* dtlb miss(read) */

	// 0x70
	.skip	(0x70 - (. - avr32_exception_base))
	rjmp	unhandled_exception		/* dtlb miss(write) */

	// 0x100
	.skip	(0x100 - (. - avr32_exception_base))
	rcall	avr32_syscall
	rets

.align 2
FUNCTION(unhandled_exception)
	rjmp	avr32_unhandled

