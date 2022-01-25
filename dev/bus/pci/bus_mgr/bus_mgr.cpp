/*
 * Copyright (c) 2021 Travis Geiseblrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "bus_mgr.h"

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

#include "device.h"
#include "bus.h"
#include "bridge.h"

#define LOCAL_TRACE 0

// root of the pci bus
namespace pci {
bus *root = nullptr;
list_node bus_list = LIST_INITIAL_VALUE(bus_list);

// used by bus object to stuff itself into a global list
void add_to_bus_list(bus *b) {
    list_add_tail(&bus_list, b->list_node_ptr());
}

namespace {

// helper routines
// iterate all devices on all busses with the functor
template <typename F>
status_t for_every_device_on_every_bus(F func) {
    status_t err = NO_ERROR;

    bus *b;
    list_for_every_entry(&bus_list, b, bus, node) {
        err = b->for_every_device(func);
        if (err != NO_ERROR) {
            return err;
        }
    }
    return err;
}

// return a pointer to the device that matches a particular location
device *lookup_device_by_loc(pci_location_t loc) {
    device *ret = nullptr;

    auto v = [&](device *d) -> status_t {
        if (d->loc() == loc) {
            ret = d;
            return 1;
        }
        return 0;
    };

    for_every_device_on_every_bus(v);

    return ret;
}

} // namespace
} // namespace pci

// C api, so outside of the namespace
using namespace pci;

status_t pci_bus_mgr_init() {
    LTRACE_ENTRY;

    // start drilling into the pci bus tree
    pci_location_t loc;

    loc = {}; // start at 0:0:0.0

    bus *b;
    // TODO: deal with root bus not having reference to bridge device
    status_t err = bus::probe(loc, nullptr, &b);
    if (err < 0) {
        return err;
    }

    // if we found anything there should be at least an empty bus device
    DEBUG_ASSERT(b);
    root = b;
    list_add_tail(&bus_list, b->list_node_ptr());

    // iterate over all the devices found
    printf("PCI dump:\n");
    root->dump(2);

#if 0
    printf("visit all devices\n");
    pci_bus_mgr_visit_devices([](pci_location_t _loc) {
        char str[14];
        printf("%s\n", pci_loc_string(_loc, str));
    });
#endif

    return NO_ERROR;
}

// for every bus in the system, pass the visit routine to the device
status_t pci_bus_mgr_visit_devices(pci_visit_routine routine, void *cookie) {
    auto v = [&](device *d) -> status_t {
        routine(d->loc(), cookie);
        return NO_ERROR;
    };

    return for_every_device_on_every_bus(v);
}

status_t pci_bus_mgr_find_device(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, size_t index) {
    LTRACEF("device_id dev %#hx vendor %#hx index %zu\n", device_id, vendor_id, index);

    if (device_id == 0xffff && vendor_id == 0xffff) {
        return ERR_INVALID_ARGS;
    }

    auto v = [&](device *d) -> status_t {

        if (device_id != 0xffff && device_id != d->device_id())
            return NO_ERROR;
        if (vendor_id != 0xffff && vendor_id != d->vendor_id())
            return NO_ERROR;

        if (index-- == 0) {
            char str[14];
            LTRACEF_LEVEL(2, "match at loc %s: device id %#hx vendor id %#hx\n", pci_loc_string(d->loc(), str), d->device_id(), d->vendor_id());
            *state = d->loc();
            return 1; // signals stop
        }
        return NO_ERROR;
    };

    status_t err = for_every_device_on_every_bus(v);
    return (err > 0) ? NO_ERROR : ERR_NOT_FOUND;
}

status_t pci_bus_mgr_find_device_by_class(pci_location_t *state, uint8_t base_class, uint8_t sub_class, uint8_t interface, size_t index) {
    LTRACEF("class %#x sub %#x interface %#x index %zu\n", base_class, sub_class, interface, index);

    if (sub_class == 0xff && interface == 0xff) {
        return ERR_INVALID_ARGS;
    }

    auto v = [&](device *d) -> status_t {
        //LTRACEF_LEVEL(2, "class %#x\n", d->base_class());

        if (base_class != d->base_class())
            return NO_ERROR;
        if (sub_class != 0xff && sub_class != d->sub_class())
            return NO_ERROR;
        if (interface != 0xff && interface != d->interface())
            return NO_ERROR;

        if (index-- == 0) {
            char str[14];
            LTRACEF_LEVEL(2, "match at loc %s: class %#hhx sub %#hhx interface %hhx\n",
                    pci_loc_string(d->loc(), str), d->base_class(), d->sub_class(), d->interface());
            *state = d->loc();
            return 1; // signals stop
        }
        return NO_ERROR;
    };

    status_t err = for_every_device_on_every_bus(v);
    return (err > 0) ? NO_ERROR : ERR_NOT_FOUND;
}

status_t pci_bus_mgr_enable_device(const pci_location_t loc) {
    char str[14];
    LTRACEF("%s\n", pci_loc_string(loc, str));

    device *d = lookup_device_by_loc(loc);
    if (!d) {
        return ERR_NOT_FOUND;
    }

    uint16_t command;
    status_t err = pci_read_config_half(loc, PCI_CONFIG_COMMAND, &command);
    if (err != NO_ERROR) return err;
    LTRACEF("command reg %#x\n", command);
    command |= PCI_COMMAND_IO_EN | PCI_COMMAND_MEM_EN | PCI_COMMAND_BUS_MASTER_EN;
    err = pci_write_config_half(loc, PCI_CONFIG_COMMAND, command);
    if (err != NO_ERROR) return err;

    return NO_ERROR;
}

status_t pci_bus_mgr_read_bars(const pci_location_t loc, pci_bar_t bar[6]) {
    char str[14];
    LTRACEF("%s\n", pci_loc_string(loc, str));

    device *d = lookup_device_by_loc(loc);
    if (!d) {
        return ERR_NOT_FOUND;
    }

    return d->read_bars(bar);
}

status_t pci_bus_mgr_allocate_msi(const pci_location_t loc, size_t num_requested, uint *irqbase) {
    char str[14];
    LTRACEF("%s num_request %zu\n", pci_loc_string(loc, str), num_requested);

    *irqbase = 0;

    device *d = lookup_device_by_loc(loc);
    if (!d) {
        return ERR_NOT_FOUND;
    }

    if (!d->has_msi()) {
        return ERR_NO_RESOURCES;
    }

    return d->allocate_msi(num_requested, irqbase);
}

status_t pci_bus_mgr_allocate_irq(const pci_location_t loc, uint *irqbase) {
    char str[14];
    LTRACEF("%s\n", pci_loc_string(loc, str));

    *irqbase = 0;

    device *d = lookup_device_by_loc(loc);
    if (!d) {
        return ERR_NOT_FOUND;
    }

    return d->allocate_irq(irqbase);
}

void pci_dump_bars(pci_bar_t bar[6], size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (bar[i].valid) {
            printf("BAR %zu: addr %#16llx size %#16zx io %d\n",
                    i, bar[i].addr, bar[i].size, bar[i].io);
        }
    }
}

const char *pci_loc_string(pci_location_t loc, char out_str[14]) {
    snprintf(out_str, 14, "%04x:%02x:%02x.%1x", loc.segment, loc.bus, loc.dev, loc.fn);
    return out_str;
}

