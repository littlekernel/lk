/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch.h>
#include <inttypes.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <dev/bus/pci.h>
#include <dev/interrupt/arm_gic.h>
#include <dev/power/psci.h>
#include <dev/timer/arm_generic.h>
#include <dev/uart.h>
#include <dev/virtio.h>
#include <dev/virtio/net.h>
#include <lib/fdtwalk.h>
#include <lk/init.h>
#include <kernel/vm.h>
#include <kernel/spinlock.h>
#include <platform.h>
#include <platform/gic.h>
#include <platform/interrupts.h>
#include <platform/qemu-virt.h>
#include "platform_p.h"

#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif

#define LOCAL_TRACE 0

#define DEFAULT_MEMORY_SIZE (MEMSIZE) /* try to fetch from the emulator via the fdt */

/* initial memory mappings. parsed by start.S */
struct mmu_initial_mapping mmu_initial_mappings[] = {
    /* all of memory */
    {
        .phys = MEMORY_BASE_PHYS,
        .virt = KERNEL_BASE,
        .size = MEMORY_APERTURE_SIZE,
        .flags = 0,
        .name = "memory"
    },

    /* 1GB of peripherals */
    {
        .phys = PERIPHERAL_BASE_PHYS,
        .virt = PERIPHERAL_BASE_VIRT,
        .size = PERIPHERAL_BASE_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "peripherals"
    },

    /* null entry to terminate the list */
    { 0 }
};

static pmm_arena_t arena = {
    .name = "ram",
    .base = MEMORY_BASE_PHYS,
    .size = DEFAULT_MEMORY_SIZE,
    .flags = PMM_ARENA_FLAG_KMAP,
};

// callbacks to the fdt_walk routine
static void memcallback(uint64_t base, uint64_t len, void *cookie) {
    bool *found_mem = (bool *)cookie;

    LTRACEF("base %#llx len %#llx cookie %p\n", base, len, cookie);

    /* add another novm arena */
    if (!*found_mem) {
        printf("FDT: found memory arena, base %#llx size %#llx\n", base, len);

        /* trim size on certain platforms */
#if ARCH_ARM
        if (len > 1024*1024*1024U) {
            len = 1024*1024*1024; /* only use the first 1GB on ARM32 */
            printf("trimming memory to 1GB\n");
        }
#endif

        /* set the size in the pmm arena */
        arena.size = len;

        *found_mem = true; // stop searching after the first one
    }
}

static void cpucallback(uint64_t id, void *cookie) {
    int *cpu_count = (int *)cookie;

    LTRACEF("id %#llx cookie %p\n", id, cookie);

    (*cpu_count)++;
}

struct pcie_detect_state {
    struct fdt_walk_pcie_info info;
} pcie_state;

static void pciecallback(const struct fdt_walk_pcie_info *info, void *cookie) {
    struct pcie_detect_state *state = cookie;

    LTRACEF("ecam base %#llx, len %#llx, bus_start %hhu, bus_end %hhu\n", info->ecam_base, info->ecam_len, info->bus_start, info->bus_end);
    state->info = *info;
}

void platform_early_init(void) {
    /* initialize the interrupt controller */
    arm_gic_init();

    arm_generic_timer_init(ARM_GENERIC_TIMER_PHYSICAL_INT, 0);

    uart_init_early();

    int cpu_count = 0;
    bool found_mem = false;
    struct fdt_walk_callbacks cb = {
        .mem = memcallback,
        .memcookie = &found_mem,
        .cpu = cpucallback,
        .cpucookie = &cpu_count,
        .pcie = pciecallback,
        .pciecookie = &pcie_state,
    };

    const void *fdt = (void *)KERNEL_BASE;
    status_t err = fdt_walk(fdt, &cb);
    LTRACEF("fdt_walk returns %d\n", err);

    if (err != 0) {
        printf("FDT: error finding FDT at %p, using default memory & cpu count\n", fdt);
    }

    /* add the main memory arena */
    pmm_add_arena(&arena);

    /* reserve the first 64k of ram, which should be holding the fdt */
    struct list_node list = LIST_INITIAL_VALUE(list);
    pmm_alloc_range(MEMBASE, 0x10000 / PAGE_SIZE, &list);

    /* count the number of secondary cpus */
    if (cpu_count == 0) {
        /* if we didn't find any in the FDT, assume max number */
        cpu_count = SMP_MAX_CPUS;
    } else if (cpu_count > 0) {
        printf("FDT: found %d cpus\n", cpu_count);
        cpu_count = MIN(cpu_count, SMP_MAX_CPUS);
    }

    LTRACEF("booting %d cpus\n", cpu_count);

    /* boot the secondary cpus using the Power State Coordintion Interface */
    for (int cpuid = 1; cpuid < cpu_count; cpuid++) {
        /* note: assumes cpuids are numbered like MPIDR 0:0:0:N */
        int ret = psci_cpu_on(cpuid, MEMBASE + KERNEL_LOAD_OFFSET);
        if (ret != 0) {
            printf("ERROR: psci CPU_ON returns %d\n", ret);
        }
    }
}

