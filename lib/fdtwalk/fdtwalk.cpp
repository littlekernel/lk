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
const uint32_t DEFAULT_ADDRESS_CELLS = 2;
const uint32_t DEFAULT_SIZE_CELLS = 1;

/* Read this node's effective #address-cells and #size-cells using libfdt.
 * These APIs already apply DT defaults (2/1) if properties are not present.
 */
void read_address_size_cells(const void *fdt, int offset, int depth,
                             uint32_t *address_cells, uint32_t *size_cells) {
    LTRACEF_LEVEL(3, "fdt %p, offset %d depth %d\n", fdt, offset, depth);

    DEBUG_ASSERT(depth >= 0 && depth < MAX_DEPTH);

    int ac = fdt_address_cells(fdt, offset);
    if (ac >= 0) {
        address_cells[depth] = static_cast<uint32_t>(ac);
    } else {
        LTRACEF("fdt_address_cells failed at offset %d: %d\n", offset, ac);
    }

    int sc = fdt_size_cells(fdt, offset);
    if (sc >= 0) {
        size_cells[depth] = static_cast<uint32_t>(sc);
    } else {
        LTRACEF("fdt_size_cells failed at offset %d: %d\n", offset, sc);
    }

    LTRACEF_LEVEL(3, "address-cells %u size-cells %u\n", address_cells[depth], size_cells[depth]);
}

status_t consume_cells_u64(const uint8_t **ptr, size_t *remaining_len, size_t cells,
                           uint64_t *out, bool require_fit) {
    if (cells > FDT_MAX_NCELLS) {
        return ERR_OUT_OF_RANGE;
    }

    if (cells == 0) {
        *out = 0;
        return NO_ERROR;
    }

    if (*remaining_len < cells * sizeof(fdt32_t)) {
        return ERR_BAD_LEN;
    }

    uint64_t value = 0;
    for (size_t i = 0; i < cells; ++i) {
        uint32_t cell = fdt32_ld(reinterpret_cast<const fdt32_t *>(*ptr));

        if (require_fit && i + 2 < cells && cell != 0) {
            return ERR_OUT_OF_RANGE;
        }

        value = (value << 32) | cell;
        *ptr += sizeof(fdt32_t);
        *remaining_len -= sizeof(fdt32_t);
    }

    *out = value;
    return NO_ERROR;
}

struct pci_ranges_entry {
    uint32_t type;
    bool prefetchable;
    uint64_t child_addr;
    uint64_t parent_addr;
    uint64_t size;
};

struct pci_ranges_entry_result {
    status_t status;
    pci_ranges_entry entry;
};

pci_ranges_entry_result read_pci_ranges_entry(const uint8_t **ptr, size_t *remaining_len,
                                              size_t child_addr_cells, size_t parent_addr_cells,
                                              size_t size_cells) {
    pci_ranges_entry_result result = {};

    if (child_addr_cells == 0) {
        result.status = ERR_OUT_OF_RANGE;
        return result;
    }

    uint64_t child_hi = 0;
    status_t err = consume_cells_u64(ptr, remaining_len, 1, &child_hi, true);
    if (err != NO_ERROR) {
        result.status = err;
        return result;
    }
    static constexpr uint32_t PCI_RANGE_SPACE_CODE_MASK = 0x03000000;
    static constexpr uint32_t PCI_RANGE_PREFETCHABLE_MASK = 0x40000000;
    uint32_t child_hi_32 = static_cast<uint32_t>(child_hi);
    result.entry.type = child_hi_32 & PCI_RANGE_SPACE_CODE_MASK;
    result.entry.prefetchable = (child_hi_32 & PCI_RANGE_PREFETCHABLE_MASK) != 0;

    err = consume_cells_u64(ptr, remaining_len, child_addr_cells - 1, &result.entry.child_addr, true);
    if (err != NO_ERROR) {
        result.status = err;
        return result;
    }

    err = consume_cells_u64(ptr, remaining_len, parent_addr_cells, &result.entry.parent_addr, true);
    if (err != NO_ERROR) {
        result.status = err;
        return result;
    }

    err = consume_cells_u64(ptr, remaining_len, size_cells, &result.entry.size, true);
    if (err != NO_ERROR) {
        result.status = err;
        return result;
    }

    result.status = NO_ERROR;
    return result;
}

struct base_len_pair {
    uint64_t base;
    uint64_t len;
};

struct base_len_pair_result {
    status_t status;
    base_len_pair entry;
};

