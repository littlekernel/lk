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
#include <debug.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <platform.h>

#if ARM_CPU_CORTEX_A8
static void set_vector_base(addr_t addr)
{
	__asm__ volatile("mcr	p15, 0, %0, c12, c0, 0" :: "r" (addr));
}
#endif

void arch_early_init(void)
{
	/* turn off the cache */
	arch_disable_cache(UCACHE);

	/* set the vector base to our exception vectors so we dont need to double map at 0 */
#if ARM_CPU_CORTEX_A8
	if (MEMBASE != 0)
		set_vector_base(MEMBASE);
#endif

#if ARM_WITH_MMU
	arm_mmu_init();

	platform_init_mmu_mappings();
#endif

	/* turn the cache back on */
	arch_enable_cache(UCACHE);

#if ARM_WITH_NEON
	/* enable cp10 and cp11 */
	uint32_t val;
	__asm__ volatile("mrc	p15, 0, %0, c1, c0, 2" : "=r" (val));
	val |= (3<<22)|(3<<20);
	__asm__ volatile("mcr	p15, 0, %0, c1, c0, 2" :: "r" (val));

	/* set enable bit in fpexc */
	__asm__ volatile("mrc  p10, 7, %0, c8, c0, 0" : "=r" (val));
	val |= (1<<30);
	__asm__ volatile("mcr  p10, 7, %0, c8, c0, 0" :: "r" (val));
#endif

#if ENABLE_CYCLE_COUNTER
#if ARM_CPU_CORTEX_A8
	/* enable the cycle count register */
	uint32_t en;
	__asm__ volatile("mrc	p15, 0, %0, c9, c12, 0" : "=r" (en));
	en &= ~(1<<3); /* cycle count every cycle */
	en |= 1; /* enable all performance counters */
	__asm__ volatile("mcr	p15, 0, %0, c9, c12, 0" :: "r" (en));

	/* enable cycle counter */
	en = (1<<31);
	__asm__ volatile("mcr	p15, 0, %0, c9, c12, 1" :: "r" (en));
#endif
#endif
}

void arch_init(void)
{
}

void arch_quiesce(void)
{
#if ENABLE_CYCLE_COUNTER
#if ARM_CPU_CORTEX_A8
	/* disable the cycle count and performance counters */
	uint32_t en;
	__asm__ volatile("mrc	p15, 0, %0, c9, c12, 0" : "=r" (en));
	en &= ~1; /* disable all performance counters */
	__asm__ volatile("mcr	p15, 0, %0, c9, c12, 0" :: "r" (en));

	/* disable cycle counter */
	en = 0;
	__asm__ volatile("mcr	p15, 0, %0, c9, c12, 1" :: "r" (en));
#endif
#if ARM_CPU_ARM1136
	/* disable the cycle count and performance counters */
	uint32_t en;
	__asm__ volatile("mrc	p15, 0, %0, c15, c12, 0" : "=r" (en));
	en &= ~1; /* disable all performance counters */
	__asm__ volatile("mcr	p15, 0, %0, c15, c12, 0" :: "r" (en));
#endif
#endif
}

