/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
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
#include <sys/types.h>
#include <compiler.h>
#include <arch.h>
#include <arch/arm.h>
#include <arch/arm/mmu.h>

#if ARM_WITH_MMU

#define MB (1024*1024)

/* the location of the table may be brought in from outside */
#if WITH_EXTERNAL_TRANSLATION_TABLE
#if !defined(MMU_TRANSLATION_TABLE_ADDR)
#error must set MMU_TRANSLATION_TABLE_ADDR in the make configuration
#endif
uint32_t *tt = (void *)MMU_TRANSLATION_TABLE_ADDR;
#else
/* the main translation table */
uint32_t tt[4096] __ALIGNED(16384) __SECTION(".bss.prebss.translation_table");
#endif

#define MMU_FLAG_CACHED 0x1
#define MMU_FLAG_BUFFERED 0x2
#define MMU_FLAG_READWRITE 0x4

void arm_mmu_map_section(addr_t paddr, addr_t vaddr, uint flags)
{
	int index;

	/* Get the index into the translation table */
	index = vaddr / MB;

	/* Set the entry value:
	 * (2<<0): Section entry
	 * (0<<5): Domain = 0
	 *  flags: TEX, CB and AP bit settings provided by the caller.
	 */
	tt[index] = (paddr & ~(MB-1)) | (MMU_MEMORY_DOMAIN_MEM << 5) | (2<<0) | flags;

	arm_invalidate_tlb();
}

void arm_mmu_unmap_section(addr_t vaddr)
{
	uint index = vaddr / MB;
	tt[index] = 0;

	arm_invalidate_tlb();
}

#if defined(ARM_ISA_ARMV6) | defined(ARM_ISA_ARMV7)
#define MMU_INIT_MAP_FLAGS	    (MMU_MEMORY_L1_TYPE_STRONGLY_ORDERED | \
				    MMU_MEMORY_L1_AP_P_RW_U_NA)
#else
#define MMU_INIT_MAP_FLAGS	    MMU_FLAG_READWRITE
#endif

void arm_mmu_init(void)
{
#if !WITH_MMU_RELOC
	/* set some mmu specific control bits */
	arm_write_sctlr(arm_read_sctlr() & ~((1<<29)|(1<<28)|(1<<0))); // access flag disabled, TEX remap disabled, mmu disabled

	/* set up an identity-mapped translation table with
	 * strongly ordered memory type and read/write access.
	 */
	for (addr_t i=0; i < 4096; i++) {
		arm_mmu_map_section(i * MB, i * MB, MMU_INIT_MAP_FLAGS);
	}

	/* set up the translation table base */
	arm_write_ttbr0((uint32_t)tt);

	/* set up the domain access register */
	arm_write_dacr(0x1 << (MMU_MEMORY_DOMAIN_MEM * 2));

	/* turn on the mmu */
	arm_write_sctlr(arm_read_sctlr() | 0x1);
#endif
}

void arch_disable_mmu(void)
{
	arm_write_sctlr(arm_read_sctlr() & ~(1<<0)); // access flag disabled, TEX remap disabled, mmu disabled
}

#endif // ARM_WITH_MMU

