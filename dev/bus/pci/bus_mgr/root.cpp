/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "root.h"

#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <stdio.h>

#include "bus.h"
#include "bus_mgr.h"

#define LOCAL_TRACE 0

namespace pci {

root::root(const pci_root_desc &desc) : desc_(desc), highest_bus_(desc.bus_start) {
    // the window array belongs to the caller, we copy it into the allocator
    desc_.windows = nullptr;
    desc_.num_windows = 0;

    for (size_t i = 0; i < desc.num_windows; i++) {
        add_window(desc.windows[i]);
    }
}

root::~root() = default;

status_t root::add_window(const pci_root_window &window) {
    LTRACEF("segment %u: type %s base %#llx size %#llx translation %#llx prefetchable %d\n",
            segment(), pci_resource_type_to_str(window.type), window.base, window.size,
            window.translation_offset, window.prefetchable);

    if (window.size == 0) {
        return ERR_INVALID_ARGS;
    }

    // TODO: carry the translation offset through to the devices that map the BARs
    return allocator_.add_range(window.type, window.prefetchable, window.base, window.size);
}

void root::note_bus_seen(uint8_t bus) {
    LTRACEF("bus %u, existing highest %u\n", bus, highest_bus_);
    if (bus > highest_bus_) {
        highest_bus_ = bus;
    }
}

status_t root::allocate_next_bus(uint8_t *out) {
    if (highest_bus_ >= bus_end()) {
        return ERR_NO_RESOURCES;
    }
    *out = ++highest_bus_;
    return NO_ERROR;
}

status_t root::probe() {
    LTRACEF("segment %u bus [%u...%u]\n", segment(), bus_start(), bus_end());

    DEBUG_ASSERT(!bus_);

    pci_location_t loc = {};
    loc.segment = segment();
    loc.bus = bus_start();

    bus *b;
    status_t err = bus::probe(loc, nullptr, this, &b);
    if (err < 0) {
        printf("PCI: failed to probe root %04x:%02x, error %d\n", segment(), bus_start(), err);
        return err;
    }

    // if we found anything there should be at least an empty bus device
    DEBUG_ASSERT(b);
    bus_ = b;
    b->add_to_global_list();

    return NO_ERROR;
}

status_t root::assign_resources(pci_assign_mode mode) {
    if (!bus_) {
        return ERR_NOT_READY;
    }

    if (LOCAL_TRACE) {
        allocator_.dump();
    }

    return bus_->allocate_resources(allocator_, mode);
}

status_t root::route_intx(pci_location_t root_level_loc, unsigned int pin, unsigned int *vector) {
    if (!desc_.intx_route) {
        return ERR_NOT_SUPPORTED;
    }
    return desc_.intx_route(desc_.intx_cookie, root_level_loc, pin, vector);
}

void root::dump(size_t indent) {
    for (size_t i = 0; i < indent; i++) {
        printf(" ");
    }
    printf("root segment %04x bus [%02x...%02x]\n", segment(), bus_start(), bus_end());
    if (bus_) {
        bus_->dump(indent + 1);
    }
}

} // namespace pci
