/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <lk/cpp.h>
#include <lk/err.h>
#include <lk/list.h>
#include <dev/bus/pci.h>

#include "resource.h"

namespace pci {

class bus;

// A PCI root: a host bridge with a segment, a range of bus numbers it may hand out, a set of
// address windows it decodes and an optional legacy interrupt routing hook. Every probed bus
// belongs to exactly one root. Platforms register roots with pci_bus_mgr_add_root(); if none
// are registered a default root covering segment 0 is created at init time.
class root {
public:
    explicit root(const pci_root_desc &desc);
    ~root();

    DISALLOW_COPY_ASSIGN_AND_MOVE(root);

    // scan the bus tree starting at bus_start
    status_t probe();

    // hand out resources to the devices under this root
    status_t assign_resources(pci_assign_mode mode);

    // add a window this root decodes
    status_t add_window(const pci_root_window &window);

    // bus number tracking, used while probing
    void note_bus_seen(uint8_t bus);
    uint8_t highest_bus() const { return highest_bus_; }
    status_t allocate_next_bus(uint8_t *out);

    // route a legacy interrupt pin (1..4) of the device at the end of path
    bool has_intx_route() const { return desc_.intx_route != nullptr; }
    status_t route_intx(const pci_intx_path_entry *path, size_t path_len, unsigned int pin,
                        unsigned int *vector);

    uint16_t segment() const { return desc_.segment; }
    uint8_t bus_start() const { return desc_.bus_start; }
    uint8_t bus_end() const { return desc_.bus_end; }
    bus *root_bus() const { return bus_; }
    resource_allocator &allocator() { return allocator_; }

    void dump(size_t indent = 0);

    // master list of roots
    list_node node = LIST_INITIAL_CLEARED_VALUE;

private:
    pci_root_desc desc_;
    bus *bus_ = nullptr;
    uint8_t highest_bus_;
    resource_allocator allocator_{true};
};

} // namespace pci
