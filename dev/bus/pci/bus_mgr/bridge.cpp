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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <platform/interrupts.h>

#define LOCAL_TRACE 0

#include "bus_mgr.h"
#include "bridge.h"
#include "bus.h"
#include "resource.h"

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
        auto *parent_bridge = parent_bus()->get_bridge();
        if (parent_bridge) {
            parent_bridge->extend_subordinate_range(new_secondary_bus);
        }
    }
}

void bridge::assign_bus_numbers(uint8_t primary, uint8_t secondary, uint8_t subordinate) {
    LTRACEF("primary %u secondary %u subordinate %u\n", primary, secondary, subordinate);

    uint32_t temp;

    pci_read_config_word(loc(), 0x18, &temp);
    temp &= 0xff000000; // leave latency timer alone
    temp |= subordinate << 16;
    temp |= secondary << 8;
    temp |= primary << 0;
    pci_write_config_word(loc(), 0x18, temp);

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
    printf("bridge %s %04hx:%04hx primary bus %d child busses [%d..%d]\n", pci_loc_string(loc(), str),
           vendor_id(), device_id(), primary_bus(),
           secondary_bus(), subordinate_bus());

    auto mr = mem_range();
    auto ir = io_range();
    auto pr = prefetch_range();
    scoot();
    printf("mem_range [%#x..%#x] io_range [%#x..%#x] pref_range [%#" PRIx64 "..%#" PRIx64"] \n",
           mr.base, mr.limit, ir.base, ir.limit, pr.base, pr.limit);

    for (size_t b = 0; b < 2; b++) {
        if (bar(b).valid) {
            for (size_t i = 0; i < indent + 1; i++) {
                printf(" ");
            }
            printf("BAR %zu: addr %#" PRIx64 " size %#zx io %d valid %d\n", b, bar(b).addr, bar(b).size, bar(b).io, bar(b).valid);
        }
    }

    if (secondary_bus_) {
        secondary_bus_->dump(indent + 1);
    }
}

// accessors to compute the io and memory range of the bridge
bridge::range<uint32_t> bridge::io_range() {
    if (config().type1.io_limit < config().type1.io_base) {
        return { 0, 0 };
    } else {
        // TODO: handle 32bit io (does this really exist?)
        return { ((uint32_t)config().type1.io_base >> 4) << 12,
                 (((uint32_t)config().type1.io_limit >> 4) << 12) | 0xfff };
    }
}

bridge::range<uint32_t> bridge::mem_range() {
    if (config().type1.memory_limit < config().type1.memory_base) {
        return { 0, 0 };
    } else {
        return { ((uint32_t)config().type1.memory_base >> 4) << 20,
                 (((uint32_t)config().type1.memory_limit >> 4) << 20) | 0xfffff };
    }
}

bridge::range<uint64_t> bridge::prefetch_range() {
    if (config().type1.prefetchable_memory_limit < config().type1.prefetchable_memory_base) {
        return { 0, 0 };
    } else {
        bool is_64 = (config().type1.prefetchable_memory_base & 0xf) == 1;

        uint64_t base, limit;

        base = (((uint64_t)config().type1.prefetchable_memory_base >> 4) << 20);
        if (is_64) {
            base |= (uint64_t)config().type1.prefetchable_base_upper << 32;
        }
        limit = (((uint64_t)config().type1.prefetchable_memory_limit >> 4) << 20) | 0xfffff;
        if (is_64) {
            limit |= (uint64_t)config().type1.prefetchable_limit_upper << 32;
        }
        return { base, limit };
    }
}

