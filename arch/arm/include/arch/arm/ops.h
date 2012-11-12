/*
 * Copyright (c) 2008-2012 Travis Geiselbrecht
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
#ifndef __ARCH_ARM_OPS_H
#define __ARHC_ARM_OPS_H

#ifndef ASSEMBLY

#include <compiler.h>
#include <reg.h>

#if ARM_ISA_ARMV7 || (ARM_ISA_ARMV6 && !__thumb__)
#define USE_GCC_ATOMICS 0

// override of some routines
__GNU_INLINE __ALWAYS_INLINE extern inline void arch_enable_ints(void)
{
	CF;
	__asm__ volatile("cpsie i");
}

__GNU_INLINE __ALWAYS_INLINE extern inline void arch_disable_ints(void)
{
	__asm__ volatile("cpsid i");
	CF;
}

__GNU_INLINE __ALWAYS_INLINE extern inline int atomic_add(volatile int *ptr, int val)
{
#if USE_GCC_ATOMICS
	return __atomic_fetch_add(ptr, val, __ATOMIC_RELAXED);
#else
	int old;
	int temp;
	int test;

	do {
		__asm__ volatile(
		    "ldrex	%[old], [%[ptr]]\n"
		    "adds	%[temp], %[old], %[val]\n"
		    "strex	%[test], %[temp], [%[ptr]]\n"
		    : [old]"=&r" (old), [temp]"=&r" (temp), [test]"=&r" (test)
		    : [ptr]"r" (ptr), [val]"r" (val)
		    : "memory", "cc");

	} while (test != 0);

	return old;
#endif
}

__GNU_INLINE __ALWAYS_INLINE extern inline int atomic_or(volatile int *ptr, int val)
{
#if USE_GCC_ATOMICS
	return __atomic_fetch_or(ptr, val, __ATOMIC_RELAXED);
#else
	int old;
	int temp;
	int test;

	do {
		__asm__ volatile(
		    "ldrex	%[old], [%[ptr]]\n"
		    "orrs	%[temp], %[old], %[val]\n"
		    "strex	%[test], %[temp], [%[ptr]]\n"
		    : [old]"=&r" (old), [temp]"=&r" (temp), [test]"=&r" (test)
		    : [ptr]"r" (ptr), [val]"r" (val)
		    : "memory", "cc");

	} while (test != 0);

	return old;
#endif
}

__GNU_INLINE __ALWAYS_INLINE extern inline int atomic_and(volatile int *ptr, int val)
{
#if USE_GCC_ATOMICS
	return __atomic_fetch_and(ptr, val, __ATOMIC_RELAXED);
#else
	int old;
	int temp;
	int test;

	do {
		__asm__ volatile(
		    "ldrex	%[old], [%[ptr]]\n"
		    "ands	%[temp], %[old], %[val]\n"
		    "strex	%[test], %[temp], [%[ptr]]\n"
		    : [old]"=&r" (old), [temp]"=&r" (temp), [test]"=&r" (test)
		    : [ptr]"r" (ptr), [val]"r" (val)
		    : "memory", "cc");

	} while (test != 0);

	return old;
#endif
}

__GNU_INLINE __ALWAYS_INLINE extern inline int atomic_swap(volatile int *ptr, int val)
{
#if USE_GCC_ATOMICS
	return __atomic_exchange_n(ptr, val, __ATOMIC_RELAXED);
#else
	int old;
	int test;

	do {
		__asm__ volatile(
		    "ldrex	%[old], [%[ptr]]\n"
		    "strex	%[test], %[val], [%[ptr]]\n"
		    : [old]"=&r" (old), [test]"=&r" (test)
		    : [ptr]"r" (ptr), [val]"r" (val)
		    : "memory");

	} while (test != 0);

	return old;
#endif
}

__GNU_INLINE __ALWAYS_INLINE extern inline int atomic_cmpxchg(volatile int *ptr, int oldval, int newval)
{
	int old;
	int test;

	do {
		__asm__ volatile(
		    "ldrex	%[old], [%[ptr]]\n"
		    "mov	%[test], #0\n"
		    "teq	%[old], %[oldval]\n"
#if ARM_ISA_ARMV7M
		    "bne	0f\n"
		    "strex	%[test], %[newval], [%[ptr]]\n"
		    "0:\n"
#else
		    "strexeq %[test], %[newval], [%[ptr]]\n"
#endif
		    : [old]"=&r" (old), [test]"=&r" (test)
		    : [ptr]"r" (ptr), [oldval]"Ir" (oldval), [newval]"r" (newval)
		    : "cc");

	} while (test != 0);

	return old;
}

__GNU_INLINE __ALWAYS_INLINE extern inline uint32_t arch_cycle_count(void)
{
#if ARM_CPU_CORTEX_M3
#define DWT_CYCCNT (0xE0001004)
	return *REG32(DWT_CYCCNT);
#else
	return 0;
#endif
}


#endif

#endif

#endif

