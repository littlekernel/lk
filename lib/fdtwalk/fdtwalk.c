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

static status_t read_base_len_pair(const uint8_t *prop_ptr, size_t prop_len,
                                   size_t address_cell_size, size_t size_cell_size,
                                   uint64_t *base, uint64_t *len) {
    *base = 0;
    *len = 0;

    /* we're looking at a memory descriptor */
    if (address_cell_size == 2 && prop_len >= 8) {
        *base = fdt64_to_cpu(*(const uint64_t *)prop_ptr);
        prop_ptr += 8;
        prop_len -= 8;
    } else if (address_cell_size == 1 && prop_len >= 4) {
        *base = fdt32_to_cpu(*(const uint32_t *)prop_ptr);
        prop_ptr += 4;
        prop_len -= 4;
    } else {
        return ERR_NOT_IMPLEMENTED;
    }

    if (size_cell_size == 2 && prop_len >= 8) {
        *len = fdt64_to_cpu(*((const uint64_t *)prop_ptr));
        prop_ptr += 8;
        prop_len -= 8;
    } else if (size_cell_size == 1 && prop_len >= 4) {
        *len = fdt32_to_cpu(*(const uint32_t *)prop_ptr);
        prop_ptr += 4;
        prop_len -= 4;
    } else {
        return ERR_NOT_IMPLEMENTED;
    }

    return NO_ERROR;
}

