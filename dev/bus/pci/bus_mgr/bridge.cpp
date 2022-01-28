/*
 * Copyright (c) 2021 Travis Geiseblrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "bridge.h"

#include <sys/types.h>
#include <lk/cpp.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/list.h>
#include <lk/trace.h>
#include <dev/bus/pci.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <platform/interrupts.h>

#define LOCAL_TRACE 0

#include "bus_mgr.h"
#include "bridge.h"
#include "bus.h"

namespace pci {

bridge::bridge(pci_location_t loc, bus *bus) : device(loc, bus) {}
bridge::~bridge() = default;

// examine the bridge device, figuring out the bus range it controls and recurse
status_t bridge::probe(pci_location_t loc, bus *parent_bus, bridge **out_bridge) {
    char str[14];
    LTRACEF("%s\n", pci_loc_string(loc, str));

    *out_bridge = nullptr;

    // read vendor id and see if this is a real device
    uint16_t vendor_id;
    status_t err = pci_read_config_half(loc, PCI_CONFIG_VENDOR_ID, &vendor_id);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }
    if (vendor_id == 0xffff) {
        return ERR_NOT_FOUND;
    }

    // read header type (0 or 1)
    uint8_t header_type;
    err = pci_read_config_byte(loc, PCI_CONFIG_HEADER_TYPE, &header_type);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }

    header_type &= PCI_HEADER_TYPE_MASK;

    if (header_type != 1) {
        LTRACEF("type %d header on bridge we don't understand, skipping\n", header_type);
        return ERR_NOT_FOUND;
    }

    // we are a bridge to a new set of busses
    bridge *br = new bridge(loc, parent_bus);

    // we only grok type 1 headers here
    err = br->load_config();
    if (err < 0) {
        delete br;
        return err;
    }

    LTRACEF("primary bus %hhd secondary %hhd subordinate %hhd\n",
            br->primary_bus(), br->secondary_bus(), br->subordinate_bus());

    // probe the bridge's capabilities
    br->probe_capabilities();

    *out_bridge = br;

    if (br->secondary_bus() == 0) {
        // allocate a new secondary bus
        uint8_t new_secondary_bus = allocate_next_bus();

        // we do not yet know the range of busses we will find downstream, lower level bridges will have to
        // back patch us as they find new children.
        uint8_t new_subordinate_bus = new_secondary_bus;

        LTRACEF("assigning secondary bus %d, parent bus %d\n", new_secondary_bus, parent_bus->bus_num());

        br->assign_bus_numbers(parent_bus->bus_num(), new_secondary_bus, new_subordinate_bus);

        // tell the parent bridge that we have a new range
        auto *parent_bridge = parent_bus->get_bridge();
        if (parent_bridge) {
            parent_bridge->extend_subordinate_range(new_secondary_bus);
        }
    }

    // sanity check that we don't have overlapping busses
    if (br->secondary_bus() < get_last_bus()) {
        TRACEF("secondary bus %u of bridge we've already seen (last bus seen %u)\n", br->secondary_bus(), get_last_bus());
        delete br;
        return ERR_NO_RESOURCES;
    }

    // start a scan of the secondary bus downstream of this.
    // via bridge devices on this bus, should find all of the subordinate busses.
    pci_location_t bus_location = {};
    bus_location.segment = loc.segment;
    bus_location.bus = br->secondary_bus();

    bus *new_bus;
    err = bus::probe(bus_location, br, &new_bus);
    if (err != NO_ERROR) {
        // TODO: don't leak bridge and/or bus
        return err;
    }

    // add the bus to our list of children
    DEBUG_ASSERT(new_bus);
    br->add_bus(new_bus);

    // add the bus to the global bus list
    new_bus->add_to_global_list();

    return NO_ERROR;
}

void bridge::extend_subordinate_range(uint8_t new_secondary_bus) {
    LTRACEF("new_secondary_bus %u existing primary bus %hhd secondary %hhd subordinate %hhd\n",
            new_secondary_bus, primary_bus(), secondary_bus(), subordinate_bus());

    if (new_secondary_bus > subordinate_bus()) {
        assign_bus_numbers(primary_bus(), secondary_bus(), new_secondary_bus);

        DEBUG_ASSERT(subordinate_bus() == new_secondary_bus);

        // tell the parent bridge that we have a new range
        auto *parent_bridge = bus_->get_bridge();
        if (parent_bridge) {
            parent_bridge->extend_subordinate_range(new_secondary_bus);
        }
    }
}

void bridge::assign_bus_numbers(uint8_t primary, uint8_t secondary, uint8_t subordinate) {
    LTRACEF("primary %u secondary %u subordinate %u\n", primary, secondary, subordinate);

    uint32_t temp;

    pci_read_config_word(loc_, 0x18, &temp);
    temp &= 0xff000000; // leave latency timer alone
    temp |= subordinate << 16;
    temp |= secondary << 8;
    temp |= primary << 0;
    pci_write_config_word(loc_, 0x18, temp);

    // reread the config
    load_config();
}

void bridge::dump(size_t indent) {
    auto scoot = [&]() {
        for (size_t i = 0; i < indent; i++) {
            printf(" ");
        }

    };
    char str[14];
    scoot();
    printf("bridge %s %04hx:%04hx primary bus %d child busses [%d..%d]\n", pci_loc_string(loc_, str),
           vendor_id(), device_id(), primary_bus(),
           secondary_bus(), subordinate_bus());

    auto mr = mem_range();
    auto ir = io_range();
    auto pr = prefetch_range();
    scoot();
    printf("mem_range [%#x..%#x] io_range [%#x..%#x] pref_range [%#llx..%#llx] \n",
           mr.base, mr.limit, ir.base, ir.limit, pr.base, pr.limit);

    for (size_t b = 0; b < 2; b++) {
        if (bars_[b].valid) {
            for (size_t i = 0; i < indent + 1; i++) {
                printf(" ");
            }
            printf("BAR %zu: addr %#llx size %#zx io %d valid %d\n", b, bars_[b].addr, bars_[b].size, bars_[b].io, bars_[b].valid);
        }
    }

    if (secondary_bus_) {
        secondary_bus_->dump(indent + 1);
    }
}

// accessors to compute the io and memory range of the bridge
bridge::range<uint32_t> bridge::io_range() {
    if (config_.type1.io_limit < config_.type1.io_base) {
        return { 0, 0 };
    } else {
        // TODO: handle 32bit io (does this really exist?)
        return { ((uint32_t)config_.type1.io_base >> 4) << 12,
                 (((uint32_t)config_.type1.io_limit >> 4) << 12) | 0xfff };
    }
}

bridge::range<uint32_t> bridge::mem_range() {
    if (config_.type1.memory_limit < config_.type1.memory_base) {
        return { 0, 0 };
    } else {
        return { ((uint32_t)config_.type1.memory_base >> 4) << 20,
                 (((uint32_t)config_.type1.memory_limit >> 4) << 20) | 0xfffff };
    }
}

bridge::range<uint64_t> bridge::prefetch_range() {
    if (config_.type1.prefetchable_memory_limit < config_.type1.prefetchable_memory_base) {
        return { 0, 0 };
    } else {
        bool is_64 = (config_.type1.prefetchable_memory_base & 0xf) == 1;

        uint64_t base, limit;

        base = (((uint64_t)config_.type1.prefetchable_memory_base >> 4) << 20);
        if (is_64) {
            base |= (uint64_t)config_.type1.prefetchable_base_upper << 32;
        }
        limit = (((uint64_t)config_.type1.prefetchable_memory_limit >> 4) << 20) | 0xfffff;
        if (is_64) {
            limit |= (uint64_t)config_.type1.prefetchable_limit_upper << 32;
        }
        return { base, limit };
    }
}

} // namespace pci
