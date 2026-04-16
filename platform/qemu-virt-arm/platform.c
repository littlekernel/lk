/*
 * Copyright (c) 2012-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <platform/qemu-virt.h>

#include <arch.h>
#include <dev/bus/pci.h>
#include <dev/interrupt/arm_gic.h>
#include <dev/power/psci.h>
#include <dev/timer/arm_generic.h>
#include <dev/virtio.h>
#include <dev/virtio/net.h>
#include <kernel/spinlock.h>
#include <kernel/vm.h>
#include <lib/fdtwalk.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/trace.h>
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
        .name = "memory"},

    /* 1GB of peripherals */
    {
        .phys = PERIPHERAL_BASE_PHYS,
        .virt = PERIPHERAL_BASE_VIRT,
        .size = PERIPHERAL_BASE_SIZE,
        .flags = MMU_INITIAL_MAPPING_FLAG_DEVICE,
        .name = "peripherals"},

    /* null entry to terminate the list */
    {0}};

const void *fdt = (void *)KERNEL_BASE;

static bool g_pci_msi_from_dt;
static bool g_pci_msi_uses_its;
static uint32_t g_pci_msi_parent_phandle;
static uint64_t g_pci_msi_address;

static void platform_configure_pci_msi_from_fdt(void) {
    g_pci_msi_from_dt = false;
    g_pci_msi_uses_its = false;
    g_pci_msi_parent_phandle = 0;
    g_pci_msi_address = 0;

    struct fdt_walk_pci_msi_route msi_route = {};
    status_t route_err = fdt_walk_pcie_lookup_msi(0, &msi_route);
    if (route_err != NO_ERROR) {
        dprintf(SPEW, "PCIE/MSI ARM: no DT msi-map route for requester-id 0 (%d)\n", route_err);
        return;
    }

    g_pci_msi_parent_phandle = msi_route.parent_phandle;

    struct fdt_walk_gic_info gic_info[4] = {};
    size_t gic_count = countof(gic_info);
    status_t gic_err = fdt_walk_find_gic_info(fdt, gic_info, &gic_count);
    if (gic_err != NO_ERROR) {
        dprintf(SPEW,
                "PCIE/MSI ARM: unable to parse GIC info for msi-map parent %#x (%d)\n",
                g_pci_msi_parent_phandle, gic_err);
        return;
    }

    for (size_t i = 0; i < gic_count; ++i) {
        if (gic_info[i].gic_version == 2) {
            for (size_t j = 0; j < gic_info[i].v2.v2m_count; ++j) {
                if (gic_info[i].v2.v2m_frame[j].phandle == g_pci_msi_parent_phandle) {
                    // V2M MSI frames use TRANSLATOR at offset 0x40.
                    g_pci_msi_address = gic_info[i].v2.v2m_frame[j].base + 0x40;
                    g_pci_msi_from_dt = true;
                    dprintf(SPEW,
                            "PCIE/MSI ARM: DT route parent %#x -> GICv2m frame[%zu] base %#llx"
                            " translator %#llx\n",
                            g_pci_msi_parent_phandle, j,
                            (unsigned long long)gic_info[i].v2.v2m_frame[j].base,
                            (unsigned long long)g_pci_msi_address);
                    return;
                }
            }
        } else if (gic_info[i].gic_version == 3) {
            for (size_t j = 0; j < gic_info[i].v3.its_count; ++j) {
                if (gic_info[i].v3.its[j].phandle == g_pci_msi_parent_phandle) {
                    g_pci_msi_uses_its = true;
                    dprintf(SPEW,
                            "PCIE/MSI ARM: DT route parent %#x is GICv3 ITS[%zu] base %#llx;"
                            " MSI programming via ITS is not yet implemented\n",
                            g_pci_msi_parent_phandle, j,
                            (unsigned long long)gic_info[i].v3.its[j].base);
                    return;
                }
            }
        }
    }

    dprintf(SPEW,
            "PCIE/MSI ARM: DT msi-map parent %#x did not match known GIC MSI controllers\n",
            g_pci_msi_parent_phandle);
}

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
}

void platform_postvm_init(uint level) {
    // Initialize the interrupt controller from the device tree.
    // Do it at this run level so the gic can map its registers.
    if (fdtwalk_setup_gic(fdt) != NO_ERROR) {
        panic("failed to initialize GIC from FDT\n");
    }
}

LK_INIT_HOOK(platform_postvm_init, platform_postvm_init, LK_INIT_LEVEL_VM);

void platform_init(void) {
    pl011_init(0);

    // start secondary cores
    fdtwalk_setup_cpus_arm(fdt);

    /* configure and start pci from device tree */
    static struct fdt_walk_pcie_info pcie_info[1];
    size_t pcie_count = 1;
    status_t err = fdtwalk_setup_pci(fdt, pcie_info, &pcie_count);
    if (err >= NO_ERROR) {
        platform_configure_pci_msi_from_fdt();

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

        // minip_start_static(ip_addr, ip_mask, ip_gateway);
        minip_start_dhcp();
    }
#endif
}

