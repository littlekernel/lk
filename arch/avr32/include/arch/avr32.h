/*
 * Copyright (c) 2009-2010 Travis Geiselbrecht
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
#ifndef __ARCH_AVR32_H
#define __ARCH_AVR32_H

#include <sys/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

void avr32_context_switch(vaddr_t *old_sp, vaddr_t new_sp);

// cache routines
#define ICACHE_FLUSH		0x0
#define ICACHE_INVALIDATE	0x1
#define ICACHE_LOCK			0x2
#define ICACHE_UNLOCK		0x3
#define ICACHE_PREFETCH		0x4

#define ICACHE_FLUSH_ALL	0x0
#define ICACHE_FLUSH_UNLOCKED	0x1
#define ICACHE_FLUSH_UNLOCK_ALL	0x2

#define DCACHE_FLUSH		0x8
#define DCACHE_LOCK			0x9
#define DCACHE_UNLOCK		0xa
#define DCACHE_INVALIDATE	0xb
#define DCACHE_CLEAN		0xc
#define DCACHE_CLEAN_INVALIDATE		0xd

#define DCACHE_FLUSH_INVALIDATE_ALL			0x0
#define DCACHE_FLUSH_INVALIDATE_UNLOCKED	0x1
#define DCACHE_FLUSH_CLEAN_ALL				0x2
#define DCACHE_FLUSH_CLEAN_UNLOCKED			0x3
#define DCACHE_FLUSH_CLEAN_INVALIDATE_ALL	0x4
#define DCACHE_FLUSH_CLEAN_INVALIDATE_UNLOCKED	0x5
#define DCACHE_FLUSH_UNLOCK_ALL				0x6

static inline void avr32_icache_invalidate(void)
{
	int zero = 0;
	__asm__ volatile("cache %0[0], 0" :: "r"(zero) : "memory");
}

static inline void avr32_icache_invalidate_line(void *addr)
{
	__asm__ volatile("cache %0[0], 1" :: "r"(addr) : "memory");
}

static inline void avr32_cache_clean(void)
{
	int zero = 0;
	__asm__ volatile("cache %0[2], 8" :: "r"(zero) : "memory");
}

static inline void avr32_cache_clean_invalidate(void)
{
	int zero = 0;
	__asm__ volatile("cache %0[4], 8" :: "r"(zero) : "memory");
}

static inline void avr32_dcache_invalidate_line(void *addr)
{
	__asm__ volatile("cache %0[0], 0xb" :: "r"(addr) : "memory");
}

static inline void avr32_dcache_clean_line(void *addr)
{
	__asm__ volatile("cache %0[0], 0xc" :: "r"(addr) : "memory");
}

static inline void avr32_dcache_clean_invalidate_line(void *addr)
{
	__asm__ volatile("cache %0[0], 0xd" :: "r"(addr) : "memory");
}

#if defined(__cplusplus)
}
#endif

#endif
