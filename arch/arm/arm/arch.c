/*
 * Copyright (c) 2008-2015 Travis Geiselbrecht
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
#include <trace.h>
#include <stdio.h>
#include <reg.h>
#include <arch.h>
#include <arch/ops.h>
#include <arch/mmu.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>
#include <arch/mp.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lk/main.h>
#include <lk/init.h>
#include <platform.h>
#include <target.h>
#include <kernel/thread.h>

#define LOCAL_TRACE 0

#if WITH_DEV_TIMER_ARM_CORTEX_A9
#include <dev/timer/arm_cortex_a9.h>
#endif
#if WITH_DEV_INTERRUPT_ARM_GIC
#include <dev/interrupt/arm_gic.h>
#endif
#if WITH_DEV_CACHE_PL310
#include <dev/cache/pl310.h>
#endif

/* initial and abort stacks */
uint8_t abort_stack[ARCH_DEFAULT_STACK_SIZE * SMP_MAX_CPUS] __CPU_ALIGN;

static void arm_basic_setup(void);
static void spinlock_test(void);
static void spinlock_test_secondary(void);

#if WITH_SMP
/* smp boot lock */
spin_lock_t arm_boot_cpu_lock = 1;
volatile int secondaries_to_init = 0;
#endif

void arch_early_init(void)
{
	/* turn off the cache */
	arch_disable_cache(UCACHE);
#if WITH_DEV_CACHE_PL310
	pl310_set_enable(false);
#endif

	arm_basic_setup();

#if WITH_SMP && ARM_CPU_CORTEX_A9
	/* enable snoop control */
	addr_t scu_base = arm_read_cbar();
	*REG32(scu_base) |= (1<<0); /* enable SCU */
#endif

#if ARM_WITH_MMU
	arm_mmu_early_init();

	platform_init_mmu_mappings();
#endif

	/* turn the cache back on */
#if WITH_DEV_CACHE_PL310
	pl310_set_enable(true);
#endif
	arch_enable_cache(UCACHE);
}

void arch_init(void)
{
#if WITH_SMP
	arch_mp_init_percpu();

	LTRACEF("midr 0x%x\n", arm_read_midr());
	LTRACEF("sctlr 0x%x\n", arm_read_sctlr());
	LTRACEF("actlr 0x%x\n", arm_read_actlr());
#if ARM_CPU_CORTEX_A9
	LTRACEF("cbar 0x%x\n", arm_read_cbar());
#endif
	LTRACEF("mpidr 0x%x\n", arm_read_mpidr());
	LTRACEF("ttbcr 0x%x\n", arm_read_ttbcr());
	LTRACEF("ttbr0 0x%x\n", arm_read_ttbr0());
	LTRACEF("dacr 0x%x\n", arm_read_dacr());
#if ARM_CPU_CORTEX_A7
	LTRACEF("l2ctlr 0x%x\n", arm_read_l2ctlr());
	LTRACEF("l2ectlr 0x%x\n", arm_read_l2ectlr());
#endif

#if ARM_CPU_CORTEX_A9
	addr_t scu_base = arm_read_cbar();
	uint32_t scu_config = *REG32(scu_base + 4);
	secondaries_to_init = scu_config & 0x3;
#elif ARM_CPU_CORTEX_A7
	uint32_t l2ctlr = arm_read_l2ctlr();
	secondaries_to_init = (l2ctlr >> 24);
#else
	secondaries_to_init = SMP_MAX_CPUS - 1; /* TODO: get count from somewhere else, or add cpus as they boot */
#endif

	lk_init_secondary_cpus(secondaries_to_init);

	/* in platforms where the cpus have already been started, go ahead and wake up all the
	 * secondary cpus here.
	 */
	dprintf(SPEW, "releasing %d secondary cpu%c\n", secondaries_to_init, secondaries_to_init > 1 ? 's' : ' ');

	/* release the secondary cpus */
	spin_unlock(&arm_boot_cpu_lock);

	/* flush the release of the lock, since the secondary cpus are running without cache on */
	arch_clean_cache_range((addr_t)&arm_boot_cpu_lock, sizeof(arm_boot_cpu_lock));

#if ARM_ARCH_WAIT_FOR_SECONDARIES
	/* wait for secondary cpus to boot before arm_mmu_init below, which will remove
	 * temporary boot mappings
	 * TODO: find a cleaner way to do this than this #define
	 */
	while (secondaries_to_init > 0) {
		__asm__ volatile("wfe");
	}
#endif
#endif // WITH_SMP

	//spinlock_test();

	/* finish intializing the mmu */
	arm_mmu_init();
}

