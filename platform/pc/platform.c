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
#include <arch/x86/apic.h>
#include <platform.h>
#include <platform/pc.h>
#include <platform/console.h>
#include <platform/keyboard.h>
#include <dev/uart.h>
#include <hw/multiboot.h>
#include <arch/x86.h>
#include <arch/mmu.h>
#include <malloc.h>
#include <assert.h>
#include <inttypes.h>
#include <kernel/vm.h>

#include "platform_p.h"

#if WITH_DEV_BUS_PCI
#include <dev/bus/pci.h>
#endif
#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif
#if WITH_LIB_ACPI_LITE
#include <lib/acpi_lite.h>

static bool found_acpi = false;
#endif

#define LOCAL_TRACE 0

/* multiboot information passed in, if present */
extern uint32_t _multiboot_info;

#define DEFAULT_MEMEND (16*1024*1024)

extern uint64_t __code_start;
extern uint64_t __code_end;
extern uint64_t __rodata_start;
extern uint64_t __rodata_end;
extern uint64_t __data_start;
extern uint64_t __data_end;
extern uint64_t __bss_start;
extern uint64_t __bss_end;

/* based on multiboot (or other methods) we support up to 16 arenas */
#define NUM_ARENAS 16
static pmm_arena_t mem_arena[NUM_ARENAS];

/* parse an array of multiboot mmap entries */
static status_t parse_multiboot_mmap(const memory_map_t *mmap, const size_t mmap_length, size_t *found_mem_arenas) {
    for (uint i = 0; i < mmap_length / sizeof(memory_map_t); i++) {

        uint64_t base = mmap[i].base_addr_low | (uint64_t)mmap[i].base_addr_high << 32;
        uint64_t length = mmap[i].length_low | (uint64_t)mmap[i].length_high << 32;

        dprintf(SPEW, "\ttype %u addr %#" PRIx64 " len %#" PRIx64 "\n",
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

            /* ignore everything that extends past the size PHYSMAP maps into the kernel.
             * see arch/x86/arch.c mmu_initial_mappings
             */
            if (base >= PHYSMAP_SIZE) {
                continue;
            }
            uint64_t end = base + length;
            if (end > PHYSMAP_SIZE) {
                end = PHYSMAP_SIZE;
                DEBUG_ASSERT(end > base);
                length = end - base;
                dprintf(INFO, "PC: trimmed memory to %" PRIu64 " bytes\n", PHYSMAP_SIZE);
            }

            /* initialize a new pmm arena */
            mem_arena[*found_mem_arenas].name = "memory";
            mem_arena[*found_mem_arenas].base = base;
            mem_arena[*found_mem_arenas].size = length;
            mem_arena[*found_mem_arenas].priority = 1;
            mem_arena[*found_mem_arenas].flags = PMM_ARENA_FLAG_KMAP;
            (*found_mem_arenas)++;
            if (*found_mem_arenas == countof(mem_arena)) {
                break;
            }
        }
    }

    return NO_ERROR;
}

/* Walk through the multiboot structure and attempt to discover all of the runs
 * of physical memory to bootstrap the pmm areas.
 * Returns number of arenas initialized in passed in pointer
 */