base_len_pair_result read_base_len_pair(const uint8_t *prop_ptr, size_t prop_len,
                                        size_t address_cell_size, size_t size_cell_size) {
    base_len_pair_result result = {};
    result.status = consume_cells_u64(&prop_ptr, &prop_len, address_cell_size, &result.entry.base, true);
    if (result.status != NO_ERROR) {
        return result;
    }
    result.status = consume_cells_u64(&prop_ptr, &prop_len, size_cell_size, &result.entry.len, true);
    return result;
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
    if (str[lenp - 1] != '\0') {
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

    // Cell widths that describe how this node's children encode addresses/sizes.
    uint32_t node_address_cell() const { return address_cells[depth]; }
    uint32_t node_size_cell() const { return size_cells[depth]; }

    // Cell widths used to decode this node's reg-style tuples.
    uint32_t parent_address_cell() const { return depth > 0 ? address_cells[depth - 1] : DEFAULT_ADDRESS_CELLS; }
    uint32_t parent_size_cell() const { return depth > 0 ? size_cells[depth - 1] : DEFAULT_SIZE_CELLS; }
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
    state.address_cells[0] = DEFAULT_ADDRESS_CELLS;
    state.size_cells[0] = DEFAULT_SIZE_CELLS;
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

        /* #address-cells/#size-cells are parent-scoped in DT. Each node's own
         * effective child encoding widths come from local override, else defaults.
         * We intentionally do not inherit ancestor overrides through the tree.
         */
        state.address_cells[state.depth] = DEFAULT_ADDRESS_CELLS;
        state.size_cells[state.depth] = DEFAULT_SIZE_CELLS;
        read_address_size_cells(fdt, state.offset, state.depth, state.address_cells, state.size_cells);

        /* get the name */
        const char *name = fdt_get_name(fdt, state.offset, NULL);
        if (!name) {
            continue;
        }

        LTRACEF_LEVEL(2, "name '%s', depth %d, node ac/sc %u/%u parent ac/sc %u/%u\n",
                      name, state.depth,
                      state.node_address_cell(), state.node_size_cell(),
                      state.parent_address_cell(), state.parent_size_cell());

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
        printf("offset %d depth %d node ac/sc %u/%u parent ac/sc %u/%u name '%s'\n", state.offset, state.depth,
               state.node_address_cell(), state.node_size_cell(), state.parent_address_cell(), state.parent_size_cell(), name);
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
                              state.parent_address_cell(), state.parent_size_cell());
                uint32_t id = 0;
                {
                    size_t remaining = static_cast<size_t>(lenp);
                    uint64_t id64 = 0;
                    status_t err = consume_cells_u64(&prop_ptr, &remaining, state.parent_address_cell(), &id64, false);
                    if (err != NO_ERROR) {
                        LTRACEF("error reading cpu reg (%d)\n", err);
                        return;
                    }
                    id = static_cast<uint32_t>(id64);
                    lenp = static_cast<int>(remaining);
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
        /* look for the 'memory@*' property */
        if (memory && *mem_count < max_memory_index) {
            if (strncmp(name, "memory@", 7) == 0 && state.depth == 1) {
                int lenp;
                const uint8_t *prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "reg", &lenp);
                if (prop_ptr) {
                    LTRACEF_LEVEL(2, "found '%s' reg prop len %d, ac %u, sc %u\n", name, lenp,
                                  state.parent_address_cell(), state.parent_size_cell());
                    /* we're looking at a memory descriptor */
                    auto result = read_base_len_pair(prop_ptr, lenp, state.parent_address_cell(), state.parent_size_cell());
                    if (result.status != NO_ERROR) {
                        TRACEF("error reading base/length from memory@ node\n");
                        /* continue on */
                    } else {
                        LTRACEF("mem base %#llx len %#llx\n", result.entry.base, result.entry.len);
                        memory[*mem_count].base = result.entry.base;
                        memory[*mem_count].len = result.entry.len;
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
                                      state.parent_address_cell(), state.parent_size_cell());
                        /* we're looking at a memory descriptor */
                        auto result = read_base_len_pair(prop_ptr, lenp, state.parent_address_cell(), state.parent_size_cell());
                        if (result.status != NO_ERROR) {
                            TRACEF("error reading base/length from reserved-memory node\n");
                            /* continue on */
                        } else {
                            LTRACEF("reserved memory base %#llx len %#llx\n", result.entry.base, result.entry.len);
                            reserved_memory[*reserved_mem_count].base = result.entry.base;
                            reserved_memory[*reserved_mem_count].len = result.entry.len;
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
                              state.parent_address_cell(), state.parent_size_cell());

                auto result = read_base_len_pair(prop_ptr, static_cast<size_t>(lenp),
                                                  state.parent_address_cell(), state.parent_size_cell());
                if (result.status != NO_ERROR) {
                    TRACEF("error reading base/length from pcie@ node\n");
                } else {
                    info[*count].ecam_base = result.entry.base;
                    info[*count].ecam_len = result.entry.len;
                }
            }

            /* find which bus range the ecam covers */
            prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "bus-range", &lenp);
            LTRACEF("%p, lenp %u\n", prop_ptr, lenp);
            if (prop_ptr) {
                if (lenp != 2 * static_cast<int>(sizeof(fdt32_t))) {
                    TRACEF("invalid '%s' bus-range length %d (expected 8)\n", name, lenp);
                    return;
                }

                uint32_t bus_start = fdt32_ld(reinterpret_cast<const fdt32_t *>(prop_ptr));
                prop_ptr += sizeof(fdt32_t);
                uint32_t bus_end = fdt32_ld(reinterpret_cast<const fdt32_t *>(prop_ptr));

                if (bus_start > UINT8_MAX || bus_end > UINT8_MAX) {
                    TRACEF("invalid '%s' bus-range values %#x..%#x (> 0xff)\n",
                           name, bus_start, bus_end);
                    return;
                }
                if (bus_start > bus_end) {
                    TRACEF("invalid '%s' bus-range order %#x..%#x (start > end)\n",
                           name, bus_start, bus_end);
                    return;
                }

                info[*count].bus_start = static_cast<uint8_t>(bus_start);
                info[*count].bus_end = static_cast<uint8_t>(bus_end);
                LTRACEF_LEVEL(2, "parsed '%s' prop 'bus-range' [%u..%u]\n",
                              name, info[*count].bus_start, info[*count].bus_end);
            }

            prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "ranges", &lenp);
            LTRACEF("%p, lenp %u\n", prop_ptr, lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' prop 'ranges' len %d, ac %u, sc %u\n", name, lenp,
                              state.node_address_cell(), state.node_size_cell());

                /* iterate this packed property */
                size_t remaining_len = static_cast<size_t>(lenp);
                while (remaining_len > 0) {
                    auto entry_result = read_pci_ranges_entry(&prop_ptr, &remaining_len,
                                                              state.node_address_cell(),
                                                              state.parent_address_cell(),
                                                              state.node_size_cell());
                    if (entry_result.status != NO_ERROR) {
                        TRACEF("error reading pcie ranges entry (%d)\n", entry_result.status);
                        break;
                    }
                    auto &entry = entry_result.entry;

                    switch (entry.type) {
                        case 0x00000000: // config space
                            LTRACEF_LEVEL(2, "config range (ignored) prefetchable %u\n", entry.prefetchable);
                            break;
                        case 0x1000000: // io range
                            LTRACEF_LEVEL(2, "io range\n");
                            info[*count].io_base = entry.child_addr;
                            info[*count].io_base_mmio = entry.parent_addr;
                            info[*count].io_len = entry.size;
                            break;
                        case 0x2000000: // mmio range
                            LTRACEF_LEVEL(2, "mmio range prefetchable %u\n", entry.prefetchable);
                            if (entry.prefetchable) {
                                info[*count].mmio_prefetch_base = entry.child_addr;
                                info[*count].mmio_prefetch_len = entry.size;
                            } else {
                                info[*count].mmio_base = entry.child_addr;
                                info[*count].mmio_len = entry.size;
                            }
                            break;
                        case 0x3000000: // mmio range (64bit)
                            LTRACEF_LEVEL(2, "mmio range (64bit) prefetchable %u\n", entry.prefetchable);
                            if (entry.prefetchable) {
                                info[*count].mmio64_prefetch_base = entry.child_addr;
                                info[*count].mmio64_prefetch_len = entry.size;
                            } else {
                                info[*count].mmio64_base = entry.child_addr;
                                info[*count].mmio64_len = entry.size;
                            }
                            break;
                        default:
                            TRACEF("unhandled pcie ranges type %#x (prefetchable %u)\n",
                                   entry.type, entry.prefetchable);
                    }

                    LTRACEF_LEVEL(2, "base %#llx base2 %#llx size %#llx\n",
                                  entry.child_addr, entry.parent_addr, entry.size);
                }
            }

            (*count)++;
        }
    };

    return _fdt_walk(fdt, walker);
}

