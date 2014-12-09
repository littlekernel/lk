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
#include <err.h>
#include <debug.h>
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

#if WITH_KERNEL_VM
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

/* Address width */
uint32_t g_addr_width;

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

void platform_init_mmu_mappings(void)
{
#ifdef ARCH_X86_64
	uint64_t *new_pml4, phy_pml4;
	struct map_range range;
	uint64_t access = 0;

	/* getting the address width from CPUID instr */
	g_addr_width = x86_get_address_width();

	/* creating a new pml4 table */
	new_pml4 = memalign(PAGE_SIZE, PAGE_SIZE);
	ASSERT(new_pml4);
	memset(new_pml4, 0, PAGE_SIZE);
	phy_pml4 = (uint64_t)X86_VIRT_TO_PHYS(new_pml4);

	/* kernel code section mapping */
	access = X86_MMU_PG_P;
	range.start_vaddr = range.start_paddr = (addr_t) &__code_start;
	range.size = ((uint64_t)&__code_end) - ((uint64_t)&__code_start);
	x86_mmu_map_range(phy_pml4, &range, access);

	/* kernel data section mapping */
	access = X86_MMU_PG_NX | X86_MMU_PG_RW | X86_MMU_PG_P;
	range.start_vaddr = range.start_paddr = (addr_t) &__data_start;
	range.size = ((uint64_t)&__data_end) - ((uint64_t)&__data_start);
	x86_mmu_map_range(phy_pml4, &range, access);

	/* kernel rodata section mapping */
	access = X86_MMU_PG_NX | X86_MMU_PG_P;
	range.start_vaddr = range.start_paddr = (addr_t) &__rodata_start;
	range.size = ((uint64_t)&__rodata_end) - ((uint64_t)&__rodata_start);
	x86_mmu_map_range(phy_pml4, &range, access);

	/* kernel bss section and kernel heap mappings */
	access = X86_MMU_PG_NX | X86_MMU_PG_RW | X86_MMU_PG_P;
	range.start_vaddr = range.start_paddr = (addr_t) &__bss_start;
	range.size = ((uint64_t)_heap_end) - ((uint64_t)&__bss_start);
	x86_mmu_map_range(phy_pml4, &range, access);

	x86_set_cr3(phy_pml4);
#endif
}

#if WITH_KERNEL_VM
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
	#if WITH_KERNEL_VM
	heap_arena_init();
	pmm_add_arena(&heap_arena);
	#endif

}

void platform_init(void)
{
	uart_init();

	platform_init_keyboard();
#ifndef ARCH_X86_64
	pci_init();
#endif

	/* MMU init for x86_64 done after the heap is setup */
#ifdef ARCH_X86_64
        arch_mmu_init();
        platform_init_mmu_mappings();
#endif

}

