/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <platform/qemu-virt.h>

#include <arch.h>
#include <assert.h>
#include <inttypes.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/trace.h>
#include <dev/bus/pci.h>
#include <dev/interrupt/arm_gic.h>
#include <dev/power/psci.h>
#include <dev/timer/arm_generic.h>
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
#if WITH_DEV_UART_PL011
#include <dev/uart/pl011.h>
#endif
#if WITH_RUST_DEV_PL011
#include <rust/dev-pl011.h>
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

    arm_generic_timer_init(ARM_GENERIC_TIMER_VIRTUAL_INT, 0);

    if (LOCAL_TRACE) {
        LTRACEF("dumping FDT at %p\n", fdt);
        fdt_walk_dump(fdt);
    }

    // detect physical memory layout from the device tree
    fdtwalk_setup_memory(fdt, MEMORY_BASE_PHYS, MEMORY_BASE_PHYS, DEFAULT_MEMORY_SIZE);

    // initialize cmdline from device tree
    fdtwalk_setup_cmdline(fdt);
}

// MSI_SETSPI_NS in the GICv2m frame, or 0 when the machine has none: QEMU builds one for a
// GICv2, while a GICv3 normally gets an ITS instead, which LK does not drive.
static uint64_t msi_doorbell;

static void find_msi_doorbell(void) {
    struct fdt_walk_gic_info gic_info = {};
    size_t count = 1;
    if (fdt_walk_find_gic_info(fdt, &gic_info, &count) != NO_ERROR || count == 0) {
        return;
    }
    if (gic_info.gic_version == 2 && gic_info.v2.v2m_count > 0) {
        msi_doorbell = gic_info.v2.v2m_frame[0].base + 0x40;
        dprintf(INFO, "PLATFORM: MSI through the GICv2m frame at %#" PRIx64 "\n",
                gic_info.v2.v2m_frame[0].base);
    } else {
        dprintf(INFO, "PLATFORM: no GICv2m frame, MSI unavailable\n");
    }
}

void platform_postvm_init(uint level) {
    // Initialize the interrupt controller from the device tree.
    // Do it at this run level so the gic can map its registers.
    if (fdtwalk_setup_gic(fdt) != NO_ERROR) {
        panic("failed to initialize GIC from FDT\n");
    }

    find_msi_doorbell();
}

LK_INIT_HOOK(platform_postvm_init, platform_postvm_init, LK_INIT_LEVEL_VM);

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

        // scan for an initialize any virtio devices
        virtio_pci_init();
    }

    /* detect any virtio devices */
    uint virtio_irqs[NUM_VIRTIO_TRANSPORTS];
    for (int i = 0; i < NUM_VIRTIO_TRANSPORTS; i++) {
        virtio_irqs[i] = VIRTIO0_INT_BASE + i;
    }

    virtio_mmio_detect((void *)VIRTIO_BASE, NUM_VIRTIO_TRANSPORTS, virtio_irqs, 0x200);
}

#if WITH_DEV_BUS_PCI
status_t platform_pci_int_line_to_vector(unsigned int pci_int_line, pci_location_t loc,
        unsigned int *vector) {
    (void)pci_int_line;
    (void)loc;
    (void)vector;

    // QEMU virt arm uses INTx pin swizzling for legacy routing.
    return ERR_NOT_SUPPORTED;
}

status_t platform_pci_int_pin_to_vector(unsigned int pci_int_pin, pci_location_t loc,
    unsigned int *vector) {

    // QEMU arm virt machine uses standard PCI swizzle on 4 legacy IRQs:
    // irq = first_irq + ((pin - 1 + slot) % 4), where pin is 1..4.
    // first_irq here is PCIE_INT_BASE.
    if (pci_int_pin < 1 || pci_int_pin > 4) {
        return ERR_OUT_OF_RANGE;
    }

    *vector = PCIE_INT_BASE + ((pci_int_pin - 1 + loc.dev) % 4);
    return NO_ERROR;
}
#endif

status_t platform_allocate_interrupts(size_t count, uint align_log2, bool msi, unsigned int *vector) {
    LTRACEF("count %zu align %u msi %d\n", count, align_log2, msi);

    // TODO: handle nonzero alignment and count > 1

    // allocated msi interrupts, bit n is MSI_INT_BASE + n
    static uint64_t msi_bitmap = 0;
    static spin_lock_t msi_lock = SPIN_LOCK_INITIAL_VALUE;
    STATIC_ASSERT(MSI_INT_COUNT <= sizeof(msi_bitmap) * 8);
    STATIC_ASSERT(MSI_INT_BASE + MSI_INT_COUNT <= MAX_INT);

    // cannot handle allocating for anything but MSI interrupts
    if (!msi || !msi_doorbell) {
        return ERR_NOT_SUPPORTED;
    }

    // single, unaligned vectors only for now
    if (count != 1 || align_log2 != 0) {
        return ERR_NOT_SUPPORTED;
    }

    int allocated = -1;
    arch_interrupt_saved_state_t state = spin_lock_irqsave(&msi_lock);
    for (uint i = 0; i < MSI_INT_COUNT; i++) {
        if ((msi_bitmap & (1ULL << i)) == 0) {
            msi_bitmap |= (1ULL << i);
            allocated = i;
            break;
        }
    }
    spin_unlock_irqrestore(&msi_lock, state);
    if (allocated < 0) {
        return ERR_NOT_FOUND;
    }

    allocated += MSI_INT_BASE;

    LTRACEF("allocated msi at %u\n", allocated);
    *vector = allocated;
    return NO_ERROR;
}

status_t platform_compute_msi_values(unsigned int vector, unsigned int cpu, bool edge,
                                     uint64_t *msi_address_out, uint16_t *msi_data_out) {

    // only handle edge triggered at the moment
    DEBUG_ASSERT(edge);
    // only handle cpu 0
    DEBUG_ASSERT(cpu == 0);

    if (!msi_doorbell) {
        return ERR_NOT_SUPPORTED;
    }

    // GICv2m: writing the SPI number to MSI_SETSPI_NS raises it
    *msi_data_out = vector;
    *msi_address_out = msi_doorbell;

    return NO_ERROR;
}

void platform_halt(platform_halt_action suggested_action, platform_halt_reason reason) {
    // Use the default halt implementation using psci as the reset and shutdown implementation.
    platform_halt_default(suggested_action, reason, &psci_system_reset, &psci_system_off);
}