// returns true or false if a particular property is a particular value
static bool check_prop_is_val_string(const void *fdt, int offset, const char *prop, const char *val) {
    int lenp;
    const uint8_t *prop_ptr = fdt_getprop(fdt, offset, prop, &lenp);
    if (!prop_ptr || lenp <= 0) {
        return false;
    }

    if (strncmp(val, (const char *)prop_ptr, strlen(val)) == 0) {
        return true;
    }

    return false;
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

    /* if >= 0, we're inside /reserved-memory */
    int reserved_memory_depth = -1;

    /* read the address/size cells properties at the root, if present */
    address_cells[0] = 2;
    size_cells[0] = 1;
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
        if (cb->mem && strncmp(name, "memory@", 7) == 0 && depth == 1) {
            int lenp;
            const uint8_t *prop_ptr = fdt_getprop(fdt, offset, "reg", &lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' reg prop len %d, ac %u, sc %u\n", name, lenp,
                              address_cells[depth], size_cells[depth]);
                /* we're looking at a memory descriptor */
                uint64_t base;
                uint64_t len;
                err = read_base_len_pair(prop_ptr, lenp, address_cells[depth], size_cells[depth], &base, &len);
                if (err != NO_ERROR) {
                    TRACEF("error reading base/length from memory@ node\n");
                    /* continue on */
                } else {
                    LTRACEF("calling mem callback with base %#llx len %#llx\n", base, len);
                    cb->mem(base, len, cb->memcookie);
                }
            }
        }

        /* look for the 'reserved-memory' tree */
        if (cb->reserved_memory) {
            /* once we see the reserved-memory first level node, track that we are inside
             * it until we step out to a node at the same or higher depth.
             */
            if (strncmp(name, "reserved-memory", 15) == 0 && depth == 1) {
                LTRACEF_LEVEL(2, "found reserved memory node\n");

                reserved_memory_depth = depth;
            } else if (reserved_memory_depth >= 0) {
                if (depth <= reserved_memory_depth) {
                    /* we have exited the reserved memory tree, so clear our tracking depth */
                    LTRACEF_LEVEL(2, "exiting reserved memory node\n");
                    reserved_memory_depth = -1;
                } else {
                    /* if we're inside the reserved meory tree, so this node must
                     * be a reserved memory region */
                    int lenp;
                    const uint8_t *prop_ptr = fdt_getprop(fdt, offset, "reg", &lenp);
                    if (prop_ptr) {
                        LTRACEF_LEVEL(2, "found '%s' reg prop len %d, ac %u, sc %u\n", name, lenp,
                                      address_cells[depth], size_cells[depth]);
                        /* we're looking at a memory descriptor */
                        uint64_t base;
                        uint64_t len;
                        err = read_base_len_pair(prop_ptr, lenp, address_cells[depth], size_cells[depth], &base, &len);
                        if (err != NO_ERROR) {
                            TRACEF("error reading base/length from reserved-memory node\n");
                            /* continue on */
                        } else {
                            LTRACEF("calling reserved memory callback with base %#llx len %#llx\n", base, len);
                            cb->reserved_memory(base, len, cb->reserved_memory_cookie);
                        }
                    }
                }
            }
        }

        /* look for a cpu leaf and count the number of cpus */
        if (cb->cpu && strncmp(name, "cpu@", 4) == 0 && depth == 2) {
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

                // is it disabled?
                if (check_prop_is_val_string(fdt, offset, "status", "disabled")) {
                    LTRACEF("cpu id %#x is disabled, skipping...\n", id);
                } else {
                    LTRACEF("calling cpu callback with id %#x\n", id);
                    cb->cpu(id, cb->cpucookie);
                }
            }
        }

        /* look for a pcie leaf and pass the address of the ecam and other info to the callback */
        if (cb->pcie && (strncmp(name, "pcie@", 5) == 0 || strncmp(name, "pci@", 4) == 0)) {
            struct fdt_walk_pcie_info info = {0};

            /* find the range of the ecam */
            int lenp;
            const uint8_t *prop_ptr = fdt_getprop(fdt, offset, "reg", &lenp);
            LTRACEF("%p, lenp %u\n", prop_ptr, lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' prop 'reg' len %d, ac %u, sc %u\n", name, lenp,
                              address_cells[depth], size_cells[depth]);

                /* seems to always be full address cells 2, size cells 2, despite it being 3/2 */
                info.ecam_base = fdt64_to_cpu(*(const uint64_t *)prop_ptr);
                prop_ptr += 8;
                info.ecam_len = fdt64_to_cpu(*(const uint64_t *)prop_ptr);
            }

            /* find which bus range the ecam covers */
            prop_ptr = fdt_getprop(fdt, offset, "bus-range", &lenp);
            LTRACEF("%p, lenp %u\n", prop_ptr, lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' prop 'bus-range' len %d, ac %u, sc %u\n", name, lenp,
                              address_cells[depth], size_cells[depth]);

                if (lenp == 8) {
                    info.bus_start = fdt32_to_cpu(*(const uint32_t *)prop_ptr);
                    prop_ptr += 4;
                    info.bus_end = fdt32_to_cpu(*(const uint32_t *)prop_ptr);
                }
            }

            prop_ptr = fdt_getprop(fdt, offset, "ranges", &lenp);
            LTRACEF("%p, lenp %u\n", prop_ptr, lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' prop 'ranges' len %d, ac %u, sc %u\n", name, lenp,
                              address_cells[depth], size_cells[depth]);

                /* iterate this packed property */
                const uint8_t *prop_end = prop_ptr + lenp;
                while (prop_ptr < prop_end) {
                    uint32_t type = fdt32_to_cpu(*(const uint32_t *)(prop_ptr));
                    prop_ptr += 4;

                    /* read 3 64bit values */
                    uint64_t base1, base2, size;
                    base1 = fdt64_to_cpu(*(const uint64_t *)prop_ptr);
                    prop_ptr += 8;
                    base2 = fdt64_to_cpu(*(const uint64_t *)prop_ptr);
                    prop_ptr += 8;
                    size = fdt64_to_cpu(*(const uint64_t *)prop_ptr);
                    prop_ptr += 8;

                    switch (type) {
                        case 0x1000000: // io range
                            LTRACEF_LEVEL(2, "io range\n");
                            info.io_base = base1;
                            info.io_base_mmio = base2;
                            info.io_len = size;
                            break;
                        case 0x2000000: // mmio range
                            LTRACEF_LEVEL(2, "mmio range\n");
                            info.mmio_base = base1;
                            info.mmio_len = size;
                            break;
                        case 0x3000000: // mmio range (64bit)
                            LTRACEF_LEVEL(2, "mmio range (64bit)\n");
                            info.mmio64_base = base1;
                            info.mmio64_len = size;
                            break;
                        default:
                            LTRACEF_LEVEL(2, "unhandled type %#x\n", type);
                    }

                    LTRACEF_LEVEL(2, "base %#llx base2 %#llx size %#llx\n", base1, base2, size);
                }
            }

            if (info.ecam_len > 0) {
                LTRACEF("calling pci callback with ecam base %#llx size %#llx\n", info.ecam_base, info.ecam_len);
                cb->pcie(&info, cb->pciecookie);
            }
        }

    }

    return NO_ERROR;
}

