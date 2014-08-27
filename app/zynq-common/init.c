/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdio.h>
#include <stdlib.h>
#include <lk/init.h>
#include <dev/spiflash.h>
#include <lib/ptable.h>
#include <lib/sysparam.h>
#include <debug.h>

#include <platform/gem.h>
#include <lib/minip.h>

static void zybo_common_target_init(uint level)
{
    /* zybo has a spiflash on qspi */
    spiflash_detect();

    bdev_t *spi = bio_open("spi0");

    if (spi) {
        /* find or create a partition table at the start of flash */
        if (ptable_scan(spi, 0) < 0) {
            ptable_create_default(spi, 0);
        }

        struct ptable_entry entry = { 0 };

        if (ptable_find("sysparam", &entry) < 0) {
            /* didn't find sysparam partition, create it */
            ptable_add("sysparam", 0x1000, 0x1000, 0);
            ptable_find("sysparam", &entry);
        }

        /* create bootloader partition if it does not exist */
        ptable_add("bootloader", 0x20000, 0x40000, 0);

        printf("flash partition table:\n");
        ptable_dump();

        if (entry.length > 0) {
            sysparam_scan(spi, entry.offset, entry.length);

#if SYSPARAM_ALLOW_WRITE
            /* for testing purposes, put at least one sysparam value in */
            if (sysparam_add("dummy", "value", sizeof("value")) >= 0) {
                sysparam_write();
            }
#endif

            sysparam_dump(true);
        }
    }

    /* pull some network stack related params out of the sysparam block */
    uint8_t mac_addr[6];
    uint32_t ip_addr = IPV4_NONE;
    uint32_t ip_mask = IPV4_NONE;
    uint32_t ip_gateway = IPV4_NONE;

    if (sysparam_read("net0.mac_addr", mac_addr, sizeof(mac_addr)) < (ssize_t)sizeof(mac_addr)) {
        /* couldn't find eth address, make up a random one */
        for (size_t i = 0; i < sizeof(mac_addr); i++) {
            mac_addr[i] = rand() & 0xff;
        }

        /* unicast and locally administered */
        mac_addr[0] &= ~(1<<0);
        mac_addr[0] |= (1<<1);
    }

    uint8_t use_dhcp = 0;
    sysparam_read("net0.use_dhcp", &use_dhcp, sizeof(use_dhcp));
    sysparam_read("net0.ip_addr", &ip_addr, sizeof(ip_addr));
    sysparam_read("net0.ip_mask", &ip_mask, sizeof(ip_mask));
    sysparam_read("net0.ip_gateway", &ip_gateway, sizeof(ip_gateway));

    minip_set_macaddr(mac_addr);
    gem_set_macaddr(mac_addr);

    if (!use_dhcp && ip_addr != IPV4_NONE) {
        minip_init(gem_send_raw_pkt, NULL, ip_addr, ip_mask, ip_gateway);
    } else {
        /* Configure IP stack and hook to the driver */
        minip_init_dhcp(gem_send_raw_pkt, NULL);
    }
    gem_set_callback(minip_rx_driver_callback);
}

/* init after target_init() */
LK_INIT_HOOK(app_zybo_common, &zybo_common_target_init, LK_INIT_LEVEL_TARGET);

