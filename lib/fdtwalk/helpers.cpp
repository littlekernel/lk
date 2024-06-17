/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/fdtwalk.h>

#include <inttypes.h>
#include <assert.h>
#include <libfdt.h>
#include <lk/cpp.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdio.h>
#include <sys/types.h>
#if WITH_KERNEL_VM
#include <kernel/vm.h>
#else
#include <kernel/novm.h>
#endif
#if ARCH_RISCV
#include <arch/riscv.h>
#include <arch/riscv/feature.h>
#endif
#if ARCH_ARM || ARCH_ARM64
#include <dev/power/psci.h>
#endif
#if WITH_DEV_BUS_PCI
#include <dev/bus/pci.h>
#endif

// A few helper routines to configure subsystems such as cpu, memory, and pci busses based
// on the information in a device tree.
//
// May eventually move some of these to other locations, but for now dump the helpers here.

#define LOCAL_TRACE 0

status_t fdtwalk_reserve_fdt_memory(const void *fdt, paddr_t fdt_phys) {
    if (fdt_check_header(fdt) != 0) {
        return ERR_NOT_FOUND;
    }

    uint32_t length = fdt_totalsize(fdt);

    paddr_t base = fdt_phys;
    base = PAGE_ALIGN(base);
    length = ROUNDUP(length, PAGE_SIZE);

    dprintf(INFO, "FDT: reserving physical range for FDT: [%#lx, %#lx]\n", base, base + length - 1);

#if WITH_KERNEL_VM
    struct list_node list = LIST_INITIAL_VALUE(list);
    pmm_alloc_range(base, length / PAGE_SIZE, &list);
#else
    novm_alloc_specific_pages((void *)base, length / PAGE_SIZE);
#endif
    return NO_ERROR;
}

status_t fdtwalk_setup_memory(const void *fdt, paddr_t fdt_phys, paddr_t default_mem_base, size_t default_mem_size) {
#if WITH_KERNEL_VM
    // TODO: consider having pmm make a copy so that this doesn't have to be static
    static pmm_arena_t arenas[8];
#endif

    struct fdt_walk_memory_region mem[8];
    struct fdt_walk_memory_region reserved_mem[16];
    size_t mem_count = countof(mem);
    size_t reserved_mem_count = countof(reserved_mem);

    // find memory from the FDT
    status_t err = fdt_walk_find_memory(fdt, mem, &mem_count, reserved_mem, &reserved_mem_count);
    if (err < NO_ERROR || mem_count == 0) {
        /* add a default memory region if we didn't find it in the FDT */
        dprintf(INFO, "FDT: could not find memory, using default base %#lx size %#zx\n", default_mem_base, default_mem_size);
#if WITH_KERNEL_VM
        mem[0].base = default_mem_base;
        mem[0].len = default_mem_size;
        mem_count = 1;
#endif
    }

    for (size_t i = 0; i < mem_count; i++) {
        LTRACEF("base %#llx len %#llx\n", mem[i].base, mem[i].len);
        dprintf(INFO, "FDT: found memory bank range [%#llx, %#llx] (length %#llx)\n", mem[i].base, mem[i].base + mem[i].len - 1, mem[i].len);

        /* trim size on certain platforms */
#if ARCH_ARM || (ARCH_RISCV && __riscv_xlen == 32)
        /* only use the first 1GB on ARM32 */
        const auto GB = 1024*1024*1024UL;
        if (mem[i].base - MEMBASE > GB) {
            dprintf(INFO, "trimming memory to 1GB\n");
            continue;
        }
        if (mem[i].base - MEMBASE + mem[i].len > GB) {
            dprintf(INFO, "trimming memory to 1GB\n");
            mem[i].len = MEMBASE + GB - mem[i].base;
            dprintf(INFO, "range is now [%#llx, %#llx]\n", mem[i].base, mem[i].base + mem[i].len - 1);
        }
#endif

#if WITH_KERNEL_VM
        if (i >= countof(arenas)) {
            printf("FDT: found too many arenas, max is %zu\n", countof(arenas));
            break;
        }

        /* add a vm arena */
        arenas[i].name = "fdt";
        arenas[i].base = mem[i].base;
        arenas[i].size = mem[i].len;
        arenas[i].flags = PMM_ARENA_FLAG_KMAP;
        pmm_add_arena(&arenas[i]);
#else
        novm_add_arena("fdt", mem[i].base, mem[i].len);
#endif
    }

    /* reserve memory described by the FDT */
    for (size_t i = 0; i < reserved_mem_count; i++) {
        dprintf(INFO, "FDT: reserving memory range [%#llx, %#llx]\n",
                reserved_mem[i].base, reserved_mem[i].base + reserved_mem[i].len - 1);

#if WITH_KERNEL_VM
        // allocate the range and place on a list
        struct list_node list = LIST_INITIAL_VALUE(list);
        pmm_alloc_range(reserved_mem[i].base, reserved_mem[i].len / PAGE_SIZE, &list);
#else
        novm_alloc_specific_pages((void *)reserved_mem[i].base, reserved_mem[i].len / PAGE_SIZE);
#endif
    }

    // TODO: deal with fdt reserved memory sections with
    // fdt_num_mem_rsv and
    // fdt_get_mem_rsv

    // reserve the memory the device tree itself uses
    fdtwalk_reserve_fdt_memory(fdt, fdt_phys);

    return NO_ERROR;
}

