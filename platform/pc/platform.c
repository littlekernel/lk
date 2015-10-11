/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
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

#include <err.h>
#include <arch/x86/mmu.h>
#include <platform.h>
#include "platform_p.h"
#include <platform/pc.h>
#include <platform/multiboot.h>
#include <platform/console.h>
#include <platform/keyboard.h>
#include <dev/pci.h>
#include <dev/uart.h>
#include <arch/x86.h>
#include <arch/mmu.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <kernel/vm.h>

extern multiboot_info_t *_multiboot_info;
extern int _end_of_ram;

#ifdef WITH_KERNEL_VM
extern int _end;
static uintptr_t _heap_start = (uintptr_t)&_end;
static uintptr_t _heap_end = (uintptr_t)&_end_of_ram;
#else
extern uintptr_t _heap_end;
#endif
extern uint64_t __code_start;
extern uint64_t __code_end;
extern uint64_t __rodata_start;
extern uint64_t __rodata_end;
extern uint64_t __data_start;
extern uint64_t __data_end;
extern uint64_t __bss_start;
extern uint64_t __bss_end;
extern void pci_init(void);
extern void arch_mmu_init(void);

/* Address width */
uint32_t g_addr_width;

/* Kernel global CR3 */
map_addr_t g_CR3 = 0;

#ifdef WITH_KERNEL_VM
struct mmu_initial_mapping mmu_initial_mappings[] = {
	/* 1 GB of memory*/
    { .phys = 0x200000,
      .virt = 0x200000,
      .size = 1024*1024*1024,
      .flags = 0,
      .name = "memory" },

	{ .phys = 0,
      .virt = 0,
      .size = 1024*1024*1024,
      .flags = MMU_INITIAL_MAPPING_TEMPORARY },
    /* null entry to terminate the list */
    { 0 }
};
#endif

void platform_init_mmu_mappings(void)
{
	struct map_range range;
	arch_flags_t access;
	map_addr_t *init_table, phy_init_table;

	/* getting the address width from CPUID instr */
	g_addr_width = x86_get_address_width();

	/* Creating the First page in the page table hirerachy */
	/* Can be pml4, pdpt or pdt based on x86_64, x86 PAE mode & x86 non-PAE mode respectively */
	init_table = memalign(PAGE_SIZE, PAGE_SIZE);
	ASSERT(init_table);
	memset(init_table, 0, PAGE_SIZE);

	phy_init_table = (map_addr_t)X86_VIRT_TO_PHYS(init_table);

	/* kernel code section mapping */
	access = ARCH_MMU_FLAG_PERM_RO;
	range.start_vaddr = range.start_paddr = (map_addr_t) &__code_start;
	range.size = ((map_addr_t)&__code_end) - ((map_addr_t)&__code_start);
	x86_mmu_map_range(phy_init_table, &range, access);

	/* kernel data section mapping */
	access = 0;
#if defined(ARCH_X86_64) || defined(PAE_MODE_ENABLED)
	access |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
#endif
	range.start_vaddr = range.start_paddr = (map_addr_t) &__data_start;
	range.size = ((map_addr_t)&__data_end) - ((map_addr_t)&__data_start);
	x86_mmu_map_range(phy_init_table, &range, access);

	/* kernel rodata section mapping */
	access = ARCH_MMU_FLAG_PERM_RO;
#if defined(ARCH_X86_64) || defined(PAE_MODE_ENABLED)
	access |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
#endif
	range.start_vaddr = range.start_paddr = (map_addr_t) &__rodata_start;
	range.size = ((map_addr_t)&__rodata_end) - ((map_addr_t)&__rodata_start);
	x86_mmu_map_range(phy_init_table, &range, access);

	/* kernel bss section and kernel heap mappings */
	access = 0;
#ifdef ARCH_X86_64
	access |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
#endif
	range.start_vaddr = range.start_paddr = (map_addr_t) &__bss_start;
	range.size = ((map_addr_t)_heap_end) - ((map_addr_t)&__bss_start);
	x86_mmu_map_range(phy_init_table, &range, access);

	/* Mapping for BIOS, devices */
	access = 0;
	range.start_vaddr = range.start_paddr = (map_addr_t) 0;
	range.size = ((map_addr_t)&__code_start);
	x86_mmu_map_range(phy_init_table, &range, access);

	/* Moving to the new CR3 */
	g_CR3 = (map_addr_t)phy_init_table;
	x86_set_cr3((map_addr_t)phy_init_table);
}

#ifdef WITH_KERNEL_VM
static pmm_arena_t heap_arena = {
    .name = "heap",
    .base = 0,
    .size = 0,
    .priority = 1,
    .flags = PMM_ARENA_FLAG_KMAP
};

void heap_arena_init()
{
	uintptr_t heap_base = ((uintptr_t)_heap_start);
	uintptr_t heap_size = (uintptr_t)_heap_end-(uintptr_t)_heap_start;

	heap_arena.base = PAGE_ALIGN(heap_base);
	heap_arena.size = PAGE_ALIGN(heap_size);
}
#endif

void platform_init_multiboot_info(void)
{
	unsigned int i;

	if (_multiboot_info) {
		if (_multiboot_info->flags & MB_INFO_MEM_SIZE) {
			_heap_end = _multiboot_info->mem_upper * 1024;
			memory_map_t *mmap = (memory_map_t *) (_multiboot_info->mmap_addr - 4);
		}

		if (_multiboot_info->flags & MB_INFO_MMAP) {
			memory_map_t *mmap = (memory_map_t *) (_multiboot_info->mmap_addr - 4);

			for (i=0; i < _multiboot_info->mmap_length / sizeof(memory_map_t); i++) {

				if (mmap[i].type == MB_MMAP_TYPE_AVAILABLE && mmap[i].base_addr_low >= _heap_end) {
					_heap_end = mmap[i].base_addr_low + mmap[i].length_low;
				} else if (mmap[i].type != MB_MMAP_TYPE_AVAILABLE && mmap[i].base_addr_low >= _heap_end) {
					/*
					 * break on first memory hole above default heap end for now.
					 * later we can add facilities for adding free chunks to the
					 * heap for each segregated memory region.
					 */
					break;
				}
			}
		}
	}
}

void platform_early_init(void)
{

	platform_init_uart();

	/* update the heap end so we can take advantage of more ram */
	platform_init_multiboot_info();

	/* get the text console working */
	platform_init_console();

	/* initialize the interrupt controller */
	platform_init_interrupts();

	/* initialize the timer */
	platform_init_timer();

#ifdef WITH_KERNEL_VM
	heap_arena_init();
	pmm_add_arena(&heap_arena);
#endif
}

void platform_init(void)
{
	uart_init();

	platform_init_keyboard();
#if defined(ARCH_X86)
	pci_init();
#endif

	/* MMU init for x86 Archs done after the heap is setup */
        arch_mmu_init();
	platform_init_mmu_mappings();
}
