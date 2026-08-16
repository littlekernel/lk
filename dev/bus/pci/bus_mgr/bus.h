/*
 * Copyright (c) 2021 Travis Geiseblrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <dev/bus/pci.h>
#include <lk/cpp.h>
#include <lk/err.h>

#include "device.h"
#include "bridge.h"

namespace pci {

class resource_allocator;
class root;

// bus device holds a list of devices and a reference to its bridge device
class bus {
public:
    bus(pci_location_t loc, bridge *b, root *r);
    ~bus() = default;

    DISALLOW_COPY_ASSIGN_AND_MOVE(bus);

    // probe the bus at loc. bridge is the upstream bridge, or nullptr for the root bus of r.
    static status_t probe(pci_location_t loc, bridge *bridge, root *r, bus **out_bus);

    // allocate resources for devices on this bus and recursively all of its children
    status_t allocate_resources(resource_allocator &allocator, pci_assign_mode mode);

    pci_location_t loc() const { return loc_; }
    uint bus_num() const { return loc().bus; }

    void add_device(device *d);
    void dump(size_t indent = 0);

    const bridge *get_bridge() const { return b_; }
    bridge *get_bridge() { return b_; }
    root *get_root() const { return root_; }
    bool is_root_bus() const { return b_ == nullptr; }

    template <typename F>
    status_t for_every_device(F func);

    void add_to_global_list();

    // master list of busses for easy iteration
    list_node node = LIST_INITIAL_CLEARED_VALUE;
    list_node *list_node_ptr() { return &node; }

private:
    pci_location_t loc_ = {};
    bridge *b_ = nullptr;   // upstream bridge, nullptr for a root bus
    root *root_ = nullptr;  // the root this bus hangs off of
    list_node child_devices_ = LIST_INITIAL_VALUE(child_devices_);
};

// call the provided functor on every device in this bus
template <typename F>
inline status_t bus::for_every_device(F func) {
    status_t err = NO_ERROR;

    device *d;
    list_for_every_entry(&child_devices_, d, device, node) {
        err = func(d);
        if (err != NO_ERROR) {
            return err;
        }
    }
    return err;
}

} // namespace pci
