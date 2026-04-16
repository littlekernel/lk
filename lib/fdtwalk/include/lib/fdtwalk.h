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

#define FDT_WALK_PCIE_MAX_INTERRUPT_MAP_ENTRIES  64
#define FDT_WALK_PCIE_MAX_INTERRUPT_MAP_CELLS    8
#define FDT_WALK_PCIE_MAX_INTERRUPT_PARENT_CELLS 4

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
    uint64_t mmio_prefetch_base;
    uint64_t mmio_prefetch_len;
    uint64_t mmio64_prefetch_base;
    uint64_t mmio64_prefetch_len;

    // interrupt-map-mask and interrupt-map decode (INTx routing)
    bool has_interrupt_map_mask;
    uint32_t interrupt_map_mask_cells;
    uint32_t interrupt_map_mask[FDT_WALK_PCIE_MAX_INTERRUPT_MAP_CELLS];

    bool has_interrupt_map;
    bool interrupt_map_truncated;
    uint32_t interrupt_map_child_addr_cells;
    uint32_t interrupt_map_child_interrupt_cells;
    size_t interrupt_map_entry_count;
    struct interrupt_map_entry {
        uint32_t child_addr[FDT_WALK_PCIE_MAX_INTERRUPT_MAP_CELLS];
        uint32_t child_interrupt[FDT_WALK_PCIE_MAX_INTERRUPT_MAP_CELLS];

        uint32_t parent_phandle;
        uint32_t parent_addr_cells;
        uint32_t parent_interrupt_cells;
        uint32_t parent_addr[FDT_WALK_PCIE_MAX_INTERRUPT_MAP_CELLS];
        uint32_t parent_interrupt[FDT_WALK_PCIE_MAX_INTERRUPT_PARENT_CELLS];
    } interrupt_map_entry[FDT_WALK_PCIE_MAX_INTERRUPT_MAP_ENTRIES];
};

struct fdt_walk_memory_region {
    uint64_t base;
    uint64_t len;
};

struct fdt_walk_cpu_info {
    uint32_t id;
#if ARCH_RISCV
    const char *isa_string;            // pointer to riscv,isa inside device tree
    const char *isa_extensions_string; // pointer to riscv,isa-etensions inside device tree
#endif
};

struct fdt_walk_pci_int_route {
    uint32_t parent_phandle;
    uint32_t parent_interrupt_cells;
    uint32_t parent_interrupt[FDT_WALK_PCIE_MAX_INTERRUPT_PARENT_CELLS];
};

#define FDT_WALK_MAX_GIC_ITS 4
#define FDT_WALK_MAX_GIC_V2M 4

struct fdt_walk_gic_info {
    uint8_t gic_version;

    uint32_t interrupt_cells;

    // v2 specific ranges
    union {
        struct {
            uint64_t distributor_base;
            uint64_t distributor_len;
            uint64_t cpu_interface_base;
            uint64_t cpu_interface_len;

            // GICv2m (MSI) frame subnodes
            size_t v2m_count;
            struct {
                uint64_t base;
                uint64_t len;
            } v2m_frame[FDT_WALK_MAX_GIC_V2M];
        } v2;
        struct {
            uint64_t distributor_base;
            uint64_t distributor_len;
            uint64_t redistributor_base;
            uint64_t redistributor_len;
            uint64_t cpu_interface_base;
            uint64_t cpu_interface_len;
            uint64_t hypervisor_interface_base;
            uint64_t hypervisor_interface_len;
            uint64_t virtual_interface_base;
            uint64_t virtual_interface_len;

            // ITS (Interrupt Translation Service) subnodes
            size_t its_count;
            struct {
                uint64_t base;
                uint64_t len;
            } its[FDT_WALK_MAX_GIC_ITS];
        } v3;
    };
};

status_t fdt_walk_dump(const void *fdt);

// New style walkers, finds a single topic at a time
status_t fdt_walk_find_pcie_info(const void *fdt, struct fdt_walk_pcie_info *, size_t *count);
// Register caller-owned PCIe info storage used by fdt_walk_pcie_lookup_intx().
// The pointer must remain valid for as long as lookups may occur.
status_t fdt_walk_register_pcie_info(const struct fdt_walk_pcie_info *info, size_t count);
status_t fdt_walk_pcie_lookup_intx(uint8_t bus, uint8_t dev, uint8_t fn, uint8_t int_pin,
                                   struct fdt_walk_pci_int_route *route);
status_t fdt_walk_find_gic_info(const void *fdt, struct fdt_walk_gic_info *, size_t *count);
status_t fdt_walk_find_memory(const void *fdt, struct fdt_walk_memory_region *memory, size_t *mem_count,
                              struct fdt_walk_memory_region *reserved_memory, size_t *reserved_mem_count);
status_t fdt_walk_find_cpus(const void *fdt, struct fdt_walk_cpu_info *cpu, size_t *cpu_count);

// Helper routines that initialize various subsystems based on device tree info
status_t fdtwalk_setup_memory(const void *fdt, paddr_t fdt_phys, paddr_t default_mem_base, size_t default_mem_size);
status_t fdtwalk_reserve_fdt_memory(const void *fdt, paddr_t fdt_phys);
#if ARCH_RISCV
status_t fdtwalk_setup_cpus_riscv(const void *fdt);
#endif
#if ARCH_ARM || ARCH_ARM64
status_t fdtwalk_setup_cpus_arm(const void *fdt);
#endif
#if WITH_DEV_BUS_PCI
status_t fdtwalk_setup_pci(const void *fdt, struct fdt_walk_pcie_info *pcie_info, size_t *count);
#endif
#if WITH_DEV_INTERRUPT_ARM_GIC
status_t fdtwalk_setup_gic(const void *fdt);
#endif

__END_CDECLS
