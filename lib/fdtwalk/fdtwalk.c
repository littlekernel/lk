/*
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/fdtwalk.h>

#include <assert.h>
#include <libfdt.h>
#include <stdio.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <sys/types.h>

#define LOCAL_TRACE 0
#define MAX_DEPTH 16

/* read the #address-cells and #size-cells properties at the current node to
 * see if there are any overriding sizes at this level. It's okay to not
 * find the properties.
 */
static void read_address_size_cells(const void *fdt, int offset, int depth,
                                    uint32_t *address_cells, uint32_t *size_cells) {
    LTRACEF_LEVEL(3, "fdt %p, offset %d depth %d\n", fdt, offset, depth);

    DEBUG_ASSERT(depth >= 0 && depth < MAX_DEPTH);

    int len;
    const void *prop_ptr = fdt_getprop(fdt, offset, "#address-cells", &len);
    LTRACEF_LEVEL(3, "%p, len %d\n", prop_ptr, len);
    if (prop_ptr && len == 4) {
        address_cells[depth] = fdt32_to_cpu(*(const uint32_t *)prop_ptr);
    }

    prop_ptr = fdt_getprop(fdt, offset, "#size-cells", &len);
    LTRACEF_LEVEL(3, "%p, len %d\n", prop_ptr, len);
    if (prop_ptr && len == 4) {
        size_cells[depth] = fdt32_to_cpu(*(const uint32_t *)prop_ptr);
    }

    LTRACEF_LEVEL(3, "address-cells %u size-cells %u\n", address_cells[depth], size_cells[depth]);
}

status_t fdt_walk(const void *fdt, const struct fdt_walk_callbacks *cb) {
    int err = fdt_check_header(fdt);
    if (err != 0) {
        return ERR_NOT_FOUND;
    }

    /* walk the nodes */
    int depth = 0;
    int offset = 0;
    uint32_t address_cells[MAX_DEPTH];
    uint32_t size_cells[MAX_DEPTH];

    /* read the address/size cells properties at the root, if present */
    address_cells[0] = size_cells[0] = 1;
    read_address_size_cells(fdt, offset, 0, address_cells, size_cells);

    for (;;) {
        offset = fdt_next_node(fdt, offset, &depth);
        if (offset < 0 || depth < 0) {
            break;
        }

        LTRACEF_LEVEL(3, "fdt_next node offset %d, depth %d\n", offset, depth);

        if (depth >= MAX_DEPTH) {
            printf("FDTWALK: exceeded max depth %d\n", MAX_DEPTH);
            return ERR_NO_MEMORY;
        }

        /* copy the address/size cells from the parent depth and then see if we
         * have local properties to override it. */
        if (depth > 0) {
            address_cells[depth] = address_cells[depth - 1];
            size_cells[depth] = size_cells[depth - 1];
        }
        read_address_size_cells(fdt, offset, depth, address_cells, size_cells);

        /* get the name */
        const char *name = fdt_get_name(fdt, offset, NULL);
        if (!name)
            continue;

        LTRACEF_LEVEL(2, "name '%s', depth %d, address cells %u, size cells %u\n",
                      name, depth, address_cells[depth], size_cells[depth]);

        /* look for the 'memory@*' property */
        if (strncmp(name, "memory@", 7) == 0 && depth == 1) {
            int lenp;
            const uint8_t *prop_ptr = fdt_getprop(fdt, offset, "reg", &lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' reg prop len %d, ac %u, sc %u\n", name, lenp,
                              address_cells[depth], size_cells[depth]);
                /* we're looking at a memory descriptor */
                uint64_t base = 0;
                uint64_t len = 0;
                if (address_cells[depth] == 2 && lenp >= 8) {
                    base = fdt64_to_cpu(*(const uint64_t *)prop_ptr);
                    prop_ptr += 8;
                    lenp -= 8;
                } else {
                    PANIC_UNIMPLEMENTED;
                }
                if (size_cells[depth] == 2 && lenp >= 8) {
                    len = fdt64_to_cpu(*((const uint64_t *)prop_ptr));
                    prop_ptr += 8;
                    lenp -= 8;
                } else {
                    PANIC_UNIMPLEMENTED;
                }

                if (cb->mem) {
                    LTRACEF("calling mem callback with base %#llx len %#llx\n", base, len);
                    cb->mem(base, len, cb->memcookie);
                }
            }
        }

        /* look for a cpu leaf and count the number of cpus */
        if (strncmp(name, "cpu@", 4) == 0 && depth == 2) {
            int lenp;
            const uint8_t *prop_ptr = fdt_getprop(fdt, offset, "reg", &lenp);
            LTRACEF("%p, lenp %u\n", prop_ptr, lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' reg prop len %d, ac %u, sc %u\n", name, lenp,
                              address_cells[depth], size_cells[depth]);
                uint32_t id = 0;
                if (address_cells[depth] == 1 && lenp >= 4) {
                    id = fdt32_to_cpu(*(const uint32_t *)prop_ptr);
                    prop_ptr += 4;
                    lenp -= 4;
                } else {
                    PANIC_UNIMPLEMENTED;
                }

                if (cb->cpu) {
                    LTRACEF("calling cpu callback with id %#x\n", id);
                    cb->cpu(id, cb->cpucookie);
                }
            }
        }
    }

    return NO_ERROR;
}