status_t bridge::compute_bar_sizes_no_local_bar(bar_sizes *bridge_sizes) {
    bar_sizes bus_sizes = {};

    // drill into our bus and accumulate from there
    auto perdev = [&](device *d) -> status_t {
        device::bar_sizes sizes = {};

        d->compute_bar_sizes(&sizes);

        if (LOCAL_TRACE) {
            char str[14];
            printf("bar sizes for device %s: io %#x align %u mmio %#x align %u mmio64 %#" PRIx64 " align %u prefetch %#" PRIx64 " align %u prefetch64 %#" PRIx64 " align %u\n",
                    pci_loc_string(d->loc(), str), sizes.io_size, sizes.io_align, sizes.mmio_size, sizes.mmio_align,
                    sizes.mmio64_size, sizes.mmio64_align, sizes.prefetchable_size, sizes.prefetchable_align,
                    sizes.prefetchable64_size, sizes.prefetchable64_align);
        }

        // accumulate to the passed in size struct
        bus_sizes += sizes;

        return 0;
    };

    LTRACEF("recursing into bus %u\n", secondary_bus_->bus_num());
    secondary_bus_->for_every_device(perdev);

    // we've accumulated the size of the bus and everything below it
    // round up some of the accumulated sizes based on limitations in the bridge

    // bridges can only pass through io ports in blocks of 0x1000
    if (bus_sizes.io_size > 0 && bus_sizes.io_align < 12) {
        bus_sizes.io_align = 12;
    }
    bus_sizes.io_size = ROUNDUP(bus_sizes.io_size, 0x1000);

    // mmio ranges can only pass through in units of 1MB (1<<20)
    if (bus_sizes.mmio_size > 0 && bus_sizes.mmio_align < 20) {
        bus_sizes.mmio_align = 20;
    }
    bus_sizes.mmio_size = ROUNDUP(bus_sizes.mmio_size, (1UL << 20));

    // 64bit mmio ranges can't be passed through bridges, so merge with the mmio range
    if (bus_sizes.mmio64_size > 0 && bus_sizes.mmio_align < bus_sizes.mmio64_align) {
        bus_sizes.mmio_align = bus_sizes.mmio64_align;
    }
    bus_sizes.mmio_size += ROUNDUP(bus_sizes.mmio64_size, (1UL << 20));
    bus_sizes.mmio64_align = 0;
    bus_sizes.mmio64_size = 0;

    // prefetchable ranges are sent through like anything else
    if (bus_sizes.prefetchable_size > 0 && bus_sizes.prefetchable_align < 20) {
        bus_sizes.prefetchable_align = 20;
    }
    bus_sizes.prefetchable_size = ROUNDUP(bus_sizes.prefetchable_size, (1UL << 20));

    // 64bit prefetchable ranges are sent through like anything else
    if (bus_sizes.prefetchable64_size > 0 && bus_sizes.prefetchable64_align < 20) {
        bus_sizes.prefetchable64_align = 20;
    }
    bus_sizes.prefetchable64_size = ROUNDUP(bus_sizes.prefetchable64_size, (1UL << 20));

    *bridge_sizes += bus_sizes;

    return NO_ERROR;
}

status_t bridge::compute_bar_sizes(bar_sizes *bridge_sizes) {
    char str[14];
    LTRACEF("bridge at %s\n", pci_loc_string(loc(), str));

    // accumulate our BARs
    device::compute_bar_sizes(bridge_sizes);

    return compute_bar_sizes_no_local_bar(bridge_sizes);
}

status_t bridge::get_bar_alloc_requests(list_node *bar_alloc_requests) {
    char str[14];
    LTRACEF("bridge at %s\n", pci_loc_string(loc(), str));

    // add our local bars to the list of requests
    device::get_bar_alloc_requests(bar_alloc_requests);

    // accumulate the size of our children
    bar_sizes bus_sizes = {};
    compute_bar_sizes_no_local_bar(&bus_sizes);

    // TODO: test if bridge supports prefetchable and if so if it supports 64bit

    // construct a request out of the different ranges returned
    auto make_request = [this, &bar_alloc_requests](pci_resource_type type, bool prefetchable, uint64_t size, uint8_t align) {
        if (size > 0) {
            auto request = new bar_alloc_request;
            *request = {};
            request->dev = this;
            request->bridge = true;
            request->type = type;
            request->prefetchable = prefetchable;
            request->size = size;
            request->align = align;

            list_add_tail(bar_alloc_requests, &request->node);
        }
    };

    make_request(PCI_RESOURCE_IO_RANGE, false, bus_sizes.io_size, bus_sizes.io_align);
    make_request(PCI_RESOURCE_MMIO_RANGE, false, bus_sizes.mmio_size, bus_sizes.mmio_align);
    make_request(PCI_RESOURCE_MMIO64_RANGE, false, bus_sizes.mmio64_size, bus_sizes.mmio64_align);


    // TODO: merge prefetchable 64 and 32bit?
    // should only be able to deal with one or the other
    DEBUG_ASSERT(!(bus_sizes.prefetchable_size && bus_sizes.prefetchable64_size));
    make_request(PCI_RESOURCE_MMIO_RANGE, true, bus_sizes.prefetchable_size, bus_sizes.prefetchable_align);
    make_request(PCI_RESOURCE_MMIO64_RANGE, true, bus_sizes.prefetchable64_size, bus_sizes.prefetchable64_align);

    return NO_ERROR;
}

