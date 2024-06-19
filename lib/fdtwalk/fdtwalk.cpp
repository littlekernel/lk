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
#include <lk/cpp.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdio.h>
#include <sys/types.h>

#define LOCAL_TRACE 0

namespace {

const int MAX_DEPTH = 16;

/* read the #address-cells and #size-cells properties at the current node to
 * see if there are any overriding sizes at this level. It's okay to not
 * find the properties.
 */
void read_address_size_cells(const void *fdt, int offset, int depth,
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

status_t read_base_len_pair(const uint8_t *prop_ptr, size_t prop_len,
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
bool check_prop_is_val_string(const void *fdt, int offset, const char *prop, const char *val) {
    int lenp;
    const uint8_t *prop_ptr = static_cast<const uint8_t *>(fdt_getprop(fdt, offset, prop, &lenp));
    if (!prop_ptr || lenp <= 0) {
        return false;
    }

    if (strncmp(val, reinterpret_cast<const char *>(prop_ptr), strlen(val)) == 0) {
        return true;
    }

    return false;
}

const char *get_prop_string(const void *fdt, int offset, const char *prop) {
    int lenp;
    const uint8_t *prop_ptr = static_cast<const uint8_t *>(fdt_getprop(fdt, offset, prop, &lenp));
    if (!prop_ptr || lenp <= 0) {
        return nullptr;
    }

    // check to see that it appears to be null terminated
    auto str = reinterpret_cast<const char *>(prop_ptr);
    if (str[lenp-1] != '\0') {
        return nullptr;
    }

    // seems safe
    return str;
}

struct fdt_walk_state {
    const void *fdt;
    int offset;
    int depth;
    uint32_t address_cells[MAX_DEPTH];
    uint32_t size_cells[MAX_DEPTH];

    uint32_t curr_address_cell() const { return address_cells[depth]; }
    uint32_t curr_size_cell() const { return size_cells[depth]; }
};

// Inner page table walker routine. Takes a callback in the form of a function or lambda
// and calls on every node in the tree.
template <typename callback>
status_t _fdt_walk(const void *fdt, callback cb) {
    int err = fdt_check_header(fdt);
    if (err != 0) {
        return ERR_NOT_FOUND;
    }

    /* walk the nodes */
    fdt_walk_state state = {};
    state.fdt = fdt;

    /* read the address/size cells properties at the root, if present */
    state.address_cells[0] = 2;
    state.size_cells[0] = 1;
    read_address_size_cells(fdt, state.offset, 0, state.address_cells, state.size_cells);

    for (;;) {
        state.offset = fdt_next_node(fdt, state.offset, &state.depth);
        if (state.offset < 0 || state.depth < 0) {
            break;
        }

        LTRACEF_LEVEL(3, "fdt_next node offset %d, depth %d\n", state.offset, state.depth);

        if (state.depth >= MAX_DEPTH) {
            printf("FDTWALK: exceeded max depth %d\n", MAX_DEPTH);
            return ERR_NO_MEMORY;
        }

        // TODO: fix the way address and size cells are inherited, they're not exactly correct
        // here.

        /* copy the address/size cells from the parent depth and then see if we
         * have local properties to override it. */
        if (state.depth > 0) {
            state.address_cells[state.depth] = state.address_cells[state.depth - 1];
            state.size_cells[state.depth] = state.size_cells[state.depth - 1];
        }
        read_address_size_cells(fdt, state.offset, state.depth, state.address_cells, state.size_cells);

        /* get the name */
        const char *name = fdt_get_name(fdt, state.offset, NULL);
        if (!name)
            continue;

        LTRACEF_LEVEL(2, "name '%s', depth %d, address cells %u, size cells %u\n",
                      name, state.depth, state.address_cells[state.depth], state.size_cells[state.depth]);

        // Callback
        cb(state, name);
    }

    return NO_ERROR;
}

} // anonymous namespace

status_t fdt_walk_dump(const void *fdt) {
    auto cb = [](const fdt_walk_state &state, const char *name) {
        for (auto i = 0; i < state.depth; i++) {
            printf("  ");
        }
        printf("offset %d depth %d acells %u scells %u name '%s'\n", state.offset, state.depth,
                state.curr_address_cell(), state.curr_size_cell(), name);
    };

    printf("FDT dump: address %p total size %#x\n", fdt, fdt_totalsize(fdt));

    return _fdt_walk(fdt, cb);
}

status_t fdt_walk_find_cpus(const void *fdt, struct fdt_walk_cpu_info *cpu, size_t *cpu_count) {
    const size_t max_cpu_count = *cpu_count;
    *cpu_count = 0;

    auto walker = [max_cpu_count, cpu, cpu_count](const fdt_walk_state &state, const char *name) {
        /* look for a cpu leaf and count the number of cpus */
        if (*cpu_count < max_cpu_count && strncmp(name, "cpu@", 4) == 0 && state.depth == 2) {
            int lenp;
            const uint8_t *prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "reg", &lenp);
            LTRACEF("%p, lenp %u\n", prop_ptr, lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' reg prop len %d, ac %u, sc %u\n", name, lenp,
                              state.curr_address_cell(), state.curr_size_cell());
                uint32_t id = 0;
                if (state.curr_address_cell() == 1 && lenp >= 4) {
                    id = fdt32_to_cpu(*(const uint32_t *)prop_ptr);
                    prop_ptr += 4;
                    lenp -= 4;
                } else {
                    PANIC_UNIMPLEMENTED;
                }

                // is it disabled?
                if (check_prop_is_val_string(state.fdt, state.offset, "status", "disabled")) {
                    LTRACEF("cpu id %#x is disabled, skipping...\n", id);
                    return;
                }

                // clear the cpu state, we're about to write down some information about it
                cpu[*cpu_count] = {};

#if ARCH_RISCV
                // look for riscv,isa and riscv,isa-extensions
                auto isa_string = get_prop_string(state.fdt, state.offset, "riscv,isa");
                if (isa_string) {
                    cpu[*cpu_count].isa_string = isa_string;
                }

                auto isa_extensions_string = get_prop_string(state.fdt, state.offset, "riscv,isa-extensions");
                if (isa_extensions_string) {
                    cpu[*cpu_count].isa_extensions_string = isa_extensions_string;
                }
#endif

                // cpu is found
                LTRACEF("found cpu id %u\n", id);
                cpu[*cpu_count].id = id;
                (*cpu_count)++;
            }
        }
    };

