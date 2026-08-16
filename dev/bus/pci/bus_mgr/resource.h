/*
 * Copyright (c) 2021 Travis Geiseblrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <stdio.h>
#include <lk/cpp.h>
#include <lk/list.h>
#include <dev/bus/pci.h>

namespace pci {

struct resource_range {
    pci_resource_type type;
    uint64_t base;
    uint64_t size;

    void dump() const {
        printf("resource type %d: base %#llx size %#llx\n",
                type, base, size);
    }
};

// A pool of free address ranges of one kind, kept as a sorted, non overlapping list.
// Ranges can be added, carved out (reserved) and allocated from with alignment.
class range_pool {
public:
    range_pool() = default;
    ~range_pool();

    DISALLOW_COPY_ASSIGN_AND_MOVE(range_pool);

    // add [base, base + size) to the pool, merging with neighbors
    status_t add(uint64_t base, uint64_t size);

    // remove [base, base + size) from the pool. It's fine if parts (or all) of it are not
    // in the pool to begin with.
    status_t reserve(uint64_t base, uint64_t size);

    // find the lowest free range of size bytes aligned to 1 << align_log2, with
    // min_addr <= address and address + size - 1 <= max_addr
    status_t alloc(uint64_t size, uint8_t align_log2, uint64_t min_addr, uint64_t max_addr,
                   uint64_t *out);

    bool empty() const { return list_is_empty(const_cast<list_node *>(&free_)); }

    // total free bytes
    uint64_t free_bytes() const;

    void dump(const char *name) const;

private:
    struct range {
        list_node node;
        uint64_t base;
        uint64_t size;
        uint64_t end() const { return base + size - 1; }
    };

    list_node free_ = LIST_INITIAL_VALUE(free_);
};

// The set of pools a bus (or root) may hand out addresses from.
class resource_allocator {
public:
    // A root allocator holds the windows of a host bridge, where the prefetchable attribute is
    // only a hint, so non prefetchable requests may fall back to prefetchable windows there.
    // Below a bridge that is not allowed.
    explicit resource_allocator(bool is_root = false) : is_root_(is_root) {}
    ~resource_allocator() = default;

    DISALLOW_COPY_ASSIGN_AND_MOVE(resource_allocator);

    // add a window to the appropriate pool
    status_t add_range(pci_resource_type type, bool prefetchable, uint64_t base, uint64_t size);
    status_t add_range(const resource_range &range, bool prefetchable = false) {
        return add_range(range.type, prefetchable, range.base, range.size);
    }

    // carve an already assigned range out of every pool that may hold it
    status_t reserve(pci_resource_type type, uint64_t base, uint64_t size);

    status_t allocate_io(uint32_t size, uint8_t align, uint32_t *out);
    status_t allocate_mmio(bool can_be_64bit, bool prefetchable, uint64_t size, uint8_t align, uint64_t *out);

    // does the allocator have any space of this kind at all
    bool has_range(pci_resource_type type, bool prefetchable) const;

    void dump() const;

private:
    range_pool &type_to_pool(pci_resource_type type, bool prefetchable);
    const range_pool &type_to_pool(pci_resource_type type, bool prefetchable) const;

    const bool is_root_;

    range_pool io_;
    range_pool mmio_;
    range_pool mmio64_;
    range_pool mmio_prefetchable_;
    range_pool mmio64_prefetchable_;
};

} // namespace pci
