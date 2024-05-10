/*
 * Copyright (c) 2021 Travis Geiseblrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "resource.h"

#include <dev/bus/pci.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdint.h>

#define LOCAL_TRACE 0

namespace pci {

resource_range &resource_allocator::type_to_range(pci_resource_type type, bool prefetchable) {
    if (prefetchable) {
        switch (type) {
            case PCI_RESOURCE_MMIO_RANGE:
                return ranges_.mmio_prefetchable;
            case PCI_RESOURCE_MMIO64_RANGE:
                return ranges_.mmio64_prefetchable;
            default:
                DEBUG_ASSERT_MSG(0, "uhandled prefetchable pci resource type %d\n", type);
        }
    } else {
        switch (type) {
            case PCI_RESOURCE_IO_RANGE:
                return ranges_.io;
            case PCI_RESOURCE_MMIO_RANGE:
                return ranges_.mmio;
            case PCI_RESOURCE_MMIO64_RANGE:
                return ranges_.mmio64;
            default:
                DEBUG_ASSERT_MSG(0, "uhandled pci resource type %d\n", type);
        }
    }
    static resource_range zero = {};
    return zero;
}

status_t resource_allocator::set_range(const resource_range &range, bool prefetchable) {
    LTRACEF("range base %#llx size %#llx type %d prefetchable %d\n", range.base, range.size, range.type, prefetchable);
    type_to_range(range.type, prefetchable) = range;
    return NO_ERROR;
}

status_t resource_allocator::allocate_mmio(bool can_be_64bit, bool prefetchable, uint64_t size, uint8_t align, uint64_t *out) {
    pci_resource_type type;

    for (;;) {
        if (can_be_64bit) {
            type = PCI_RESOURCE_MMIO64_RANGE;
        } else {
            type = PCI_RESOURCE_MMIO_RANGE;
        }

        auto &range = type_to_range(type, prefetchable);

        LTRACEF("range base %#llx size %#llx. request size %#llx align %u prefetchable %d can_be_64 %d\n",
                range.base, range.size, size, align, prefetchable, can_be_64bit);

        // TODO: make sure align is honored or removed
        if (range.base + size <= range.base + range.size) {
            *out = range.base;
            range.base += size;
            range.size -= size;
            return NO_ERROR;
        }

        // after trying once to allocate in a 64bit range, drop to 32bit and try again
        if (can_be_64bit) {
            can_be_64bit = false;
            continue;
        }
        break;
    }

    return ERR_NO_RESOURCES;
}

status_t resource_allocator::allocate_io(uint32_t size, uint8_t align, uint32_t *out) {
    auto &range = type_to_range(PCI_RESOURCE_IO_RANGE, false);

    LTRACEF("range base %#llx size %#llx. request size %#x align %u\n", range.base, range.size, size, align);

    // TODO: make sure align is honored or removed
    if (range.base + size <= range.base + range.size) {
        *out = range.base;
        range.base += size;
        range.size -= size;
        return NO_ERROR;
    }

    return ERR_NO_RESOURCES;
}

} // namespace pci
