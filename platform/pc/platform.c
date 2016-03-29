/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2016 Travis Geiselbrecht
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
#include <trace.h>
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

#define LOCAL_TRACE 0

/* multiboot information passed in, if present */
extern multiboot_info_t *_multiboot_info;

#define DEFAULT_MEMEND (16*1024*1024)

static paddr_t mem_top = DEFAULT_MEMEND;
extern uint64_t __code_start;
extern uint64_t __code_end;
extern uint64_t __rodata_start;
extern uint64_t __rodata_end;
extern uint64_t __data_start;
extern uint64_t __data_end;
extern uint64_t __bss_start;
extern uint64_t __bss_end;

extern void pci_init(void);

void platform_init_mmu_mappings(void)
{
    // XXX move into arch/x86 setup
#if 0
    struct map_range range;
    arch_flags_t access;
    map_addr_t *init_table, phy_init_table;

    LTRACE_ENTRY;

    /* Creating the First page in the page table hirerachy */
    /* Can be pml4, pdpt or pdt based on x86_64, x86 PAE mode & x86 non-PAE mode respectively */
    init_table = memalign(PAGE_SIZE, PAGE_SIZE);
    ASSERT(init_table);
    memset(init_table, 0, PAGE_SIZE);

    phy_init_table = (map_addr_t)X86_VIRT_TO_PHYS(init_table);
    LTRACEF("phy_init_table: %p\n", phy_init_table);

    /* kernel code section mapping */
    LTRACEF("mapping kernel code\n");
    access = ARCH_MMU_FLAG_PERM_RO;
    range.start_vaddr = range.start_paddr = (map_addr_t) &__code_start;
    range.size = ((map_addr_t)&__code_end) - ((map_addr_t)&__code_start);
    x86_mmu_map_range(phy_init_table, &range, access);

    /* kernel data section mapping */
    LTRACEF("mapping kernel data\n");
    access = 0;
#if defined(ARCH_X86_64) || defined(PAE_MODE_ENABLED)
    access |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
#endif
    range.start_vaddr = range.start_paddr = (map_addr_t) &__data_start;
    range.size = ((map_addr_t)&__data_end) - ((map_addr_t)&__data_start);
    x86_mmu_map_range(phy_init_table, &range, access);

    /* kernel rodata section mapping */
    LTRACEF("mapping kernel rodata\n");
    access = ARCH_MMU_FLAG_PERM_RO;
#if defined(ARCH_X86_64) || defined(PAE_MODE_ENABLED)
    access |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
#endif
    range.start_vaddr = range.start_paddr = (map_addr_t) &__rodata_start;
    range.size = ((map_addr_t)&__rodata_end) - ((map_addr_t)&__rodata_start);
    x86_mmu_map_range(phy_init_table, &range, access);

    /* kernel bss section and kernel heap mappings */
    LTRACEF("mapping kernel bss+heap\n");
    access = 0;
#ifdef ARCH_X86_64
    access |= ARCH_MMU_FLAG_PERM_NO_EXECUTE;
#endif
    range.start_vaddr = range.start_paddr = (map_addr_t) &__bss_start;
    range.size = ((map_addr_t)_heap_end) - ((map_addr_t)&__bss_start);
    x86_mmu_map_range(phy_init_table, &range, access);

    /* Mapping for BIOS, devices */
    LTRACEF("mapping bios devices\n");
    access = 0;
    range.start_vaddr = range.start_paddr = (map_addr_t) 0;
    range.size = ((map_addr_t)&__code_start);
    x86_mmu_map_range(phy_init_table, &range, access);

    /* Moving to the new CR3 */
    g_CR3 = (map_addr_t)phy_init_table;
    x86_set_cr3((map_addr_t)phy_init_table);

    LTRACE_EXIT;
#endif
}

