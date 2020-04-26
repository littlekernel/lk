/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/fdtwalk.h>

#include <libfdt.h>
#include <stdio.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <sys/types.h>

#define LOCAL_TRACE 0

status_t fdt_walk(const void *fdt, const struct fdt_walk_callbacks *cb) {
    int err = fdt_check_header(fdt);
    if (err < 0) {
        return ERR_NOT_FOUND;
    }

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
        if (strncmp(name, "memory@", 7) == 0 && depth == 1) {
            int lenp;
            const void *prop_ptr = fdt_getprop(fdt, offset, "reg", &lenp);
            if (prop_ptr && lenp == 0x10) {
                /* we're looking at a memory descriptor */
                // TODO: handle different sized descriptors
                uint64_t base = fdt64_to_cpu(*(const uint64_t *)prop_ptr);
                uint64_t len = fdt64_to_cpu(*((const uint64_t *)prop_ptr + 1));

                if (cb->mem) {
                    LTRACEF("calling mem callback with base %#llx len %#llx\n", base, len);
                    cb->mem(base, len, cb->memcookie);
                }
            }
        }

        /* look for a cpu leaf and count the number of cpus */
        if (strncmp(name, "cpu@", 4) == 0 && depth == 2) {
            int lenp;
            const void *prop_ptr = fdt_getprop(fdt, offset, "reg", &lenp);
            if (prop_ptr && lenp == 0x4) {
                // TODO: handle different sized descriptors
                uint32_t id = fdt32_to_cpu(*(const uint32_t *)prop_ptr);

                if (cb->cpu) {
                    LTRACEF("calling cpu callback with id %#x\n", id);
                    cb->cpu(id, cb->cpucookie);
                }
            }
        }
    }

    return NO_ERROR;
}

