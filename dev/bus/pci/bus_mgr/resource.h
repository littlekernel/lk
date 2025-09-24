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
#include <dev/bus/pci.h>

namespace pci {

// TODO: move into separate file
struct resource_range {
    pci_resource_type type;
    uint64_t base;
    uint64_t size;

    void dump() const {
        printf("resource type %d: base %#llx size %#llx\n",
                type, base, size);
    }
};

struct resource_range_set {
    resource_range io;
    resource_range mmio;
    resource_range mmio64;
    resource_range mmio_prefetchable;
    resource_range mmio64_prefetchable;

    void dump() const {
        printf("resource range set:\n");
        io.dump();
        mmio.dump();
        mmio64.dump();
        mmio_prefetchable.dump();
        mmio64_prefetchable.dump();
    }
};

class resource_allocator {
public:
    resource_allocator() = default;
    ~resource_allocator() = default;

    status_t set_range(const resource_range &range, bool prefetchable = false);
    status_t allocate_io(uint32_t size, uint8_t align, uint32_t *out);
    status_t allocate_mmio(bool can_be_64bit, bool prefetchable, uint64_t size, uint8_t align, uint64_t *out);

private:
    resource_range &type_to_range(pci_resource_type type, bool prefetchable);

    resource_range_set ranges_ = {};
};

} // namespace pci
