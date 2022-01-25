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

#include "device.h"

namespace pci {

class bus;

// bridge device, holds a list of busses that it is responsible for
class bridge : public device {
public:
    bridge(pci_location_t loc, bus *bus);
    ~bridge() override;

    DISALLOW_COPY_ASSIGN_AND_MOVE(bridge);

    static status_t probe(pci_location_t loc, bus *bus, bridge **out_bridge);

    void add_bus(bus *b) { secondary_bus_ = b; }

    void dump(size_t indent = 0) override;

private:
    bus *secondary_bus_ = nullptr;
};

} // pci