status_t bridge::assign_resource(bar_alloc_request *request, uint64_t address) {
    char str[14];
    LTRACEF("bridge at %s resource addr %#" PRIx64 " request:\n", pci_loc_string(loc(), str), address);
    if (LOCAL_TRACE) {
        request->dump();
    }

    if (!request->bridge) {
        // this is a request for one of our bars, pass it to the device base class virtual
        return device::assign_resource(request, address);
    }

    DEBUG_ASSERT(IS_ALIGNED(address, (1UL << request->align)));

    // this is an allocation for one of the bridge resources
    uint32_t temp;
    switch (request->type) {
        case PCI_RESOURCE_IO_RANGE:
            // write to the io configuration bits
            DEBUG_ASSERT(IS_ALIGNED(address, (1UL << 12)));
            temp = ((address >> 8) & 0xf0); // io base
            temp |= (((address + request->size - 1) >> 8) & 0xf0) << 8;
            pci_write_config_word(loc(), 0x1c, temp);
            break;
        case PCI_RESOURCE_MMIO_RANGE:
            DEBUG_ASSERT(IS_ALIGNED(address, (1UL << 20)));
            DEBUG_ASSERT(IS_ALIGNED(request->size, (1UL << 20)));
            if (request->prefetchable) {
                temp = ((address >> 16) & 0xfff0); // mmio base
                temp |= ((address + request->size - 1) >> 16 & 0xfff0) << 16;
                pci_write_config_word(loc(), 0x24, temp);
            } else {
                // non prefetchable mmio range
                temp = ((address >> 16) & 0xfff0); // mmio base
                temp |= ((address + request->size - 1) >> 16 & 0xfff0) << 16;
                pci_write_config_word(loc(), 0x20, temp);
            }
            break;
        case PCI_RESOURCE_MMIO64_RANGE:
            if (!request->prefetchable) {
                // cannot handle non prefetchable 64bit due to the way the bus works
                printf("PCI bridge at %s: invalid 64bit non prefetchable range\n", pci_loc_string(loc(), str));
                return ERR_NOT_SUPPORTED;
            }
            // assert that the device supports 64bit addresses
            DEBUG_ASSERT((config().type1.prefetchable_memory_base & 0xf) == 1);

            DEBUG_ASSERT(IS_ALIGNED(address, (1UL << 20)));
            DEBUG_ASSERT(IS_ALIGNED(request->size, (1UL << 20)));

            // bottom part of prefetchable base and limit
            temp = ((address >> 16) & 0xfff0); // mmio base
            temp |= ((address + request->size - 1) >> 16 & 0xfff0) << 16;
            pci_write_config_word(loc(), 0x24, temp);

            // high part of base and limit
            temp = (address >> 32);
            pci_write_config_word(loc(), 0x28, temp);
            temp = (address + request->size - 1) >> 32;
            pci_write_config_word(loc(), 0x2c, temp);
            break;
        default:
            PANIC_UNIMPLEMENTED;
    }
    load_config();

    // PROBLEM: do we recurse here and start the bus allocation for the children with a restricted resource allocator
    // but we have only a subset of resources to allocate. Or do we wait until the bridge is completely configured,
    // and then trigger a recursive assignment on each sub bus (this is probably the right strategy).


    return NO_ERROR;
}

status_t bridge::assign_child_resources() {
    char str[14];
    LTRACEF("bridge at %s\n", pci_loc_string(loc(), str));

    // construct a resource allocator that covers what we've been assigned
    resource_allocator allocator;

    auto io = io_range();
    if (io.limit > io.base) {
        resource_range r;
        r.base = io.base;
        r.size = io.limit + io.base + 1;
        r.type = PCI_RESOURCE_IO_RANGE;
        allocator.set_range(r);
    }

    auto mmio = mem_range();
    if (mmio.limit > mmio.base) {
        resource_range r;
        r.base = mmio.base;
        r.size = mmio.limit - mmio.base + 1;
        r.type = PCI_RESOURCE_MMIO_RANGE;
        allocator.set_range(r);
    }

    auto pref = prefetch_range();
    if (pref.limit > pref.base) {
        resource_range r;
        r.base = pref.base;
        r.size = pref.limit - pref.base + 1;

        // if the prefetch window is completely < 4GB, set it up as a 32bit mmio prefetchable
        if (pref.base < (1ULL << 32) || (pref.base + pref.limit < (1ULL << 32))) {
            r.type = PCI_RESOURCE_MMIO_RANGE;
        } else {
            r.type = PCI_RESOURCE_MMIO64_RANGE;
        }

        allocator.set_range(r, true);
    }

    // recurse into the secondary bus with the allocator we've set up for it
    secondary_bus_->allocate_resources(allocator);

    // enable the bridge
    enable();

    return NO_ERROR;
}


} // namespace pci
