/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <inttypes.h>
#include <lk/err.h>
#include <lk/main.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <arch/riscv.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/virt.h>
#include <sys/types.h>
#include <lib/fdtwalk.h>
#include <dev/bus/pci.h>
#include <dev/virtio.h>
#include <dev/virtio/net.h>
#include <dev/interrupt/riscv_plic.h>
#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif
#if WITH_KERNEL_VM
#include <kernel/vm.h>
#else
#include <kernel/novm.h>
#endif

#include "platform_p.h"

#define LOCAL_TRACE 0

static const void *fdt;
static volatile uint32_t *power_reset_reg;

void platform_early_init(void) {
    plic_early_init(PLIC_BASE_VIRT, NUM_IRQS, false);

    LTRACEF("starting FDT scan\n");

    /* look for a flattened device tree in the second arg passed to us */
    fdt = (void *)lk_boot_args[1];
#if WITH_KERNEL_VM
    fdt = (const void *)((uintptr_t)fdt + PERIPHERAL_BASE_VIRT);
#endif

    if (LOCAL_TRACE) {
        LTRACEF("dumping FDT at %p\n", fdt);
        fdt_walk_dump(fdt);
    }

    // detect physical memory layout from the device tree
    fdtwalk_setup_memory(fdt, lk_boot_args[1], MEMORY_BASE_PHYS, MEMSIZE);

    // detect secondary cores to start
    fdtwalk_setup_cpus_riscv(fdt);

    LTRACEF("done scanning FDT\n");

    /* save a copy of the pointer to the poweroff/reset register */
    /* TODO: read it from the FDT */
#if WITH_KERNEL_VM
    power_reset_reg = paddr_to_kvaddr(0x100000);
#else
    power_reset_reg = (void *)0x100000;
#endif
}

void platform_init(void) {
    plic_init();
    uart_init();

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
        virtio_irqs[i] = IRQ_VIRTIO_BASE + i;
    }

    virtio_mmio_detect((void *)VIRTIO_BASE_VIRT, NUM_VIRTIO_TRANSPORTS, virtio_irqs, VIRTIO_STRIDE);

#if WITH_LIB_MINIP
    if (virtio_net_found() > 0) {
        uint8_t mac_addr[6];

        virtio_net_get_mac_addr(mac_addr);

        TRACEF("found virtio networking interface\n");

        /* start minip */
        minip_set_eth(virtio_net_send_minip_pkt, NULL, mac_addr);

        virtio_net_start();

#if 0
        __UNUSED uint32_t ip_addr = IPV4(192, 168, 0, 99);
        __UNUSED uint32_t ip_mask = IPV4(255, 255, 255, 0);
        __UNUSED uint32_t ip_gateway = IPV4_NONE;

        minip_start_static(ip_addr, ip_mask, ip_gateway);
#else
        minip_start_dhcp();
#endif
    }
#endif
}

static void reboot_(void) {
#if RISCV_S_MODE
    sbi_system_reset(SBI_RESET_TYPE_COLD_REBOOT, SBI_RESET_REASON_NONE);
#endif
    *power_reset_reg = 0x7777;
}

static void shutdown_(void) {
#if RISCV_S_MODE
    // try to use sbi as a cleaner way to stop
    sbi_system_reset(SBI_RESET_TYPE_SHUTDOWN, SBI_RESET_REASON_NONE);
#endif
    *power_reset_reg = 0x5555;
}

void platform_halt(platform_halt_action suggested_action, platform_halt_reason reason) {
    // Use the default halt implementation using sbi as the reset and shutdown implementation.
    platform_halt_default(suggested_action, reason, &reboot_, &shutdown_);
}

status_t platform_pci_int_to_vector(unsigned int pci_int, unsigned int *vector) {
    // at the moment there's no translation between PCI IRQs and native irqs
    *vector = pci_int;
    return NO_ERROR;
}

status_t platform_allocate_interrupts(size_t count, uint align_log2, bool msi, unsigned int *vector) {
    return ERR_NOT_SUPPORTED;
}

status_t platform_compute_msi_values(unsigned int vector, unsigned int cpu, bool edge,
        uint64_t *msi_address_out, uint16_t *msi_data_out) {
    return ERR_NOT_SUPPORTED;
}