#if WITH_KERNEL_VM
struct mmu_initial_mapping mmu_initial_mappings[] = {
#if ARCH_X86_64
    /* 64GB of memory mapped where the kernel lives */
    {
        .phys = MEMBASE,
        .virt = KERNEL_ASPACE_BASE,
        .size = 64ULL*GB, /* x86-64 maps first 64GB by default */
        .flags = 0,
        .name = "memory"
    },
#endif
    /* 1GB of memory mapped where the kernel lives */
    {
        .phys = MEMBASE,
        .virt = KERNEL_BASE,
        .size = 1*GB, /* x86 maps first 1GB by default */
        .flags = 0,
        .name = "kernel"
    },

    /* null entry to terminate the list */
    { 0 }
};

static pmm_arena_t mem_arena = {
    .name = "memory",
    .base = MEMBASE,
    .size = DEFAULT_MEMEND, /* default amount of memory in case we don't have multiboot */
    .priority = 1,
    .flags = PMM_ARENA_FLAG_KMAP
};

/* set up the size of the physical memory map based on the end of memory we detected in
 * platform_init_multiboot_info()
 */
void mem_arena_init(void)
{
    uintptr_t mem_base = (uintptr_t)MEMBASE;
    uintptr_t mem_size = mem_top;

    mem_arena.base = PAGE_ALIGN(mem_base) + MB;
    mem_arena.size = PAGE_ALIGN(mem_size) - MB;
}
#endif

void platform_init_multiboot_info(void)
{
    LTRACEF("_multiboot_info %p\n", _multiboot_info);
    if (_multiboot_info) {
        /* bump the multiboot pointer up to the kernel mapping */
        _multiboot_info = (void *)((uintptr_t)_multiboot_info + KERNEL_BASE);

        if (_multiboot_info->flags & MB_INFO_MEM_SIZE) {
            LTRACEF("memory lower 0x%x\n", _multiboot_info->mem_lower * 1024U);
            LTRACEF("memory upper 0x%llx\n", _multiboot_info->mem_upper * 1024ULL);
            mem_top = _multiboot_info->mem_upper * 1024;
        }

        if (_multiboot_info->flags & MB_INFO_MMAP) {
            memory_map_t *mmap = (memory_map_t *)(uintptr_t)(_multiboot_info->mmap_addr - 4);
            mmap = (void *)((uintptr_t)mmap + KERNEL_BASE);

            LTRACEF("memory map:\n");
            for (uint i = 0; i < _multiboot_info->mmap_length / sizeof(memory_map_t); i++) {

                LTRACEF("\ttype %u addr 0x%x %x len 0x%x %x\n",
                    mmap[i].type, mmap[i].base_addr_high, mmap[i].base_addr_low,
                    mmap[i].length_high, mmap[i].length_low);
                if (mmap[i].type == MB_MMAP_TYPE_AVAILABLE && mmap[i].base_addr_low >= mem_top) {
                    mem_top = mmap[i].base_addr_low + mmap[i].length_low;
                } else if (mmap[i].type != MB_MMAP_TYPE_AVAILABLE && mmap[i].base_addr_low >= mem_top) {
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

#if ARCH_X86_32
    if (mem_top > 1*GB) {
        /* trim the memory map to 1GB, since that's what's already mapped in the kernel */
        TRACEF("WARNING: trimming memory to first 1GB\n");
        mem_top = 1*GB;
    }
#endif
    LTRACEF("mem_top 0x%lx\n", mem_top);
}

void platform_early_init(void)
{
    /* get the debug output working */
    platform_init_debug_early();

    /* get the text console working */
    platform_init_console();

    /* initialize the interrupt controller */
    platform_init_interrupts();

    /* initialize the timer */
    platform_init_timer();

    /* look at multiboot to determine our memory size */
    platform_init_multiboot_info();

#ifdef WITH_KERNEL_VM
    mem_arena_init();
    pmm_add_arena(&mem_arena);
#endif
}

void platform_init(void)
{
    platform_init_debug();

    platform_init_keyboard(&console_input_buf);
#if defined(ARCH_X86)
    pci_init();
#endif

    platform_init_mmu_mappings();
}
