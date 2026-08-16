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
#include "resource.h"
#include "root.h"

#define LOCAL_TRACE 0

// global state of the pci bus manager
namespace pci {

// all of the roots (host bridges) in the system, and every bus hanging off of them
list_node root_list = LIST_INITIAL_VALUE(root_list);
list_node bus_list = LIST_INITIAL_VALUE(bus_list);

namespace {

// set once pci_bus_mgr_init has run
bool initialized = false;

// the root created when the platform registers none of its own
root *default_root = nullptr;

// find or create the default root
root *get_default_root() {
    if (default_root) {
        return default_root;
    }

    // covers segment 0 from bus 0 through whatever the config accessors can reach
    pci_root_desc desc = {};
    desc.segment = 0;
    desc.bus_start = 0;
    int last_bus = pci_get_last_bus();
    desc.bus_end = (last_bus < 0) ? 255 : (uint8_t)last_bus;

    default_root = new root(desc);
    list_add_tail(&root_list, &default_root->node);
    return default_root;
}

template <typename F>
status_t for_every_root(F func) {
    status_t err = NO_ERROR;

    root *r;
    list_for_every_entry(&root_list, r, root, node) {
        err = func(r);
        if (err != NO_ERROR) {
            return err;
        }
    }
    return err;
}

// local helper routines
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

template <typename F>
status_t for_every_bus(F func) {
    status_t err = NO_ERROR;

    bus *b;
    list_for_every_entry(&bus_list, b, bus, node) {
        err = func(b);
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

// used by bus object to stuff itself into a global list
void add_to_bus_list(bus *b) {
    list_add_tail(&bus_list, b->list_node_ptr());
}

// find a bus by segment and number
bus *lookup_bus(uint16_t segment, uint8_t bus_num) {
    bus *found_bus = nullptr;
    auto b_finder = [&](bus *b) -> status_t {
        if (segment == b->loc().segment && bus_num == b->bus_num()) {
            found_bus = b;
            return 1;
        }
        return 0;
    };

    for_every_bus(b_finder);
    return found_bus;
}

} // namespace pci

// C api, so outside of the namespace
using namespace pci;

status_t pci_bus_mgr_add_root(const struct pci_root_desc *desc) {
    if (!desc || desc->bus_end < desc->bus_start) {
        return ERR_INVALID_ARGS;
    }
    if (initialized) {
        return ERR_BAD_STATE;
    }

    LTRACEF("segment %u bus [%u...%u] %zu windows\n", desc->segment, desc->bus_start,
            desc->bus_end, desc->num_windows);

    // reject roots that overlap an existing one
    root *r;
    list_for_every_entry(&root_list, r, root, node) {
        if (r->segment() == desc->segment && desc->bus_start <= r->bus_end() &&
            desc->bus_end >= r->bus_start()) {
            printf("PCI: root %04x:[%02x...%02x] overlaps an existing root\n", desc->segment,
                   desc->bus_start, desc->bus_end);
            return ERR_ALREADY_EXISTS;
        }
    }

    r = new root(*desc);
    list_add_tail(&root_list, &r->node);
    return NO_ERROR;
}

status_t pci_bus_mgr_add_resource(enum pci_resource_type type, uint64_t mmio_base, uint64_t len) {
    LTRACEF("type %d: mmio base %#llx len %#llx\n", type, mmio_base, len);

    if (initialized) {
        return ERR_BAD_STATE;
    }

    pci_root_window w = {};
    w.type = type;
    w.base = mmio_base;
    w.size = len;
    return get_default_root()->add_window(w);
}

status_t pci_bus_mgr_init() {
    LTRACE_ENTRY;

    if (initialized) {
        return ERR_BAD_STATE;
    }

    // if the platform didn't register any roots, make one up covering segment 0
    if (list_is_empty(&root_list)) {
        get_default_root();
    }

    // scan every root, keep going if one of them fails
    status_t final_err = NO_ERROR;
    for_every_root([&](root *r) -> status_t {
        status_t err = r->probe();
        if (err != NO_ERROR) {
            final_err = err;
        }
        return NO_ERROR;
    });

    initialized = true;

    // iterate over all the devices found
    if (LK_DEBUGLEVEL >= SPEW) {
        printf("PCI dump:\n");
        for_every_root([](root *r) -> status_t {
            r->dump(1);
            return NO_ERROR;
        });
    }

    if (LOCAL_TRACE) {
        printf("visit all devices\n");
        pci_bus_mgr_visit_devices([](pci_location_t _loc) {
            char str[14];
            printf("%s\n", pci_loc_string(_loc, str));
        });
    }

    return final_err;
}

status_t pci_bus_mgr_assign_resources() {
    LTRACE_ENTRY;

    if (!initialized) {
        return ERR_NOT_READY;
    }

    status_t final_err = NO_ERROR;
    for_every_root([&](root *r) -> status_t {
        status_t err = r->assign_resources();
        if (err != NO_ERROR) {
            printf("PCI: error %d assigning resources to devices on root %04x:%02x\n", err,
                   r->segment(), r->bus_start());
            final_err = err;
        }
        return NO_ERROR;
    });

    // iterate over all the devices found
    if (LK_DEBUGLEVEL >= SPEW) {
        printf("PCI dump post assign:\n");
        for_every_root([](root *r) -> status_t {
            r->dump(1);
            return NO_ERROR;
        });
    }

    return final_err;
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

    return d->enable();
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

status_t pci_bus_mgr_read_interrupt_line(const pci_location_t loc, uint8_t *interrupt_line) {
    char str[14];
    LTRACEF("%s\n", pci_loc_string(loc, str));

    if (!interrupt_line) {
        return ERR_INVALID_ARGS;
    }

    device *d = lookup_device_by_loc(loc);
    if (!d) {
        return ERR_NOT_FOUND;
    }

    *interrupt_line = d->interrupt_line();
    return NO_ERROR;
}

bool pci_bus_mgr_has_msi(const pci_location_t loc) {
    char str[14];
    LTRACEF("%s\n", pci_loc_string(loc, str));

    device *d = lookup_device_by_loc(loc);
    if (!d) {
        return false;
    }

    return d->has_msi();
}

bool pci_bus_mgr_has_msix(const pci_location_t loc) {
    char str[14];
    LTRACEF("%s\n", pci_loc_string(loc, str));

    device *d = lookup_device_by_loc(loc);
    if (!d) {
        return false;
    }

    return d->has_msix();
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

status_t pci_bus_mgr_allocate_msix(const pci_location_t loc, size_t num_requested, uint *irqbase) {
    char str[14];
    LTRACEF("%s num_request %zu\n", pci_loc_string(loc, str), num_requested);

    *irqbase = 0;

    device *d = lookup_device_by_loc(loc);
    if (!d) {
        return ERR_NOT_FOUND;
    }

    if (!d->has_msix()) {
        return ERR_NO_RESOURCES;
    }

    return d->allocate_msix(num_requested, irqbase);
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

ssize_t pci_read_vendor_capability(const pci_location_t loc, size_t index, void *buf, size_t buflen) {
    char str[14];
    LTRACEF("%s\n", pci_loc_string(loc, str));

    device *d = lookup_device_by_loc(loc);
    if (!d) {
        return ERR_NOT_FOUND;
    }

    return d->read_vendor_capability(index, buf, buflen);
}

void pci_dump_bar(const pci_bar_t *bar, int index) {
    if (bar->addr >= UINT32_MAX || bar->size >= UINT32_MAX) {
        printf("BAR %d: addr %-#16llx size %-#16zx io %d 64b %d pref %d\n",
                index, bar->addr, bar->size, bar->io, bar->size_64, bar->prefetchable);
    } else {
        printf("BAR %d: addr %-#8llx size %-#8zx io %d 64b %d pref %d\n",
                index, bar->addr, bar->size, bar->io, bar->size_64, bar->prefetchable);
    }
}

void pci_dump_bars(pci_bar_t bar[6], size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (bar[i].valid) {
            pci_dump_bar(bar + i, i);
        }
    }
}

const char *pci_loc_string(pci_location_t loc, char out_str[14]) {
    snprintf(out_str, 14, "%04x:%02x:%02x.%1x", loc.segment, loc.bus, loc.dev, loc.fn);
    return out_str;
}

const char *pci_resource_type_to_str(enum pci_resource_type type) {
    switch (type) {
        case PCI_RESOURCE_IO_RANGE:
            return "io";
        case PCI_RESOURCE_MMIO_RANGE:
            return "mmio";
        case PCI_RESOURCE_MMIO64_RANGE:
            return "mmio64";
    }
    return "unknown";
}