void platform_init(void) {
    status_t err;

    uart_init();

    /* detect pci */
#if ARCH_ARM
    if (pcie_state.info.ecam_base > (1ULL << 32)) {
        // dont try to configure this since we dont have LPAE support
        printf("PCIE: skipping pci initialization due to high memory ECAM\n");
        pcie_state.info.ecam_len = 0;
    }
#endif
    if (pcie_state.info.ecam_len > 0) {
        printf("PCIE: initializing pcie with ecam at %#" PRIx64 " found in FDT\n", pcie_state.info.ecam_base);
        err = pci_init_ecam(pcie_state.info.ecam_base, pcie_state.info.ecam_len, pcie_state.info.bus_start, pcie_state.info.bus_end);
        if (err == NO_ERROR) {
            // add some additional resources to the pci bus manager in case it needs to configure
            if (pcie_state.info.io_len > 0) {
                // we can only deal with a mapping of io base 0 to the mmio base
                DEBUG_ASSERT(pcie_state.info.io_base == 0);
                pci_bus_mgr_add_resource(PCI_RESOURCE_IO_RANGE, pcie_state.info.io_base, pcie_state.info.io_len);

                // TODO: set the mmio base somehow so pci knows what to do with it
            }
            if (pcie_state.info.mmio_len > 0) {
                pci_bus_mgr_add_resource(PCI_RESOURCE_MMIO_RANGE, pcie_state.info.mmio_base, pcie_state.info.mmio_len);
            }
            if (pcie_state.info.mmio64_len > 0) {
                pci_bus_mgr_add_resource(PCI_RESOURCE_MMIO64_RANGE, pcie_state.info.mmio64_base, pcie_state.info.mmio64_len);
            }

            // start the bus manager
            pci_bus_mgr_init();

            pci_bus_mgr_assign_resources();
        }
    }

    /* detect any virtio devices */
    uint virtio_irqs[NUM_VIRTIO_TRANSPORTS];
    for (int i = 0; i < NUM_VIRTIO_TRANSPORTS; i++) {
        virtio_irqs[i] = VIRTIO0_INT_BASE + i;
    }

    virtio_mmio_detect((void *)VIRTIO_BASE, NUM_VIRTIO_TRANSPORTS, virtio_irqs, 0x200);

#if WITH_LIB_MINIP
    if (virtio_net_found() > 0) {
        uint8_t mac_addr[6];

        virtio_net_get_mac_addr(mac_addr);

        TRACEF("found virtio networking interface\n");

        /* start minip */
        minip_set_eth(virtio_net_send_minip_pkt, NULL, mac_addr);

        __UNUSED uint32_t ip_addr = IPV4(192, 168, 0, 99);
        __UNUSED uint32_t ip_mask = IPV4(255, 255, 255, 0);
        __UNUSED uint32_t ip_gateway = IPV4_NONE;

        virtio_net_start();

        //minip_start_static(ip_addr, ip_mask, ip_gateway);
        minip_start_dhcp();
    }
#endif
}

status_t platform_pci_int_to_vector(unsigned int pci_int, unsigned int *vector) {
    // only 4 legacy vectors supported, within PCIE_INT_BASE and PCIE_INT_BASE + 3
    if (pci_int >= 4) {
        return ERR_OUT_OF_RANGE;
    }
    *vector = pci_int + PCIE_INT_BASE;
    return NO_ERROR;
}

status_t platform_allocate_interrupts(size_t count, uint align_log2, bool msi, unsigned int *vector) {
    TRACEF("count %zu align %u msi %d\n", count, align_log2, msi);

    // TODO: handle nonzero alignment, count > 0, and add locking

    // list of allocated msi interrupts
    static uint64_t msi_bitmap = 0;

    // cannot handle allocating for anything but MSI interrupts
    if (!msi) {
        return ERR_NOT_SUPPORTED;
    }

    // cannot deal with alignment yet
    DEBUG_ASSERT(align_log2 == 0);
    DEBUG_ASSERT(count == 1);

    // make a copy of the bitmap
    int allocated = -1;
    for (size_t i = 0; i < sizeof(msi_bitmap) * 8; i++) {
        if ((msi_bitmap & (1UL << i)) == 0) {
            msi_bitmap |= (1UL << i);
            allocated = i;
            break;
        }
    }
    if (allocated < 0) {
        return ERR_NOT_FOUND;
    }

    allocated += MSI_INT_BASE;

    TRACEF("allocated msi at %u\n", allocated);
    *vector = allocated;
    return NO_ERROR;
}

status_t platform_compute_msi_values(unsigned int vector, unsigned int cpu, bool edge,
                                     uint64_t *msi_address_out, uint16_t *msi_data_out) {

    // only handle edge triggered at the moment
    DEBUG_ASSERT(edge);
    // only handle cpu 0
    DEBUG_ASSERT(cpu == 0);

    // TODO: call through to the appropriate gic driver to deal with GICv2 vs v3

    *msi_data_out = vector;
    *msi_address_out = 0x08020040; // address of the GICv2 MSI port

    return NO_ERROR;
}

void platform_halt(platform_halt_action suggested_action, platform_halt_reason reason) {
    switch (suggested_action) {
        case HALT_ACTION_SHUTDOWN:
        case HALT_ACTION_HALT:
            psci_system_off();
            break;
        case HALT_ACTION_REBOOT:
            psci_system_reset();
            break;
    }
    dprintf(ALWAYS, "HALT: spinning forever... (reason = %d)\n", reason);
    arch_disable_ints();
    for (;;);
}