status_t platform_pci_int_to_vector(unsigned int pci_int, unsigned int pci_bus,
                                    unsigned int pci_dev, unsigned int pci_func, unsigned int *vector) {
    struct fdt_walk_pci_int_route route = {};
    status_t route_err = fdt_walk_pcie_lookup_intx((uint8_t)pci_bus,
                                                   (uint8_t)pci_dev,
                                                   (uint8_t)pci_func,
                                                   (uint8_t)pci_int,
                                                   &route);
    if (route_err == NO_ERROR) {
        if (route.parent_interrupt_cells >= 2) {
            // GIC interrupts are encoded as <type, irq, flags>.
            // For SPIs (type 0), DT irq is relative to 32; for PPIs (type 1), relative to 16.
            uint32_t int_type = route.parent_interrupt[0];
            uint32_t hwirq = route.parent_interrupt[1];
            if (int_type == 0) {
                *vector = 32 + hwirq;
            } else if (int_type == 1) {
                *vector = 16 + hwirq;
            } else {
                dprintf(SPEW,
                        "PCIE/INTx ARM: unsupported GIC int type %u for bdf %u:%u.%u pin %u, falling back\n",
                        int_type, pci_bus, pci_dev, pci_func, pci_int);
                goto fallback_swizzle;
            }
            dprintf(SPEW,
                    "PCIE/INTx ARM: DT route bdf %u:%u.%u pin %u -> phandle %#x hwirq %u vector %u"
                    " (spec cells %u, type %u, flags %u)\n",
                    pci_bus, pci_dev, pci_func, pci_int,
                    route.parent_phandle, hwirq, *vector,
                    route.parent_interrupt_cells,
                    route.parent_interrupt[0],
                    route.parent_interrupt_cells >= 3 ? route.parent_interrupt[2] : 0);
            return NO_ERROR;
        }
        if (route.parent_interrupt_cells == 1) {
            *vector = route.parent_interrupt[0];
            dprintf(SPEW,
                    "PCIE/INTx ARM: DT route bdf %u:%u.%u pin %u -> phandle %#x irq %u"
                    " (single-cell parent spec)\n",
                    pci_bus, pci_dev, pci_func, pci_int,
                    route.parent_phandle, *vector);
            return NO_ERROR;
        }

        dprintf(SPEW,
                "PCIE/INTx ARM: DT route bdf %u:%u.%u pin %u had invalid parent cell count %u"
                ", falling back\n",
                pci_bus, pci_dev, pci_func, pci_int, route.parent_interrupt_cells);
    }

fallback_swizzle:

    // QEMU arm virt machine uses standard PCI swizzle on 4 legacy IRQs:
    // irq = first_irq + ((pin - 1 + slot) % 4), where pin is 1..4.
    // first_irq here is PCIE_INT_BASE.
    if (pci_int < 1 || pci_int > 4) {
        dprintf(SPEW,
                "PCIE/INTx ARM: invalid pin %u for bdf %u:%u.%u\n",
                pci_int, pci_bus, pci_dev, pci_func);
        return ERR_OUT_OF_RANGE;
    }

    *vector = PCIE_INT_BASE + ((pci_int - 1 + pci_dev) % 4);
    dprintf(SPEW,
            "PCIE/INTx ARM: fallback swizzle bdf %u:%u.%u pin %u -> irq %u\n",
            pci_bus, pci_dev, pci_func, pci_int, *vector);
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

    if (g_pci_msi_uses_its) {
        dprintf(SPEW,
                "PCIE/MSI ARM: parent %#x resolves to ITS; MSI message computation is not supported yet\n",
                g_pci_msi_parent_phandle);
        return ERR_NOT_SUPPORTED;
    }

    *msi_data_out = vector;
    if (g_pci_msi_from_dt) {
        *msi_address_out = g_pci_msi_address;
        dprintf(SPEW,
                "PCIE/MSI ARM: vector %u -> addr %#llx data %#x (from DT msi-map parent %#x)\n",
                vector, (unsigned long long)*msi_address_out, *msi_data_out,
                g_pci_msi_parent_phandle);
    } else {
        // Fallback for older DTs/configurations that do not expose a usable msi-map path.
        *msi_address_out = 0x08020040;
        dprintf(SPEW,
                "PCIE/MSI ARM: vector %u -> addr %#llx data %#x (fallback GICv2m default)\n",
                vector, (unsigned long long)*msi_address_out, *msi_data_out);
    }

    return NO_ERROR;
}

void platform_halt(platform_halt_action suggested_action, platform_halt_reason reason) {
    // Use the default halt implementation using psci as the reset and shutdown implementation.
    platform_halt_default(suggested_action, reason, &psci_system_reset, &psci_system_off);
}
