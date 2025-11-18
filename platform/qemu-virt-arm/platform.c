/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <platform/qemu-virt.h>

#include <arch.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <dev/bus/pci.h>
#include <dev/interrupt/arm_gic.h>
#include <dev/power/psci.h>
#include <dev/timer/arm_generic.h>
#include <dev/uart/pl011.h>
#include <dev/virtio.h>
#include <dev/virtio/net.h>
#include <lib/fdtwalk.h>
#include <lk/init.h>
#include <kernel/vm.h>
#include <kernel/spinlock.h>
#include <platform.h>
#include <platform/gic.h>
#include <platform/interrupts.h>

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

const void *fdt = (void *)KERNEL_BASE;

const void *get_fdt(void) {
    return fdt;
}

void platform_early_init(void) {
    const struct pl011_config uart_config = {
        .base = UART_BASE,
        .irq = UART0_INT,
        .flag = PL011_FLAG_DEBUG_UART,
    };

    pl011_init_early(0, &uart_config);

    /* initialize the interrupt controller */
    arm_gic_init();

    arm_generic_timer_init(ARM_GENERIC_TIMER_VIRTUAL_INT, 0);

    if (LOCAL_TRACE) {
        LTRACEF("dumping FDT at %p\n", fdt);
        fdt_walk_dump(fdt);
    }

    // detect physical memory layout from the device tree
    fdtwalk_setup_memory(fdt, MEMORY_BASE_PHYS, MEMORY_BASE_PHYS, DEFAULT_MEMORY_SIZE);
}

void platform_init(void) {
    pl011_init(0);

    // start secondary cores
    fdtwalk_setup_cpus_arm(fdt);

    /* configure and start pci from device tree */
    status_t err = fdtwalk_setup_pci(fdt);
    if (err >= NO_ERROR) {
        // start the bus manager
        pci_bus_mgr_init();

        // assign resources to all devices in case they need it
        pci_bus_mgr_assign_resources();
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
    // Use the default halt implementation using psci as the reset and shutdown implementation.
    platform_halt_default(suggested_action, reason, &psci_system_reset, &psci_system_off);
}
