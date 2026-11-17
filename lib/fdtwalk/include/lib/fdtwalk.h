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

#define FDT_WALK_PCIE_MAX_INTX_MAP 32

// One interrupt-map entry of a pcie node: the child unit address (phys.hi, phys.mid, phys.lo)
// and pin it matches, as written, and the parent interrupt decoded to a platform vector.
struct fdt_walk_pcie_intx_map_entry {
    uint32_t child[4];
    uint32_t vector;
};

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

    // prefetchable mmio apertures, 0 length when the node has none
    uint64_t mmio_prefetch_base;
    uint64_t mmio_prefetch_len;
    uint64_t mmio64_prefetch_base;
    uint64_t mmio64_prefetch_len;

    // interrupt-map, with the entries whose parent controller could be decoded (a GIC or a
    // single cell controller such as the PLIC); intx_map_count is 0 when there are none
    uint32_t intx_map_mask[4];
    size_t intx_map_count;
    struct fdt_walk_pcie_intx_map_entry intx_map[FDT_WALK_PCIE_MAX_INTX_MAP];
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

#if ARCH_RISCV
// A PLIC interrupt target (context) and the hart it delivers to.
struct fdt_walk_plic_context {
    uint32_t hart_id;
    uint32_t context;
};
#endif

#define FDT_WALK_MAX_GIC_ITS 4
#define FDT_WALK_MAX_GIC_V2M 4

// An MSI frame below a GIC node: a GICv2m frame or a GICv3 ITS
struct fdt_walk_gic_msi_frame {
    uint64_t base;
    uint64_t len;
    uint32_t phandle; // 0 when the node has none
};

struct fdt_walk_gic_info {
    uint8_t gic_version;

    uint32_t interrupt_cells;
    uint32_t phandle; // of the gic node itself, 0 when it has none

    // v2 specific ranges
    union {
        struct {
            uint64_t distributor_base;
            uint64_t distributor_len;
            uint64_t cpu_interface_base;
            uint64_t cpu_interface_len;
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
        } v3;
    };

    // MSI frame subnodes, recorded whatever the gic version: a GICv2m frame normally sits
    // under a GICv2, but QEMU's hvf irqchip puts one under a GICv3
    size_t v2m_count;
    struct fdt_walk_gic_msi_frame v2m_frame[FDT_WALK_MAX_GIC_V2M];
    size_t its_count;
    struct fdt_walk_gic_msi_frame its[FDT_WALK_MAX_GIC_ITS];
};

status_t fdt_walk_dump(const void *fdt);

// New style walkers, finds a single topic at a time
status_t fdt_walk_find_pcie_info(const void *fdt, struct fdt_walk_pcie_info *, size_t *count);
// Look up the vector the interrupt-map of a pcie node gives a pin (1..4) of the function at
// bus/dev/fn. The caller has already swizzled the pin across any bridges, so this is the pin
// as it arrives at the host bridge. ERR_NOT_FOUND when nothing in the map matches.
status_t fdt_walk_pcie_lookup_intx(const struct fdt_walk_pcie_info *info, uint8_t bus, uint8_t dev,
                                   uint8_t fn, uint8_t pin, uint32_t *vector);
status_t fdt_walk_find_gic_info(const void *fdt, struct fdt_walk_gic_info *, size_t *count);
status_t fdt_walk_find_memory(const void *fdt, struct fdt_walk_memory_region *memory, size_t *mem_count,
                              struct fdt_walk_memory_region *reserved_memory, size_t *reserved_mem_count);
status_t fdt_walk_find_cpus(const void *fdt, struct fdt_walk_cpu_info *cpu, size_t *cpu_count);
#if ARCH_RISCV
// The PLIC contexts that raise interrupt cause 'cause' (RISCV_INTERRUPT_SEXT or _MEXT) on a hart.
status_t fdt_walk_find_plic_contexts(const void *fdt, uint32_t cause,
                                     struct fdt_walk_plic_context *context, size_t *context_count);
#endif

// Helper routines that initialize various subsystems based on device tree info
status_t fdtwalk_setup_memory(const void *fdt, paddr_t fdt_phys, paddr_t default_mem_base, size_t default_mem_size);
status_t fdtwalk_reserve_fdt_memory(const void *fdt, paddr_t fdt_phys);
#if WITH_LIB_CMDLINE
status_t fdtwalk_setup_cmdline(const void *fdt);
#endif
#if ARCH_RISCV
status_t fdtwalk_setup_cpus_riscv(const void *fdt);
#endif
#if ARCH_ARM || ARCH_ARM64
status_t fdtwalk_setup_cpus_arm(const void *fdt);
#endif
#if WITH_DEV_BUS_PCI
status_t fdtwalk_setup_pci(const void *fdt);
#endif
#if WITH_DEV_INTERRUPT_ARM_GIC
status_t fdtwalk_setup_gic(const void *fdt);
#endif
#if WITH_DEV_INTERRUPT_RISCV_PLIC
// Records each hart's PLIC context for the current mode; call before plic_early_init().
status_t fdtwalk_setup_plic(const void *fdt);
#endif

__END_CDECLS
