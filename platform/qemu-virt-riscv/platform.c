/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/reg.h>
#include <kernel/thread.h>
#include <kernel/novm.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/timer.h>
#include <platform/virt.h>
#include <sys/types.h>
#include <libfdt.h>

#include "platform_p.h"

extern ulong lk_boot_args[4];

void platform_early_init(void) {
    plic_early_init();

    /* look for a flattened device tree in the second arg passed to us */
    const void *fdt = (void *)lk_boot_args[1];
    int err = fdt_check_header(fdt);
    if (err >= 0) {
        /* walk the nodes, looking for 'memory@*' */
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

            /* look for the 'memory@*' property */
            if (strncmp(name, "memory@", 7) == 0) {
                int lenp;
                const void *prop_ptr = fdt_getprop(fdt, offset, "reg", &lenp);
                if (prop_ptr && lenp == 0x10) {
                    /* we're looking at a memory descriptor */
                    uint64_t base = fdt64_to_cpu(*(uint64_t *)prop_ptr);
                    uint64_t len = fdt64_to_cpu(*((const uint64_t *)prop_ptr + 1));

                    /* add another novm arena */
                    printf("FDT: found memory arena, base %#llx size %#llx\n", base, len);
                    novm_add_arena("fdt", base, len);
                    break; // stop searching after the first one
                }
            }
        }
    }
}

void platform_init(void) {
    plic_init();
    uart_init();
}


