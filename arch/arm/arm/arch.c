/*
 * Copyright (c) 2008-2014 Travis Geiselbrecht
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
#include <trace.h>
#include <stdlib.h>
#include <err.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/mmu.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <platform.h>
#include <target.h>
#include <kernel/thread.h>

#define LOCAL_TRACE 0

void arch_early_init(void)
{
	/* turn off the cache */
	arch_disable_cache(UCACHE);

	/* set the vector base to our exception vectors so we dont need to double map at 0 */
#if ARM_ISA_ARMV7
	arm_write_vbar(KERNEL_BASE + KERNEL_LOAD_OFFSET);
#endif

#if ARM_WITH_MMU
	arm_mmu_init();

	platform_init_mmu_mappings();
#endif

	/* turn the cache back on */
	arch_enable_cache(UCACHE);

#if ARM_WITH_VFP
	/* enable cp10 and cp11 */
	uint32_t val = arm_read_cpacr();
	val |= (3<<22)|(3<<20);
	arm_write_cpacr(val);

	/* make sure the fpu starts off disabled */
	arm_fpu_set_enable(false);
#endif

#if ENABLE_CYCLE_COUNTER
#if ARM_ISA_ARMV7
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
#if ARM_ISA_ARMV7
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

#if ARM_ISA_ARMV7
/* virtual to physical translation */
status_t arm_vtop(addr_t va, addr_t *pa)
{
	arm_write_ats1cpr(va & 0xfffff000);
	uint32_t par = arm_read_par();

	if (par & 1)
		return ERR_NOT_FOUND;

	if (pa) {
		*pa = (par & 0xfffff000) | (va & 0xfff);
	}

	return NO_ERROR;
}
#endif

void arch_chain_load(void *entry, ulong arg0, ulong arg1, ulong arg2, ulong arg3)
{
	LTRACEF("entry %p, args 0x%lx 0x%lx 0x%lx 0x%lx\n", entry, arg0, arg1, arg2, arg3);

	/* we are going to shut down the system, start by disabling interrupts */
	enter_critical_section();

	/* give target and platform a chance to put hardware into a suitable
	 * state for chain loading.
	 */
	target_quiesce();
	platform_quiesce();

	arch_quiesce();

#if WITH_KERNEL_VM
	/* get the physical address of the entry point we're going to branch to */
	paddr_t entry_pa;
	if (arm_vtop((addr_t)entry, &entry_pa) < 0) {
		panic("error translating entry physical address\n");
	}

	/* add the low bits of the virtual address back */
	entry_pa |= ((addr_t)entry & 0xfff);

	LTRACEF("entry pa 0x%lx\n", entry_pa);

	/* figure out the mapping for the chain load routine */
	paddr_t loader_pa;
	if (arm_vtop((addr_t)&arm_chain_load, &loader_pa) < 0) {
		panic("error translating loader physical address\n");
	}

	/* add the low bits of the virtual address back */
	loader_pa |= ((addr_t)&arm_chain_load & 0xfff);

	paddr_t loader_pa_section = ROUNDDOWN(loader_pa, SECTION_SIZE);

	LTRACEF("loader address %p, phys 0x%lx, surrounding large page 0x%lx\n",
			&arm_chain_load, loader_pa, loader_pa_section);

	/* using large pages, map around the target location */
	arch_mmu_map(loader_pa_section, loader_pa_section, (2 * SECTION_SIZE / PAGE_SIZE), 0);

	LTRACEF("disabling instruction/data cache\n");
	arch_disable_cache(UCACHE);

	LTRACEF("branching to physical address of loader\n");

	/* branch to the physical address version of the chain loader routine */
	void (*loader)(paddr_t entry, ulong, ulong, ulong, ulong) __NO_RETURN = (void *)loader_pa;
	loader(entry_pa, arg0, arg1, arg2, arg3);
#else
#error handle the non vm path (should be simpler)
#endif
}

/* vim: set ts=4 sw=4 noexpandtab: */
