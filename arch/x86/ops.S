/*
 * Copyright (c) 2009 Corey Tabaka
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

/* void arch_enable_ints(void); */
FUNCTION(arch_enable_ints)
	sti
	ret

/* void arch_disable_ints(void); */
FUNCTION(arch_disable_ints)
	cli
	ret

/* int atomic_swap(int *ptr, int val); */
FUNCTION(atomic_swap)
	movl 4(%esp), %edx
	movl 8(%esp), %eax
	xchgl %eax, (%edx)
	ret

/* int atomic_add(int *ptr, int val); */
FUNCTION(atomic_add)
	movl 4(%esp), %edx
	movl 8(%esp), %eax
	lock
	xadd %eax, (%edx)
	ret
	
/* int atomic_and(int *ptr, int val); */
FUNCTION(atomic_and)
	movl 4(%esp), %edx
	movl (%edx), %eax
0:
	movl %eax, %ecx
	andl 8(%esp), %ecx
	lock
	cmpxchgl %ecx, (%edx)
	jnz 1f					/* static prediction: branch forward not taken */
	ret
1:
	jmp 0b
	
	
/* int atomic_or(int *ptr, int val); */
FUNCTION(atomic_or)
movl 4(%esp), %edx
	movl (%edx), %eax
0:
	movl %eax, %ecx
	orl 8(%esp), %ecx
	lock
	cmpxchgl %ecx, (%edx)
	jnz 1f					/* static prediction: branch forward not taken */
	ret
1:
	jmp 0b

/* void arch_idle(); */
FUNCTION(arch_idle)
	pushf
	popl %eax
	andl $0x200, %eax
	test %eax, %eax
	je 1f					/* don't halt if local interrupts are disabled */
	hlt
1:
	ret

/* void arch_switch_stacks_and_call(addr_t call, addr_t stack) */
FUNCTION(arch_switch_stacks_and_call)
	movl 4(%esp), %eax
	movl 8(%esp), %edx
	movl %edx, %esp
	call *%eax			/* perhaps this should be a jmp? it's not used anywhere so I don't know. */
