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

struct avr32_iframe {
	uint32_t rsr;
	uint32_t rar;
	uint32_t r14;
	uint32_t usp;
	uint32_t r12;
	uint32_t r11;
	uint32_t r10;
	uint32_t r9;
	uint32_t r8;
	uint32_t r7;
	uint32_t r6;
	uint32_t r5;
	uint32_t r4;
	uint32_t r3;
	uint32_t r2;
	uint32_t r1;
	uint32_t r0;
};

int platform_irq(struct avr32_iframe *iframe);

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

/* system registers */
#define SR_GETPUT(name, regnum) \
	static inline uint32_t avr32_get_##name(void) \
	{ \
		uint32_t ret; \
		__asm__ volatile("mfsr	%0," #regnum : "=r"(ret)); \
		return ret; \
	} \
\
	static inline void avr32_set_##name(uint32_t val) \
	{ \
		__asm__ volatile("mtsr	" #regnum ", %0" :: "r"(val)); \
	}

SR_GETPUT(sr, 0)
SR_GETPUT(evba, 4)
SR_GETPUT(acba, 8)
SR_GETPUT(cpucr, 12)
SR_GETPUT(ecr, 16)
SR_GETPUT(rsr_sup, 20)
SR_GETPUT(rsr_int0, 24)
SR_GETPUT(rsr_int1, 28)
SR_GETPUT(rsr_int2, 32)
SR_GETPUT(rsr_int3, 36)
SR_GETPUT(rsr_ex, 40)
SR_GETPUT(rsr_nmi, 44)
SR_GETPUT(rsr_dbg, 48)
SR_GETPUT(rar_sup, 52)
SR_GETPUT(rar_int0, 56)
SR_GETPUT(rar_int1, 60)
SR_GETPUT(rar_int2, 64)
SR_GETPUT(rar_int3, 68)
SR_GETPUT(rar_ex, 72)
SR_GETPUT(rar_nmi, 76)
SR_GETPUT(rar_dbg, 80)
SR_GETPUT(config0, 256)
SR_GETPUT(config1, 260)
SR_GETPUT(count, 264)
SR_GETPUT(compare, 268)
SR_GETPUT(tlbehi, 272)
SR_GETPUT(tlbelo, 276)
SR_GETPUT(ptbr, 280)
SR_GETPUT(tlbear, 284)
SR_GETPUT(mmucr, 288)
SR_GETPUT(tlbarlo, 292)
SR_GETPUT(tlbarhi, 296)
SR_GETPUT(pccnt, 300)
SR_GETPUT(pcnt0, 304)
SR_GETPUT(pcnt1, 308)
SR_GETPUT(pccr, 312)
SR_GETPUT(bear, 316)
SR_GETPUT(sabal, 768)
SR_GETPUT(sabah, 772)
SR_GETPUT(sabd, 776)

// helpers
extern void avr32_interrupt_base;
extern void avr32_exception_base;

static inline uint32_t avr32_get_interrupt_autovector_offset(void)
{
	return (uint32_t)&avr32_interrupt_base - (uint32_t)&avr32_exception_base;
}

#if defined(__cplusplus)
}
#endif

#endif
