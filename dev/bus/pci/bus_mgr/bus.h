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

// bus device holds a list of devices and a reference to its bridge device
class bus {
public:
    bus(pci_location_t loc, bridge *b);
    ~bus() = default;

    DISALLOW_COPY_ASSIGN_AND_MOVE(bus);

    static status_t probe(pci_location_t loc, bridge *bridge, bus **out_bus);

    pci_location_t loc() const { return loc_; }
    uint bus_num() const { return loc().bus; }

    void add_device(device *d);

    void dump(size_t indent = 0);

    list_node *list_node_ptr() { return &node; }

    template <typename F>
    status_t for_every_device(F func);

    void add_to_global_list();

    // master list of busses for easy iteration
    list_node node = LIST_INITIAL_CLEARED_VALUE;

private:
    pci_location_t loc_ = {};
    bridge *b_ = nullptr;
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
