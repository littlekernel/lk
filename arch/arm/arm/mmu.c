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
static uint32_t *tt = (void *)MMU_TRANSLATION_TABLE_ADDR;
#else
/* the main translation table */
static uint32_t tt[4096] __ALIGNED(16384) __SECTION(".bss.prebss.translation_table");
#endif

#define MMU_FLAG_CACHED 0x1
#define MMU_FLAG_BUFFERED 0x2
#define MMU_FLAG_READWRITE 0x4

void arm_mmu_map_section(addr_t paddr, addr_t vaddr, uint flags)
{
	int index;
	uint AP;
	uint CB = 0;
	uint TEX = 0;

#if defined(PLATFORM_MSM7K)
	if ((paddr >= 0x88000000) && (paddr < 0xD0000000)) {
		/* peripherals in the 0x88000000 - 0xD0000000 range must
		 * be mapped as DEVICE NON-SHARED: TEX=2, C=0, B=0
		 */
		TEX = 2;
		flags &= (~(MMU_FLAG_CACHED | MMU_FLAG_BUFFERED));
	}
#endif

	AP = (flags & MMU_FLAG_READWRITE) ? 0x3 : 0x2;
#if 1
	CB = ((flags & MMU_FLAG_CACHED) ? 0x2 : 0) | ((flags & MMU_FLAG_BUFFERED) ? 0x1 : 0);
#elif 0
	CB = ((flags & MMU_FLAG_CACHED) ? 0x2 : 0) | ((flags & MMU_FLAG_BUFFERED) ? 0x1 : 0);
	if (CB) {
		TEX = 1; // full write allocate on all levels
	}
#elif 0
	// try out some of the extended TEX options
	if (flags & MMU_FLAG_CACHED) {
		TEX = 6;
		CB = 3;
	}
#endif

	index = vaddr / MB;

	// section mapping
	tt[index] = (paddr & ~(MB-1)) | (TEX << 12) | (AP << 10) | (0<<5) | (CB << 2) | (2<<0);

	arm_invalidate_tlb();
}

void arm_mmu_unmap_section(addr_t vaddr)
{
	uint index = vaddr / MB;
	tt[index] = 0;

	arm_invalidate_tlb();
}

void arm_mmu_init(void)
{
	/* set some mmu specific control bits */
	arm_write_cr1(arm_read_cr1() & ~((1<<29)|(1<<28)|(1<<0))); // access flag disabled, TEX remap disabled, mmu disabled

	/* set up an identity-mapped translation table with cache disabled */
	for (addr_t i=0; i < 4096; i++) {
		arm_mmu_map_section(i * MB, i * MB,  MMU_FLAG_READWRITE); // map everything uncached
	}

	/* set up the translation table base */
	arm_write_ttbr((uint32_t)tt);

	/* set up the domain access register */
	arm_write_dacr(0x00000001);

	/* turn on the mmu */
	arm_write_cr1(arm_read_cr1() | 0x1);
}

void arch_disable_mmu(void)
{
	arm_write_cr1(arm_read_cr1() & ~(1<<0)); // access flag disabled, TEX remap disabled, mmu disabled
}

#endif // ARM_WITH_MMU

