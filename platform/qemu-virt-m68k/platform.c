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

extern uint8_t __bss_end;

// parse bootinfo
struct bootinfo_item {
    uint16_t tag;
    uint16_t size;
    uint32_t data[0];
};

// look for tags that qemu left at the end of the kernel that hold various
// pieces of system configuration info.
static void *bootinfo_find_record(uint16_t id, uint16_t *size_out) {
    uint8_t *ptr = &__bss_end;

    *size_out = 0;
    for (;;) {
        struct bootinfo_item *item = (struct bootinfo_item *)ptr;

        LTRACEF_LEVEL(2, "item %p: tag %hx, size %hu\n", item, item->tag, item->size);

        if (item->tag == id) {
            *size_out = item->size - 4;
            return item->data;
        } else if (item->tag == 0) { // end token
            return NULL;
        }

        // move to the next field
        ptr += item->size;
    }
}

void platform_early_init(void) {
    goldfish_tty_early_init();
    pic_early_init();
    goldfish_rtc_early_init();

    // look for tag 0x5, which describes the memory layout of the system
    uint16_t size;
    void *ptr = bootinfo_find_record(0x5, &size);
    if (!ptr) {
        panic("68K VIRT: unable to find MEMCHUNK BOOTINFO record\n");
    }
    LTRACEF("MEMCHUNK ptr %p, size %hu\n", ptr, size);

    uint32_t membase = *(uint32_t *)ptr;
    uint32_t memsize = *(uint32_t *)((uintptr_t)ptr + 4);

    dprintf(INFO, "VIRT: memory base %#x size %#x\n", membase, memsize);
    novm_add_arena("mem", membase, memsize);
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
