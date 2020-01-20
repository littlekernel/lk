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
#include <kernel/novm.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/virt.h>
#include <sys/types.h>
#include <libfdt.h>
#include <dev/virtio.h>
#include <dev/virtio/net.h>
#if WITH_LIB_MINIP
#include <lib/minip.h>
#endif

#include "platform_p.h"

#define LOCAL_TRACE 1

extern ulong lk_boot_args[4];

static int cpu_count = 0;

void platform_early_init(void) {
    plic_early_init();

    LTRACEF("starting FDT scan\n");

    /* look for a flattened device tree in the second arg passed to us */
    bool found_mem = false;
    const void *fdt = (void *)lk_boot_args[1];
    int err = fdt_check_header(fdt);
    if (err >= 0) {
        /* walk the nodes */
        int depth = 0;
        int offset = 0;
        for (;;) {
            offset = fdt_next_node(fdt, offset, &depth);
            if (offset < 0)
                break;

            /* get the name */
            const char *name = fdt_get_name(fdt, offset, NULL);
            if (!name)
                continue;

            LTRACEF_LEVEL(2, "name '%s', depth %d\n", name, depth);

            /* look for the 'memory@*' property */
            if (!found_mem && strncmp(name, "memory@", 7) == 0 && depth == 1) {
                int lenp;
                const void *prop_ptr = fdt_getprop(fdt, offset, "reg", &lenp);
                if (prop_ptr && lenp == 0x10) {
                    /* we're looking at a memory descriptor */
                    uint64_t base = fdt64_to_cpu(*(const uint64_t *)prop_ptr);
                    uint64_t len = fdt64_to_cpu(*((const uint64_t *)prop_ptr + 1));

                    /* add another novm arena */
                    printf("FDT: found memory arena, base %#llx size %#llx\n", base, len);
                    novm_add_arena("fdt", base, len);
                    found_mem = true; // stop searching after the first one
                }
            }

            /* look for a cpu leaf and count the number of cpus */
            if (strncmp(name, "cpu@", 4) == 0 && depth == 2) {
                int lenp;
                const void *prop_ptr = fdt_getprop(fdt, offset, "reg", &lenp);
                if (prop_ptr && lenp == 0x4) {
                    uint32_t id = fdt32_to_cpu(*(const uint32_t *)prop_ptr);
                    printf("FDT: found cpu id %u\n", id);
                    cpu_count++;
                }
            }
        }
    }

    if (cpu_count > 0) {
        riscv_set_secondary_count(cpu_count - 1);
    }

    LTRACEF("done scanning FDT\n");
}

void platform_init(void) {
    plic_init();
    uart_init();

    /* detect any virtio devices */
    uint virtio_irqs[NUM_VIRTIO_TRANSPORTS];
    for (int i = 0; i < NUM_VIRTIO_TRANSPORTS; i++) {
        virtio_irqs[i] = IRQ_VIRTIO_BASE + i;
    }

    virtio_mmio_detect((void *)VIRTIO_BASE, NUM_VIRTIO_TRANSPORTS, virtio_irqs, VIRTIO_STRIDE);

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
}