#if WITH_SMP
void arm_secondary_entry(uint asm_cpu_num)
{
	uint cpu = arch_curr_cpu_num();
	if (cpu != asm_cpu_num)
		return;

	arm_basic_setup();

	/* enable the local L1 cache */
	//arch_enable_cache(UCACHE);

	// XXX may not be safe, but just hard enable i and d cache here
	// at the moment cannot rely on arch_enable_cache not dumping the L2
	uint32_t sctlr = arm_read_sctlr();
	sctlr |= (1<<12) | (1<<2); // enable i and dcache
	arm_write_sctlr(sctlr);

	/* run early secondary cpu init routines up to the threading level */
	lk_init_level(LK_INIT_FLAG_SECONDARY_CPUS, LK_INIT_LEVEL_EARLIEST, LK_INIT_LEVEL_THREADING - 1);

	arch_mp_init_percpu();

	LTRACEF("cpu num %d\n", cpu);
	LTRACEF("sctlr 0x%x\n", arm_read_sctlr());
	LTRACEF("actlr 0x%x\n", arm_read_actlr());

	/* we're done, tell the main cpu we're up */
	atomic_add(&secondaries_to_init, -1);
	smp_mb();
	__asm__ volatile("sev");

	lk_secondary_cpu_entry();
}
#endif

static void arm_basic_setup(void)
{
	uint32_t sctlr = arm_read_sctlr();

	/* ARMV7 bits */
	sctlr &= ~(1<<10); /* swp disable */
	sctlr |=  (1<<11); /* enable program flow prediction */
	sctlr &= ~(1<<14); /* random cache/tlb replacement */
	sctlr &= ~(1<<25); /* E bit set to 0 on exception */
	sctlr &= ~(1<<30); /* no thumb exceptions */

	arm_write_sctlr(sctlr);

	uint32_t actlr = arm_read_actlr();
#if ARM_CPU_CORTEX_A9
	actlr |= (1<<2); /* enable dcache prefetch */
#if WITH_DEV_CACHE_PL310
	actlr |= (1<<7); /* L2 exclusive cache */
	actlr |= (1<<3); /* L2 write full line of zeroes */
	actlr |= (1<<1); /* L2 prefetch hint enable */
#endif
#if WITH_SMP
	/* enable smp mode, cache and tlb broadcast */
	actlr |= (1<<6) | (1<<0);
#endif
#endif // ARM_CPU_CORTEX_A9
#if ARM_CPU_CORTEX_A7
#if WITH_SMP
	/* enable smp mode */
	actlr |= (1<<6);
#endif
#endif // ARM_CPU_CORTEX_A7

	arm_write_actlr(actlr);

#if ENABLE_CYCLE_COUNTER && ARM_ISA_ARMV7
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

#if ARM_WITH_VFP
	/* enable cp10 and cp11 */
	uint32_t val = arm_read_cpacr();
	val |= (3<<22)|(3<<20);
	arm_write_cpacr(val);

	/* set enable bit in fpexc */
	__asm__ volatile("mrc  p10, 7, %0, c8, c0, 0" : "=r" (val));
	val |= (1<<30);
	__asm__ volatile("mcr  p10, 7, %0, c8, c0, 0" :: "r" (val));

	/* make sure the fpu starts off disabled */
	arm_fpu_set_enable(false);
#endif

	/* set the vector base to our exception vectors so we dont need to double map at 0 */
#if ARM_ISA_ARMV7
	arm_write_vbar(KERNEL_BASE + KERNEL_LOAD_OFFSET);
#endif
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

	uint32_t actlr = arm_read_actlr();
#if ARM_CPU_CORTEX_A9
	actlr = 0; /* put the aux control register back to default */
#endif // ARM_CPU_CORTEX_A9
	arm_write_actlr(actlr);
}

#if ARM_ISA_ARMV7
/* virtual to physical translation */
status_t arm_vtop(addr_t va, addr_t *pa)
{
	spin_lock_saved_state_t irqstate;

	arch_interrupt_save(&irqstate, SPIN_LOCK_FLAG_INTERRUPTS);

	arm_write_ats1cpr(va & ~(PAGE_SIZE-1));
	uint32_t par = arm_read_par();

	arch_interrupt_restore(irqstate, SPIN_LOCK_FLAG_INTERRUPTS);

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
	arch_disable_ints();

	/* give target and platform a chance to put hardware into a suitable
	 * state for chain loading.
	 */
	target_quiesce();
	platform_quiesce();

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
#if WITH_DEV_CACHE_PL310
	pl310_set_enable(false);
#endif

	/* put the booting cpu back into close to a default state */
	arch_quiesce();

	LTRACEF("branching to physical address of loader\n");

	/* branch to the physical address version of the chain loader routine */
	void (*loader)(paddr_t entry, ulong, ulong, ulong, ulong) __NO_RETURN = (void *)loader_pa;
	loader(entry_pa, arg0, arg1, arg2, arg3);
#else
#error handle the non vm path (should be simpler)
#endif
}

static spin_lock_t lock = 0;

static void spinlock_test(void)
{
	TRACE_ENTRY;

	spin_lock_saved_state_t state;
	spin_lock_irqsave(&lock, state);

	TRACEF("cpu0: i have the lock\n");
	spin(1000000);
	TRACEF("cpu0: releasing it\n");

	spin_unlock_irqrestore(&lock, state);

	spin(1000000);
}

static void spinlock_test_secondary(void)
{
	TRACE_ENTRY;

	spin(500000);
	spin_lock_saved_state_t state;
	spin_lock_irqsave(&lock, state);

	TRACEF("cpu1: i have the lock\n");
	spin(250000);
	TRACEF("cpu1: releasing it\n");

	spin_unlock_irqrestore(&lock, state);
}

/* vim: set ts=4 sw=4 noexpandtab: */