    return _fdt_walk(fdt, walker);
}

status_t fdt_walk_find_memory(const void *fdt, struct fdt_walk_memory_region *memory, size_t *mem_count,
                              struct fdt_walk_memory_region *reserved_memory, size_t *reserved_mem_count) {
    /* if >= 0, we're inside /reserved-memory */
    int reserved_memory_depth = -1;
    const size_t max_memory_index = *mem_count;
    const size_t max_reserved_index = *reserved_mem_count;
    *mem_count = *reserved_mem_count = 0;

    auto walker = [&](const fdt_walk_state &state, const char *name) {
        int err;

        /* look for the 'memory@*' property */
        if (memory && *mem_count < max_memory_index) {
            if (strncmp(name, "memory@", 7) == 0 && state.depth == 1) {
                int lenp;
                const uint8_t *prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "reg", &lenp);
                if (prop_ptr) {
                    LTRACEF_LEVEL(2, "found '%s' reg prop len %d, ac %u, sc %u\n", name, lenp,
                                  state.curr_address_cell(), state.curr_size_cell());
                    /* we're looking at a memory descriptor */
                    uint64_t base;
                    uint64_t len;
                    err = read_base_len_pair(prop_ptr, lenp, state.curr_address_cell(), state.curr_size_cell(), &base, &len);
                    if (err != NO_ERROR) {
                        TRACEF("error reading base/length from memory@ node\n");
                        /* continue on */
                    } else {
                        LTRACEF("mem base %#llx len %#llx\n", base, len);
                        memory[*mem_count].base = base;
                        memory[*mem_count].len = len;
                        (*mem_count)++;
                    }
                }
            }
        }

        /* look for the 'reserved-memory' tree */
        if (reserved_memory && *reserved_mem_count < max_reserved_index) {
            /* once we see the reserved-memory first level node, track that we are inside
             * it until we step out to a node at the same or higher depth.
             */
            if (strncmp(name, "reserved-memory", 15) == 0 && state.depth == 1) {
                LTRACEF_LEVEL(2, "found reserved memory node\n");

                reserved_memory_depth = state.depth;
            } else if (reserved_memory_depth >= 0) {
                if (state.depth <= reserved_memory_depth) {
                    /* we have exited the reserved memory tree, so clear our tracking depth */
                    LTRACEF_LEVEL(2, "exiting reserved memory node\n");
                    reserved_memory_depth = -1;
                } else {
                    /* if we're inside the reserved meory tree, so this node must
                     * be a reserved memory region */
                    int lenp;
                    const uint8_t *prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "reg", &lenp);
                    if (prop_ptr) {
                        LTRACEF_LEVEL(2, "found '%s' reg prop len %d, ac %u, sc %u\n", name, lenp,
                                      state.curr_address_cell(), state.curr_size_cell());
                        /* we're looking at a memory descriptor */
                        uint64_t base;
                        uint64_t len;
                        err = read_base_len_pair(prop_ptr, lenp, state.curr_address_cell(), state.curr_size_cell(), &base, &len);
                        if (err != NO_ERROR) {
                            TRACEF("error reading base/length from reserved-memory node\n");
                            /* continue on */
                        } else {
                            LTRACEF("reserved memory base %#llx len %#llx\n", base, len);
                            reserved_memory[*reserved_mem_count].base = base;
                            reserved_memory[*reserved_mem_count].len = len;
                            (*reserved_mem_count)++;
                        }
                    }
                }
            }
        }
    };

    return _fdt_walk(fdt, walker);
}