status_t fdt_walk_find_gic_info(const void *fdt, struct fdt_walk_gic_info *info, size_t *count) {
    const size_t info_len = *count;
    *count = 0;
    int gic_node_depth = -1;
    fdt_walk_gic_info *gic_infop = nullptr;
    auto walker = [info, info_len, &count, &gic_node_depth, &gic_infop](const fdt_walk_state &state, const char *name) {
        // Track ITS/v2m subnodes within a GIC node
        if (gic_node_depth >= 0) {
            if (state.depth <= gic_node_depth) {
                // Exited the GIC subtree
                gic_node_depth = -1;
                gic_infop = nullptr;
            } else {
                // Inside the GIC subtree — look for child nodes at the direct child level
                if (state.depth == gic_node_depth + 1) {
                    const char *child_compat = get_prop_string(state.fdt, state.offset, "compatible");
                    if (child_compat) {
                        if (strstr(child_compat, "arm,gic-v3-its") != nullptr) {
                            size_t idx = gic_infop->v3.its_count;
                            if (idx < FDT_WALK_MAX_GIC_ITS) {
                                int lenp;
                                const uint8_t *prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "reg", &lenp);
                                if (prop_ptr) {
                                    auto result = read_base_len_pair(prop_ptr, static_cast<size_t>(lenp),
                                                                     state.parent_address_cell(), state.parent_size_cell());
                                    if (result.status == NO_ERROR) {
                                        LTRACEF("found ITS subnode, base %#llx len %#llx\n",
                                                result.entry.base, result.entry.len);
                                        gic_infop->v3.its[idx].base = result.entry.base;
                                        gic_infop->v3.its[idx].len = result.entry.len;
                                        gic_infop->v3.its_count++;
                                    }
                                }
                            }
                        } else if (strstr(child_compat, "arm,gic-v2m-frame") != nullptr) {
                            size_t idx = gic_infop->v2.v2m_count;
                            if (idx < FDT_WALK_MAX_GIC_V2M) {
                                int lenp;
                                const uint8_t *prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "reg", &lenp);
                                if (prop_ptr) {
                                    auto result = read_base_len_pair(prop_ptr, static_cast<size_t>(lenp),
                                                                     state.parent_address_cell(), state.parent_size_cell());
                                    if (result.status == NO_ERROR) {
                                        LTRACEF("found GICv2m frame subnode, base %#llx len %#llx\n",
                                                result.entry.base, result.entry.len);
                                        gic_infop->v2.v2m_frame[idx].base = result.entry.base;
                                        gic_infop->v2.v2m_frame[idx].len = result.entry.len;
                                        gic_infop->v2.v2m_count++;
                                    }
                                }
                            }
                        }
                    }
                }
                return;
            }
        }

        /* look for a gic node and pass the address of the ecam and other info to the callback */
        fdt_walk_gic_info *infop = &info[*count];
        if (*count < info_len) {
            int lenp;

            /* does it have the node 'interrupt-controller' */
            if (!fdt_getprop(state.fdt, state.offset, "interrupt-controller", &lenp)) {
                return;
            }

            // check against a list of compatible strings
            const char *compat = get_prop_string(state.fdt, state.offset, "compatible");
            if (!compat) {
                return;
            }

            enum { UNKNOWN = 0,
                   GIC_V2 = 2,
                   GIC_V3 = 3 } found_version = UNKNOWN;

            char const *gic_v2_compat_list[] = {
                "arm,arm1176jzf-devchip-gic",
                "arm,arm11mp-gic",
                "arm,cortex-a15-gic",
                "arm,cortex-a7-gic",
                "arm,cortex-a9-gic",
                "arm,eb11mp-gic",
                "arm,gic-400",
                "arm,pl390",
                "arm,tc11mp-gic",
                "brcm,brahma-b15-gic",
                "nvidia,tegra210-agic",
                "qcom,msm-8660-qgic",
                "qcom,msm-qgic2"};
            for (const char *gic_compat : gic_v2_compat_list) {
                if (strstr(compat, gic_compat) != nullptr) {
                    LTRACEF("compatible string '%s' matches gic v2 compat '%s'\n", compat, gic_compat);
                    found_version = GIC_V2;
                    break;
                }
            }
            char const *gic_v3_compat_list[] = {
                "arm,gic-v3",
                "qcom,msm8996-gic-v3",
            };
            for (const char *gic_compat : gic_v3_compat_list) {
                if (strstr(compat, gic_compat) != nullptr) {
                    LTRACEF("compatible string '%s' matches gic v3 compat '%s'\n", compat, gic_compat);
                    found_version = GIC_V3;
                    break;
                }
            }
            if (found_version == UNKNOWN) {
                return;
            }

            // match found
            LTRACEF("found gic node '%s'\n", name);

            // zero out the info struct, we're about to fill it in with what we find in the dt
            *infop = {};

            // at this point, we have a gic node either v2 or v3, so read out relevant properties
            const uint8_t *prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "#interrupt-cells", &lenp);
            LTRACEF_LEVEL(3, "%p, len %d\n", prop_ptr, lenp);
            if (prop_ptr && lenp == 4) {
                infop->interrupt_cells = fdt32_ld((const fdt32_t *)prop_ptr);
            } else {
                // default to 3
                infop->interrupt_cells = 3;
            }

            /* find the mmio range */
            prop_ptr = (const uint8_t *)fdt_getprop(state.fdt, state.offset, "reg", &lenp);
            LTRACEF("%p, lenp %u\n", prop_ptr, lenp);
            if (prop_ptr) {
                LTRACEF_LEVEL(2, "found '%s' prop 'reg' len %d, ac %u, sc %u\n", name, lenp,
                              state.parent_address_cell(), state.parent_size_cell());
            }

            const size_t gic_ac = state.parent_address_cell();
            const size_t gic_sc = state.parent_size_cell();
            const size_t gic_pair_size = (gic_ac + gic_sc) * sizeof(fdt32_t);
            size_t gic_remaining = static_cast<size_t>(lenp);

            if (found_version == GIC_V2) {
                if (gic_remaining < gic_pair_size * 2) {
                    printf("gic v2 reg property too small, len %zu\n", gic_remaining);
                    return;
                }

                infop->gic_version = 2;

                // read the v2 specific mmio range
                consume_cells_u64(&prop_ptr, &gic_remaining, gic_ac, &infop->v2.distributor_base, true);
                consume_cells_u64(&prop_ptr, &gic_remaining, gic_sc, &infop->v2.distributor_len, true);
                consume_cells_u64(&prop_ptr, &gic_remaining, gic_ac, &infop->v2.cpu_interface_base, true);
                consume_cells_u64(&prop_ptr, &gic_remaining, gic_sc, &infop->v2.cpu_interface_len, true);
            } else if (found_version == GIC_V3) {
                if (gic_remaining < gic_pair_size * 2) {
                    printf("gic v3 reg property too small, len %zu\n", gic_remaining);
                    return;
                }

                infop->gic_version = 3;

                // read v3 specific properties here
                consume_cells_u64(&prop_ptr, &gic_remaining, gic_ac, &infop->v3.distributor_base, true);
                consume_cells_u64(&prop_ptr, &gic_remaining, gic_sc, &infop->v3.distributor_len, true);
                consume_cells_u64(&prop_ptr, &gic_remaining, gic_ac, &infop->v3.redistributor_base, true);
                consume_cells_u64(&prop_ptr, &gic_remaining, gic_sc, &infop->v3.redistributor_len, true);
                // the rest of these are optional
                if (gic_remaining >= gic_pair_size) {
                    consume_cells_u64(&prop_ptr, &gic_remaining, gic_ac, &infop->v3.cpu_interface_base, true);
                    consume_cells_u64(&prop_ptr, &gic_remaining, gic_sc, &infop->v3.cpu_interface_len, true);
                }
                if (gic_remaining >= gic_pair_size) {
                    consume_cells_u64(&prop_ptr, &gic_remaining, gic_ac, &infop->v3.hypervisor_interface_base, true);
                    consume_cells_u64(&prop_ptr, &gic_remaining, gic_sc, &infop->v3.hypervisor_interface_len, true);
                }
                if (gic_remaining >= gic_pair_size) {
                    consume_cells_u64(&prop_ptr, &gic_remaining, gic_ac, &infop->v3.virtual_interface_base, true);
                    consume_cells_u64(&prop_ptr, &gic_remaining, gic_sc, &infop->v3.virtual_interface_len, true);
                }

            }

            (*count)++;

            // Track GIC subtree so we can detect child subnodes on subsequent nodes
            if (found_version == GIC_V2 || found_version == GIC_V3) {
                gic_node_depth = state.depth;
                gic_infop = infop;
            }
        }
    };

    return _fdt_walk(fdt, walker);
}