#if ARCH_RISCV
status_t fdtwalk_setup_cpus_riscv(const void *fdt) {
#if WITH_SMP
    struct fdt_walk_cpu_info cpus[SMP_MAX_CPUS];
    size_t cpu_count = countof(cpus);

    status_t err = fdt_walk_find_cpus(fdt, cpus, &cpu_count);
    if (err >= NO_ERROR) {
        const char *isa_string = {};

        if (cpu_count > 0) {
            dprintf(INFO, "FDT: found %zu cpu%c\n", cpu_count, cpu_count == 1 ? ' ' : 's');
            uint harts[SMP_MAX_CPUS - 1];

            // copy from the detected cpu list to an array of harts, excluding the boot hart
            size_t hart_index = 0;
            for (size_t i = 0; i < cpu_count; i++) {
                if (cpus[i].id != riscv_current_hart()) {
                    harts[hart_index++] = cpus[i].id;
                }

                // we can start MAX CPUS - 1 secondaries
                if (hart_index >= SMP_MAX_CPUS - 1) {
                    break;
                }

                if (cpus[i].isa_string) {
                    if (!isa_string) {
                        // save the first isa string we found
                        isa_string = cpus[i].isa_string;
                    } else {
                        if (strcmp(cpus[i].isa_string, isa_string) != 0) {
                            printf("FDT Warning: isa_strings do not match between cpus, using first found\n");
                        }
                    }
                }

            }

            // tell the riscv layer how many cores we have to start
            if (hart_index > 0) {
                riscv_set_secondary_harts_to_start(harts, hart_index);
            }

            if (isa_string) {
                dprintf(INFO, "FDT: isa string '%s'\n", isa_string);
                riscv_set_isa_string(isa_string);
            }
        }
    }
#endif

    return err;
}
#endif

#if ARCH_ARM || ARCH_ARM64
status_t fdtwalk_setup_cpus_arm(const void *fdt) {
#if WITH_SMP
    struct fdt_walk_cpu_info cpus[SMP_MAX_CPUS];
    size_t cpu_count = countof(cpus);

    status_t err = fdt_walk_find_cpus(fdt, cpus, &cpu_count);
    if (err >= NO_ERROR) {
        if (cpu_count > 0) {
            dprintf(INFO, "FDT: found %zu cpu%c\n", cpu_count, cpu_count == 1  ? ' ' : 's');

            if (cpu_count > SMP_MAX_CPUS) {
                cpu_count = MIN(cpu_count, SMP_MAX_CPUS);
                dprintf(INFO, "FDT: clamping max cpus to %zu\n", cpu_count);
            }

            LTRACEF("booting %zu cpus\n", cpu_count);

            /* boot the secondary cpus using the Power State Coordintion Interface */
            for (size_t i = 1; i < cpu_count; i++) {
                /* note: assumes cpuids are numbered like MPIDR 0:0:0:N */
                dprintf(INFO, "ARM: starting cpu %#x\n", cpus[i].id);
                int ret = psci_cpu_on(cpus[i].id, MEMBASE + KERNEL_LOAD_OFFSET);
                if (ret != 0) {
                    printf("ERROR: psci CPU_ON returns %d\n", ret);
                }
            }
        }
    }
#endif

    return err;
}
#endif

#if WITH_DEV_BUS_PCI
status_t fdtwalk_setup_pci(const void *fdt) {
    /* detect pci */
    struct fdt_walk_pcie_info pcie_info[4] = {};

    size_t count = countof(pcie_info);
    status_t err = fdt_walk_find_pcie_info(fdt, pcie_info, &count);
    LTRACEF("fdt_walk_find_pcie_info returns %d, count %zu\n", err, count);
    if (err == NO_ERROR) {
        for (size_t i = 0; i < count; i++) {
            LTRACEF("ecam base %#" PRIx64 ", len %#" PRIx64 ", bus_start %hhu, bus_end %hhu\n", pcie_info[i].ecam_base,
                    pcie_info[i].ecam_len, pcie_info[i].bus_start, pcie_info[i].bus_end);

            // currently can only handle the first segment
            if (i > 0) {
                printf("skipping pci segment %zu, not supported (yet)\n", i);
                continue;
            }

            if (pcie_info[i].ecam_len > 0) {
                dprintf(INFO, "PCIE: initializing pcie with ecam at %#" PRIx64 " found in FDT\n", pcie_info[i].ecam_base);
                err = pci_init_ecam(pcie_info[i].ecam_base, pcie_info[i].ecam_len, pcie_info[i].bus_start, pcie_info[i].bus_end);
                if (err == NO_ERROR) {
                    // add some additional resources to the pci bus manager in case it needs to configure
                    if (pcie_info[i].io_len > 0) {
                        // we can only deal with a mapping of io base 0 to the mmio base
                        DEBUG_ASSERT(pcie_info[i].io_base == 0);
                        pci_bus_mgr_add_resource(PCI_RESOURCE_IO_RANGE, pcie_info[i].io_base, pcie_info[i].io_len);

                        // TODO: set the mmio base somehow so pci knows what to do with it
                    }
                    if (pcie_info[i].mmio_len > 0) {
                        pci_bus_mgr_add_resource(PCI_RESOURCE_MMIO_RANGE, pcie_info[i].mmio_base, pcie_info[i].mmio_len);
                    }
                    if (sizeof(void *) >= 8) {
                        if (pcie_info[i].mmio64_len > 0) {
                            pci_bus_mgr_add_resource(PCI_RESOURCE_MMIO64_RANGE, pcie_info[i].mmio64_base, pcie_info[i].mmio64_len);
                        }
                    }
                }
            }
        }
    }

    return err;
}
#endif