status_t fdt_walk_find_pcie_info(const void *fdt, struct fdt_walk_pcie_info *info, size_t *count) {
    size_t info_len = *count;
    *count = 0;
    auto walker = [info, info_len, &count](const fdt_walk_state &state, const char *name) {
        /* look for a pcie leaf and pass the address of the ecam and other info to the callback */
        if (*count < info_len && (strncmp(name, "pcie@", 5) == 0 || strncmp(name, "pci@", 4) == 0)) {
            int lenp;

            /* check the status, is it disabled? */
            const uint8_t *prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "status", &lenp);
            if (prop_ptr) {
                if (fdt_stringlist_contains((const char *)prop_ptr, lenp, "disabled")) {
                    LTRACEF("found disabled pci node\n");
                    return;
                }
            }


            /* find the range of the ecam */
            prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "reg", &lenp);
            LTRACEF("%p, lenp %u\n", prop_ptr, lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' prop 'reg' len %d, ac %u, sc %u\n", name, lenp,
                              state.curr_address_cell(), state.curr_size_cell());

                /* seems to always be full address cells 2, size cells 2, despite it being 3/2 */
                info[*count].ecam_base = fdt64_to_cpu(*(const uint64_t *)prop_ptr);
                prop_ptr += 8;
                info[*count].ecam_len = fdt64_to_cpu(*(const uint64_t *)prop_ptr);
            }

            /* find which bus range the ecam covers */
            prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "bus-range", &lenp);
            LTRACEF("%p, lenp %u\n", prop_ptr, lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' prop 'bus-range' len %d, ac %u, sc %u\n", name, lenp,
                              state.curr_address_cell(), state.curr_size_cell());

                if (lenp == 8) {
                    info[*count].bus_start = fdt32_to_cpu(*(const uint32_t *)prop_ptr);
                    prop_ptr += 4;
                    info[*count].bus_end = fdt32_to_cpu(*(const uint32_t *)prop_ptr);
                }
            }

            prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "ranges", &lenp);
            LTRACEF("%p, lenp %u\n", prop_ptr, lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' prop 'ranges' len %d, ac %u, sc %u\n", name, lenp,
                              state.curr_address_cell(), state.curr_size_cell());

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
                            info[*count].io_base = base1;
                            info[*count].io_base_mmio = base2;
                            info[*count].io_len = size;
                            break;
                        case 0x2000000: // mmio range
                            LTRACEF_LEVEL(2, "mmio range\n");
                            info[*count].mmio_base = base1;
                            info[*count].mmio_len = size;
                            break;
                        case 0x3000000: // mmio range (64bit)
                            LTRACEF_LEVEL(2, "mmio range (64bit)\n");
                            info[*count].mmio64_base = base1;
                            info[*count].mmio64_len = size;
                            break;
                        default:
                            LTRACEF_LEVEL(2, "unhandled type %#x\n", type);
                    }

                    LTRACEF_LEVEL(2, "base %#llx base2 %#llx size %#llx\n", base1, base2, size);
                }
            }

            (*count)++;
        }
    };

    return _fdt_walk(fdt, walker);
}

