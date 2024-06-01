/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/compiler.h>
#include <sys/types.h>

__BEGIN_CDECLS

/*
 * A set of routines to assist with walking a Flattened Device Tree in memory
 * for interesting nodes. Uses libfdt internally.
 */

struct fdt_walk_pcie_info {
    // location of the ECAM and the pci ranges it covers
    uint64_t ecam_base;
    uint64_t ecam_len;
    uint8_t bus_start;
    uint8_t bus_end;

    // discovered io and mmio apertures
    uint64_t io_base;
    uint64_t io_base_mmio;
    uint64_t io_len;
    uint64_t mmio_base;
    uint64_t mmio_len;
    uint64_t mmio64_base;
    uint64_t mmio64_len;
};

struct fdt_walk_memory_region {
    uint64_t base;
    uint64_t len;
};

struct fdt_walk_cpu_info {
    uint32_t id;
#if ARCH_RISCV
    const char *isa_string; // pointer to riscv,isa inside device tree
    const char *isa_extensions_string; // pointer to riscv,isa-etensions inside device tree
#endif
};

status_t fdt_walk_dump(const void *fdt);

// New style walkers, finds a single topic at a time
status_t fdt_walk_find_pcie_info(const void *fdt, struct fdt_walk_pcie_info *, size_t *count);
status_t fdt_walk_find_memory(const void *fdt, struct fdt_walk_memory_region *memory, size_t *mem_count,
                              struct fdt_walk_memory_region *reserved_memory, size_t *reserved_mem_count);
status_t fdt_walk_find_cpus(const void *fdt, struct fdt_walk_cpu_info *cpu, size_t *cpu_count);

// Helper routines that initialize various subsystems based on device tree info
status_t fdtwalk_setup_memory(const void *fdt, paddr_t fdt_phys, paddr_t default_mem_base, size_t default_mem_size);
#if ARCH_RISCV
status_t fdtwalk_setup_cpus_riscv(const void *fdt);
#endif
#if ARCH_ARM || ARCH_ARM64
status_t fdtwalk_setup_cpus_arm(const void *fdt);
#endif
#if WITH_DEV_BUS_PCI
status_t fdtwalk_setup_pci(const void *fdt);
#endif
status_t fdtwalk_reserve_fdt_memory(const void *fdt, paddr_t fdt_phys);

__END_CDECLS
