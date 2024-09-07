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

#include "bootinfo.h"
#include "platform_p.h"

#define LOCAL_TRACE 0

void platform_early_init(void) {
    goldfish_tty_early_init();
    pic_early_init();
    goldfish_rtc_early_init();

    // Dump the bootinfo structure
    if (LK_DEBUGLEVEL >= INFO) {
        dump_all_bootinfo_records();
    }

    // look for tag 0x5, which describes the memory layout of the system
    uint16_t size;
    const void *ptr = bootinfo_find_record(BOOTINFO_TAG_MEMCHUNK, &size);
    if (!ptr) {
        panic("68K VIRT: unable to find MEMCHUNK BOOTINFO record\n");
    }
    if (size < 8) {
        panic("68K VIRT: MEMCHUNK BOOTINFO record too small\n");
    }
    LTRACEF("MEMCHUNK ptr %p, size %hu\n", ptr, size);

    uint32_t membase = *(const uint32_t *)ptr;
    uint32_t memsize = *(const uint32_t *)((uintptr_t)ptr + 4);

    dprintf(INFO, "VIRT: memory base %#x size %#x\n", membase, memsize);
    novm_add_arena("mem", membase, memsize);

    // TODO: read the rest of the device bootinfo records and dynamically locate devices
}

void platform_init(void) {
    pic_init();
    goldfish_tty_init();
    goldfish_rtc_init();

    /* detect any virtio devices */
    uint virtio_irqs[NUM_VIRT_VIRTIO];
    for (int i = 0; i < NUM_VIRT_VIRTIO; i++) {
        virtio_irqs[i] = VIRT_VIRTIO_IRQ_BASE + i;
    }

    virtio_mmio_detect((void *)VIRT_VIRTIO_MMIO_BASE, NUM_VIRT_VIRTIO, virtio_irqs, 0x200);

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
