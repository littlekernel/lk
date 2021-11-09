/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/thread.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/virt.h>
#include <sys/types.h>
#include <dev/virtio.h>
#include <dev/virtio/net.h>
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

extern ulong lk_boot_args[4];

#if WITH_KERNEL_VM
#define DEFAULT_MEMORY_SIZE (MEMSIZE) /* try to fetch from the emulator via the fdt */

static pmm_arena_t arena = {
    .name = "ram",
    .base = MEMORY_BASE_PHYS,
    .size = DEFAULT_MEMORY_SIZE,
    .flags = PMM_ARENA_FLAG_KMAP,
};
#endif

void platform_early_init(void) {
    goldfish_tty_early_init();
    pic_early_init();
    goldfish_rtc_early_init();
#if 0
    plic_early_init();

    LTRACEF("starting FDT scan\n");

    /* look for a flattened device tree in the second arg passed to us */
    bool found_mem = false;
    int cpu_count = 0;
    const void *fdt = (void *)lk_boot_args[1];
#if WITH_KERNEL_VM
    fdt = (const void *)((uintptr_t)fdt + PERIPHERAL_BASE_VIRT);
#endif

    struct fdt_walk_callbacks cb = {
        .mem = memcallback,
        .memcookie = &found_mem,
        .cpu = cpucallback,
        .cpucookie = &cpu_count,
    };

    status_t err = fdt_walk(fdt, &cb);
    LTRACEF("fdt_walk returns %d\n", err);

    if (err != 0) {
        printf("FDT: error finding FDT at %p, using default memory & cpu count\n", fdt);
    }

    if (!found_mem) {
#if WITH_KERNEL_VM
        pmm_add_arena(&arena);
#else
        novm_add_arena("default", MEMBASE, MEMSIZE);
#endif
    }

    if (cpu_count > 0) {
        printf("FDT: found %d cpus\n", cpu_count);
        riscv_set_secondary_count(cpu_count - 1);
    }

#if WITH_KERNEL_VM
    /* reserve the first 256K of ram which is marked protected by the PMP in firmware */
    struct list_node list = LIST_INITIAL_VALUE(list);
    pmm_alloc_range(MEMBASE, 0x40000 / PAGE_SIZE, &list);
#endif

    LTRACEF("done scanning FDT\n");

    /* save a copy of the pointer to the poweroff/reset register */
    /* TODO: read it from the FDT */
#if WITH_KERNEL_VM
    power_reset_reg = paddr_to_kvaddr(0x100000);
#else
    power_reset_reg = (void *)0x100000;
#endif
#endif
}

void platform_init(void) {
    pic_init();
    goldfish_tty_init();
    goldfish_rtc_init();
#if 0
    plic_init();
    uart_init();

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
        minip_set_macaddr(mac_addr);

        __UNUSED uint32_t ip_addr = IPV4(192, 168, 0, 99);
        __UNUSED uint32_t ip_mask = IPV4(255, 255, 255, 0);
        __UNUSED uint32_t ip_gateway = IPV4_NONE;

        //minip_init(virtio_net_send_minip_pkt, NULL, ip_addr, ip_mask, ip_gateway);
        minip_init_dhcp(virtio_net_send_minip_pkt, NULL);

        virtio_net_start();
    }
#endif
#endif
}

#if 0
void platform_halt(platform_halt_action suggested_action,
                          platform_halt_reason reason) {
    switch (suggested_action) {
        case HALT_ACTION_SHUTDOWN:
            dprintf(ALWAYS, "Shutting down... (reason = %d)\n", reason);
            *power_reset_reg = 0x5555;
            break;
        case HALT_ACTION_REBOOT:
            dprintf(ALWAYS, "Rebooting... (reason = %d)\n", reason);
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
#endif
