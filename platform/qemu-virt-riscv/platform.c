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
#if WITH_LIB_CONSOLE
#include <lib/console.h>
#endif

#include "platform_p.h"

#define LOCAL_TRACE 0

#if WITH_KERNEL_VM
#define DEFAULT_MEMORY_SIZE (MEMSIZE) /* try to fetch from the emulator via the fdt */

static pmm_arena_t arena = {
    .name = "ram",
    .base = MEMORY_BASE_PHYS,
    .size = DEFAULT_MEMORY_SIZE,
    .flags = PMM_ARENA_FLAG_KMAP,
};
#endif

static volatile uint32_t *power_reset_reg;

// callbacks to the fdt_walk routine
static void memcallback(uint64_t base, uint64_t len, void *cookie) {
    bool *found_mem = (bool *)cookie;

    LTRACEF("base %#llx len %#llx cookie %p\n", base, len, cookie);

    /* add another vm arena */
    if (!*found_mem) {
        printf("FDT: found memory arena, base %#llx size %#llx\n", base, len);
#if WITH_KERNEL_VM
        arena.base = base;
        arena.size = len;
        pmm_add_arena(&arena);
#else
        novm_add_arena("fdt", base, len);
#endif
        *found_mem = true; // stop searching after the first one
    }
}

struct reserved_memory_regions {
    size_t count;

    struct {
        uint64_t base;
        uint64_t len;
    } regions[16];
};

static void reserved_memory_callback(uint64_t base, uint64_t len, void *cookie) {
    struct reserved_memory_regions *mem = cookie;

    LTRACEF("base %#llx len %#llx\n", base, len);

    if (mem->count < countof(mem->regions)) {
        mem->regions[mem->count].base = base;
        mem->regions[mem->count].len = len;
        mem->count++;
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
    plic_early_init(PLIC_BASE_VIRT, NUM_IRQS, false);

    LTRACEF("starting FDT scan\n");

    /* look for a flattened device tree in the second arg passed to us */
    bool found_mem = false;
    int cpu_count = 0;
    struct reserved_memory_regions reserved = {0};

    const void *fdt = (void *)lk_boot_args[1];
#if WITH_KERNEL_VM
    fdt = (const void *)((uintptr_t)fdt + PERIPHERAL_BASE_VIRT);
#endif

    struct fdt_walk_callbacks cb = {
        .mem = memcallback,
        .memcookie = &found_mem,
        .reserved_memory = reserved_memory_callback,
        .reserved_memory_cookie = &reserved,
        .cpu = cpucallback,
        .cpucookie = &cpu_count,
        .pcie = pciecallback,
        .pciecookie = &pcie_state,
    };

    status_t err = fdt_walk(fdt, &cb);
    LTRACEF("fdt_walk returns %d\n", err);

    if (err != 0) {
        printf("FDT: error finding FDT at %p, using default memory & cpu count\n", fdt);
    }

    /* add a default memory region if we didn't find it in the FDT */
    if (!found_mem) {
#if WITH_KERNEL_VM
        pmm_add_arena(&arena);
#else
        novm_add_arena("default", MEMBASE, MEMSIZE);
#endif
    }

#if WITH_KERNEL_VM
    /* reserve memory described by the FDT */
    for (size_t i = 0; i < reserved.count; i++) {
        printf("FDT: reserving memory range [%#llx ... %#llx]\n",
                reserved.regions[i].base, reserved.regions[i].base + reserved.regions[i].len - 1);
        struct list_node list = LIST_INITIAL_VALUE(list);
        pmm_alloc_range(reserved.regions[i].base, reserved.regions[i].len / PAGE_SIZE, &list);
    }
#endif

    if (cpu_count > 0) {
        printf("FDT: found %d cpu%c\n", cpu_count, cpu_count == 1 ? ' ' : 's');
        riscv_set_secondary_count(cpu_count - 1);
    }

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

    /* detect pci */
    if (pcie_state.info.ecam_len > 0) {
        printf("PCIE: initializing pcie with ecam at %#" PRIx64 " found in FDT\n", pcie_state.info.ecam_base);
        status_t err = pci_init_ecam(pcie_state.info.ecam_base, pcie_state.info.ecam_len, pcie_state.info.bus_start, pcie_state.info.bus_end);
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

            pci_bus_mgr_assign_resources();            // add some additional resources to the pci bus manager in case it needs to configure
        };
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

void platform_halt(platform_halt_action suggested_action,
                          platform_halt_reason reason) {
    switch (suggested_action) {
        case HALT_ACTION_SHUTDOWN:
            dprintf(ALWAYS, "Shutting down... (reason = %d)\n", reason);
#if RISCV_S_MODE
            // try to use SBI as a cleaner way to stop
            sbi_system_reset(SBI_RESET_TYPE_SHUTDOWN, SBI_RESET_REASON_NONE);
#endif
            *power_reset_reg = 0x5555;
            break;
        case HALT_ACTION_REBOOT:
            dprintf(ALWAYS, "Rebooting... (reason = %d)\n", reason);
#if RISCV_S_MODE
            sbi_system_reset(SBI_RESET_TYPE_COLD_REBOOT, SBI_RESET_REASON_NONE);
#endif
            *power_reset_reg = 0x7777;
            break;
        case HALT_ACTION_HALT:
#if ENABLE_PANIC_SHELL
            if (reason == HALT_REASON_SW_PANIC) {
                dprintf(ALWAYS, "CRASH: starting debug shell... (reason = %d)\n", reason);
                arch_disable_ints();
                panic_shell_start();
            }
#endif  // ENABLE_PANIC_SHELL
            dprintf(ALWAYS, "HALT: spinning forever... (reason = %d)\n", reason);
            break;
    }

    arch_disable_ints();
    for (;;)
        arch_idle();
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
