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

namespace pci {

class bus;
struct capability;

// generic pci device
class device {
public:
    device(pci_location_t loc, bus *bus);
    virtual ~device();

    DISALLOW_COPY_ASSIGN_AND_MOVE(device);

    static status_t probe(pci_location_t loc, bus *bus, device **out_device);

    status_t probe_capabilities();
    status_t init_msi_capability(capability *cap);
    status_t init_msix_capability(capability *cap);

    status_t allocate_irq(uint *irq);
    status_t allocate_msi(size_t num_requested, uint *msi_base);
    status_t load_config();
    status_t load_bars();

    pci_location_t loc() const { return loc_; }
    const bus *get_bus() const { return bus_; }

    uint16_t device_id() const { return config_.device_id; }
    uint16_t vendor_id() const { return config_.vendor_id; }
    uint8_t base_class() const { return config_.base_class; }
    uint8_t sub_class() const { return config_.sub_class; }
    uint8_t interface() const { return config_.program_interface; }
    uint8_t header_type() const { return config_.header_type & PCI_HEADER_TYPE_MASK; }

    status_t read_bars(pci_bar_t bar[6]);

    bool has_msi() const { return msi_cap_; }
    bool has_msix() const { return msix_cap_; }

    virtual void dump(size_t indent = 0);

protected:
    // let the bus device directly manipulate our list node
    friend class bus;
    list_node node = LIST_INITIAL_CLEARED_VALUE;

    pci_location_t loc_ = {};
    bus *bus_ = nullptr;

    pci_config_t config_ = {};
    pci_bar_t bars_[6] = {};

    // capability list
    list_node capability_list_ = LIST_INITIAL_VALUE(capability_list_);
    capability *msi_cap_ = nullptr;
    capability *msix_cap_ = nullptr;
};

struct capability {
    list_node node = LIST_INITIAL_CLEARED_VALUE;
    uint16_t config_offset = 0;
    uint16_t id = 0;

    // simple accessors
    bool is_msi() const { return id == 0x5; }
    bool is_msix() const { return id == 0x11; }
};

} // pci