static status_t platform_parse_multiboot_info(size_t *found_mem_arenas) {
    *found_mem_arenas = 0;

    dprintf(SPEW, "PC: multiboot address %#" PRIx32 "\n", _multiboot_info);
    if (_multiboot_info == 0) {
        return ERR_NOT_FOUND;
    }

    /* bump the multiboot pointer up to the kernel mapping */
    /* TODO: test that it's within range of the kernel mapping */
    const multiboot_info_t *multiboot_info = (void *)((uintptr_t)_multiboot_info + KERNEL_BASE);

    dprintf(SPEW, "\tflags %#x\n", multiboot_info->flags);

    // legacy multiboot memory size field
    if (multiboot_info->flags & MB_INFO_MEM_SIZE) {
        dprintf(SPEW, "PC: multiboot memory lower %#x upper %#" PRIx64 "\n",
                multiboot_info->mem_lower * 1024U, multiboot_info->mem_upper * 1024ULL);
        if ((multiboot_info->flags & MB_INFO_MMAP) == 0) {
            // There is no mmap to give us a more detailed memory map
            // so we'll need to use this one. Synthesize a fake mmap array to pass
            // to the mmap code.
            memory_map_t mmap[2] = {};
            mmap[0].length_low = multiboot_info->mem_lower * 1024U;
            mmap[0].type = MB_MMAP_TYPE_AVAILABLE;
            mmap[1].base_addr_low = 1 * 1024U * 1024U;
            mmap[1].length_low = multiboot_info->mem_upper * 1024U;
            mmap[1].type = MB_MMAP_TYPE_AVAILABLE;
            parse_multiboot_mmap(mmap, 2 * sizeof(memory_map_t), found_mem_arenas);
        }
    }

    // more modern multiboot mmap array
    if (multiboot_info->flags & MB_INFO_MMAP) {
        const memory_map_t *mmap = (const memory_map_t *)(uintptr_t)multiboot_info->mmap_addr;
        mmap = (void *)((uintptr_t)mmap + KERNEL_BASE);

        dprintf(SPEW, "PC: multiboot memory map, length %u:\n", multiboot_info->mmap_length);
        parse_multiboot_mmap(mmap, multiboot_info->mmap_length, found_mem_arenas);
    }

    if (multiboot_info->flags & MB_INFO_FRAMEBUFFER) {
        dprintf(SPEW, "PC: multiboot framebuffer info present\n");
        dprintf(SPEW, "\taddress %#" PRIx64 " pitch %u width %u height %u bpp %hhu type %u\n",
                multiboot_info->framebuffer_addr, multiboot_info->framebuffer_pitch,
                multiboot_info->framebuffer_width, multiboot_info->framebuffer_height,
                multiboot_info->framebuffer_bpp, multiboot_info->framebuffer_type);

        if (multiboot_info->framebuffer_type == MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
            dprintf(SPEW, "\tcolor bit layout: R %u:%u G %u:%u B %u:%u\n",
                    multiboot_info->framebuffer_red_field_position, multiboot_info->framebuffer_red_mask_size,
                    multiboot_info->framebuffer_green_field_position, multiboot_info->framebuffer_green_mask_size,
                    multiboot_info->framebuffer_blue_field_position, multiboot_info->framebuffer_blue_mask_size);
        }
    }

    return NO_ERROR;
}

void platform_early_init(void) {
    /* get the debug output working */
    platform_init_debug_early();

    /* get the text console working */
    platform_init_console();

    /* initialize the interrupt controller */
    platform_init_interrupts();

    /* look at multiboot to determine our memory size */
    size_t found_arenas;
    platform_parse_multiboot_info(&found_arenas);
    if (found_arenas <= 0) {
        /* if we couldn't find any memory, initialize a default arena */
        mem_arena[0] = (pmm_arena_t) {
            .name = "memory",
            .base = MEMBASE,
            .size = DEFAULT_MEMEND,
            .priority = 1,
            .flags = PMM_ARENA_FLAG_KMAP
        };
        found_arenas = 1;
        printf("PC: WARNING failed to detect memory map from multiboot, using default\n");
    }

    DEBUG_ASSERT(found_arenas > 0 && found_arenas <= countof(mem_arena));

    /* add the arenas we just set up to the pmm */
    uint64_t total_mem = 0;
    for (size_t i = 0; i < found_arenas; i++) {
        pmm_add_arena(&mem_arena[i]);
        total_mem += mem_arena[i].size;
    }
    dprintf(INFO, "PC: total memory detected %" PRIu64 " bytes\n", total_mem);
}

// Look for the ACPI tables just after the vm is initialized.
void platform_init_postvm(uint level) {
#if WITH_LIB_ACPI_LITE
    // Look for the root ACPI table
    status_t err = acpi_lite_init(0);
    if (err != NO_ERROR) {
        return;
    }
    found_acpi = true;

    if (LOCAL_TRACE) {
        acpi_lite_dump_tables(false);
    }
    acpi_lite_dump_madt_table();
#endif

    platform_init_interrupts_postvm();
    platform_init_timer();
}

LK_INIT_HOOK(platform_init_postvm, platform_init_postvm, LK_INIT_LEVEL_VM);

void platform_init(void) {
    platform_init_debug();

    platform_init_keyboard(&console_input_buf);

    // Look for secondary cpus
#if WITH_SMP
    platform_start_secondary_cpus();
#endif

#if WITH_DEV_BUS_PCI
    bool pci_initted = false;
    if (found_acpi) {
        // TODO: handle interrupt source overrides from the MADT table

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
