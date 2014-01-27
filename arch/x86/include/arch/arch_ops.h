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
#ifndef __ARCH_X86_OPS_H
#define __ARHC_X86_OPS_H

#include <compiler.h>

#ifndef ASSEMBLY

#include <arch/x86.h>

/* override of some routines */
static inline void arch_enable_ints(void)
{
	CF;
	__asm__ volatile("sti");
}

static inline inline void arch_disable_ints(void)
{
	__asm__ volatile("cli");
	CF;
}

static inline inline bool arch_ints_disabled(void)
{
	unsigned int state;

	__asm__ volatile(
	   "pushfl;"
	   "popl %%eax"
	   : "=a" (state)
	   :: "memory");

	return !!(state & (1<<9));
}

int _atomic_and(volatile int *ptr, int val);
int _atomic_or(volatile int *ptr, int val);
int _atomic_cmpxchg(volatile int *ptr, int oldval, int newval);

static inline int atomic_add(volatile int *ptr, int val)
{
	__asm__ volatile(
		"lock xaddl %[val], %[ptr];"
		: [val]"=a" (val)
		: "a" (val), [ptr]"m" (*ptr)
		: "memory"
	);

	return val;
}

static inline int atomic_swap(volatile int *ptr, int val)
{
	__asm__ volatile(
		"xchgl %[val], %[ptr];"
		: [val]"=a" (val)
		: "a" (val), [ptr]"m" (*ptr)
		: "memory"
	);

	return val;
}


static inline int atomic_and(volatile int *ptr, int val) { return _atomic_and(ptr, val); }
static inline int atomic_or(volatile int *ptr, int val) { return _atomic_or(ptr, val); }
static inline int atomic_cmpxchg(volatile int *ptr, int oldval, int newval) { return _atomic_cmpxchg(ptr, oldval, newval); }

static inline uint32_t arch_cycle_count(void)
{
	uint32_t timestamp;
	rdtscl(timestamp);

	return timestamp;
}

#endif // !ASSEMBLY

#endif

