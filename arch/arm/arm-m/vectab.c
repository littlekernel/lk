/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <compiler.h>
#include <stdint.h>

/*
 * Make a nice 8 byte aligned stack to run on before the threading system is up.
 * Put it in the .bss.prebss.* section to make sure it doesn't get wiped
 * when bss is cleared a little ways into boot.
 */
static uint8_t initial_stack[1024] __SECTION(".bss.prebss.initial_stack") __ALIGNED(8);

extern void _start(void);
extern void _nmi(void);
extern void _hardfault(void);
extern void _memmanage(void);
extern void _busfault(void);
extern void _usagefault(void);
extern void _svc(void);
extern void _pendsv(void);
extern void _pendsv(void);
extern void _systick(void);

const void * const __SECTION(".text.boot.vectab1") vectab[] = {
	/* arm exceptions */
	initial_stack + sizeof(initial_stack),
	_start,
	_nmi, // nmi
	_hardfault, // hard fault
	_memmanage, // mem manage
	_busfault, // bus fault
	_usagefault, // usage fault
	0, // reserved
	0, // reserved
	0, // reserved
	0, // reserved
	_svc, // svcall
	0, // debug monitor
	0, // reserved
	_pendsv, // pendsv
	_systick, // systick
};



