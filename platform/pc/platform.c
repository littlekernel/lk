/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2016 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <lk/err.h>
#include <lk/init.h>
#include <lk/trace.h>
#include <arch/x86/mmu.h>
#include <platform.h>
#include "platform_p.h"
#include <platform/pc.h>
#include <platform/multiboot.h>
#include <platform/console.h>
#include <platform/keyboard.h>
#include <dev/uart.h>
#include <arch/x86.h>
#include <arch/mmu.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <kernel/vm.h>
#include <lib/acpi_lite.h>

#if WITH_DEV_BUS_PCI
#include <dev/bus/pci.h>
#endif
#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif

#define LOCAL_TRACE 0

/* multiboot information passed in, if present */
extern multiboot_info_t *_multiboot_info;

#define DEFAULT_MEMEND (16*1024*1024)

extern uint64_t __code_start;
extern uint64_t __code_end;
extern uint64_t __rodata_start;
extern uint64_t __rodata_end;
extern uint64_t __data_start;
extern uint64_t __data_end;
extern uint64_t __bss_start;
extern uint64_t __bss_end;

#if ARCH_X86_64
#define MAX_PHYSICAL_RAM (64ULL*GB)
#elif ARCH_X86_32
#define MAX_PHYSICAL_RAM (1ULL*GB)
#endif

/* Instruct start.S how to set up the initial kernel address space.
 * These data structures are later used by vm routines to lookup pointers
 * to physical pages based on physical addresses.
 */
struct mmu_initial_mapping mmu_initial_mappings[] = {
#if ARCH_X86_64
    /* 64GB of memory mapped where the kernel lives */
    {
        .phys = MEMBASE,
        .virt = KERNEL_ASPACE_BASE,
        .size = MAX_PHYSICAL_RAM, /* x86-64 maps first 64GB by default */
        .flags = 0,
        .name = "physmap"
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

/* based on multiboot (or other methods) we support up to 16 arenas */
#define NUM_ARENAS 16
static pmm_arena_t mem_arena[NUM_ARENAS];

/* Walk through the multiboot structure and attempt to discover all of the runs
 * of physical memory to bootstrap the pmm areas.
 * Returns number of arenas initialized (or error).
 */
static int platform_parse_multiboot_info(void) {
    int count = 0;

    LTRACEF("_multiboot_info %p\n", _multiboot_info);
    if (_multiboot_info) {
        /* bump the multiboot pointer up to the kernel mapping */
        _multiboot_info = (void *)((uintptr_t)_multiboot_info + KERNEL_BASE);
        if (_multiboot_info->flags & MB_INFO_MEM_SIZE) {
            LTRACEF("memory lower %#x\n", _multiboot_info->mem_lower * 1024U);
            LTRACEF("memory upper %#" PRIx64"\n", _multiboot_info->mem_upper * 1024ULL);
        }

        if (_multiboot_info->flags & MB_INFO_MMAP) {
            memory_map_t *mmap = (memory_map_t *)(uintptr_t)_multiboot_info->mmap_addr;
            mmap = (void *)((uintptr_t)mmap + KERNEL_BASE);

            LTRACEF("memory map, length %u:\n", _multiboot_info->mmap_length);
            for (uint i = 0; i < _multiboot_info->mmap_length / sizeof(memory_map_t); i++) {

                uint64_t base = mmap[i].base_addr_low | (uint64_t)mmap[i].base_addr_high << 32;
                uint64_t length = mmap[i].length_low | (uint64_t)mmap[i].length_high << 32;

                LTRACEF("\ttype %u addr %#" PRIx64 " len %#" PRIx64 "\n",
                        mmap[i].type, base, length);
                if (mmap[i].type == MB_MMAP_TYPE_AVAILABLE) {

                    /* do some sanity checks to cut out small arenas */
                    if (length < PAGE_SIZE * 2) {
                        continue;
                    }

                    /* align the base and length */
                    uint64_t oldbase = base;
                    base = PAGE_ALIGN(base);
                    if (base > oldbase) {
                        length -= base - oldbase;
                    }
                    length = ROUNDDOWN(length, PAGE_SIZE);

                    /* ignore memory < 1MB */
                    if (base < 1*MB) {
                        /* skip everything < 1MB */
                        continue;
                    }

                    /* ignore everything that extends past max memory */
                    if (base >= MAX_PHYSICAL_RAM) {
                        continue;
                    }
                    uint64_t end = base + length;
                    if (end > MAX_PHYSICAL_RAM) {
                        end = MAX_PHYSICAL_RAM;
                        DEBUG_ASSERT(end > base);
                        length = end - base;
                        dprintf(INFO, "PC: trimmed memory to %" PRIu64 " bytes\n", MAX_PHYSICAL_RAM);
                    }

                    /* initialize a new pmm arena */
                    mem_arena[count].name = "memory";
                    mem_arena[count].base = base;
                    mem_arena[count].size = length;
                    mem_arena[count].priority = 1;
                    mem_arena[count].flags = PMM_ARENA_FLAG_KMAP;
                    count++;
                    if (count == countof(mem_arena)) {
                        break;
                    }
                }
            }
        }
    }

    return count;
}

void platform_early_init(void) {
    /* get the debug output working */
    platform_init_debug_early();

    /* get the text console working */
    platform_init_console();

    /* initialize the interrupt controller */
    platform_init_interrupts();

    /* initialize the timer */
    platform_init_timer();

    /* look at multiboot to determine our memory size */
    int found_arenas = platform_parse_multiboot_info();
    if (found_arenas <= 0) {
        /* if we couldn't find any memory, initialize a default arena */
        mem_arena[0] = (pmm_arena_t){
            .name = "memory",
            .base = MEMBASE,
            .size = DEFAULT_MEMEND,
            .priority = 1,
            .flags = PMM_ARENA_FLAG_KMAP
        };
        found_arenas = 1;
        printf("PC: WARNING failed to detect memory map from multiboot, using default\n");
    }

    DEBUG_ASSERT(found_arenas > 0 && found_arenas <= (int)countof(mem_arena));

    /* add the arenas we just set up to the pmm */
    uint64_t total_mem = 0;
    for (int i = 0; i < found_arenas; i++) {
        pmm_add_arena(&mem_arena[i]);
        total_mem += mem_arena[i].size;
    }
    dprintf(INFO, "PC: total memory detected %" PRIu64 " bytes\n", total_mem);
}

void local_apic_callback(const void *_entry, size_t entry_len) {
    const struct acpi_madt_local_apic_entry *entry = _entry;

    printf("\tLOCAL APIC id %d, processor id %d, flags %#x\n",
            entry->apic_id, entry->processor_id, entry->flags);
}

void io_apic_callback(const void *_entry, size_t entry_len) {
    const struct acpi_madt_io_apic_entry *entry = _entry;

    printf("\tIO APIC id %d, address %#x gsi base %u\n",
            entry->io_apic_id, entry->io_apic_address, entry->global_system_interrupt_base);
}

void int_source_override_callback(const void *_entry, size_t entry_len) {
    const struct acpi_madt_int_source_override_entry *entry = _entry;

    printf("\tINT OVERRIDE bus %u, source %u, gsi %u, flags %#x\n",
            entry->bus, entry->source, entry->global_sys_interrupt, entry->flags);
}

void platform_init(void) {
    platform_init_debug();

    platform_init_keyboard(&console_input_buf);

#if WITH_DEV_BUS_PCI
    bool pci_initted = false;
    if (acpi_lite_init(0) == NO_ERROR) {
        if (LOCAL_TRACE) {
            acpi_lite_dump_tables();
        }

        // dump the APIC table
        printf("MADT/APIC table:\n");
        acpi_process_madt_entries_etc(ACPI_MADT_TYPE_LOCAL_APIC, &local_apic_callback);
        acpi_process_madt_entries_etc(ACPI_MADT_TYPE_IO_APIC, &io_apic_callback);
        acpi_process_madt_entries_etc(ACPI_MADT_TYPE_INT_SOURCE_OVERRIDE, &int_source_override_callback);

        // try to find the mcfg table
        const struct acpi_mcfg_table *table = (const struct acpi_mcfg_table *)acpi_get_table_by_sig(ACPI_MCFG_SIG);
        if (table) {
            if (table->header.length >= sizeof(*table) + sizeof(struct acpi_mcfg_entry)) {
                const struct acpi_mcfg_entry *entry = (const void *)(table + 1);
                printf("PCI MCFG: segment %#hx bus [%hhu...%hhu] address %#llx\n",
                        entry->segment, entry->start_bus, entry->end_bus, entry->base_address);

                // try to initialize pci based on the MCFG ecam aperture
                status_t err = pci_init_ecam(entry->base_address, entry->segment, entry->start_bus, entry->end_bus);
                if (err == NO_ERROR) {
                    pci_bus_mgr_init();
                    pci_initted = true;
                }
            }
        }
    }

    // fall back to legacy pci if we couldn't find the pcie aperture
    if (!pci_initted) {
        status_t err = pci_init_legacy();
        if (err == NO_ERROR) {
            pci_bus_mgr_init();
        }
    }
#endif

    platform_init_mmu_mappings();
}

#if WITH_LIB_MINIP
void _start_minip(uint level) {
    extern status_t e1000_register_with_minip(void);
    status_t err = e1000_register_with_minip();
    if (err == NO_ERROR) {
        minip_start_dhcp();
    }
}

LK_INIT_HOOK(start_minip, _start_minip, LK_INIT_LEVEL_APPS - 1);
#endif
