/*
 * Copyright (c) 2021 Travis Geiseblrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "bus.h"

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

#include "device.h"
#include "bus_mgr.h"
#include "resource.h"

namespace pci {

bus::bus(pci_location_t loc, bridge *b, bool root_bus) : loc_(loc), b_(b), root_bus_(root_bus) {}

void bus::add_device(device *d) {
    // TODO: assert that no two devices have the same address
    list_add_tail(&child_devices_, &d->node);
}

// walk all devices on a bus recursively walking into any bridge and scanning those busses
status_t bus::probe(pci_location_t loc, bridge *br, bus **out_bus, bool root_bus) {
    char str[14];
    LTRACEF("%s\n", pci_loc_string(loc, str));

    status_t err;

    // create a bus to hold any devices we find
    bus *b = new bus(loc, br, root_bus);

    // mark this as at least the last we've seen
    set_last_bus(b->bus_num());

    // probe all functions on all 32 devices on this bus.
    // add any devices found to this bus
    for (uint dev = 0; dev < 32; dev++) {
        loc.dev = dev;

        // loop all 8 functions, but only if after seeing the multifunction bit set
        // in the header type on fn 0. Otherwise just try fn 0 and stop.
        bool possibly_multifunction = false;
        for (uint fn = 0; (fn == 0) || (possibly_multifunction && fn < 8); fn++) {
            loc.fn = fn;

            // see if there's something at this slot
            // read vendor id and see if this is a real device
            uint16_t vendor_id;
            err = pci_read_config_half(loc, PCI_CONFIG_VENDOR_ID, &vendor_id);
            if (err != NO_ERROR || vendor_id == 0xffff) {
                continue;
            }

            LTRACEF("something at %s\n", pci_loc_string(loc, str));

            // read base and sub class
            uint8_t base_class;
            err = pci_read_config_byte(loc, PCI_CONFIG_CLASS_CODE_BASE, &base_class);
            if (err != NO_ERROR) {
                continue;
            }
            uint8_t sub_class;
            err = pci_read_config_byte(loc, PCI_CONFIG_CLASS_CODE_SUB, &sub_class);
            if (err != NO_ERROR) {
                continue;
            }

            // read header type (0 or 1)
            uint8_t header_type;
            err = pci_read_config_byte(loc, PCI_CONFIG_HEADER_TYPE, &header_type);
            if (err != NO_ERROR) {
                continue;
            }

            // is it multifunction?
            if (loc.fn == 0 && header_type & PCI_HEADER_TYPE_MULTI_FN) {
                possibly_multifunction = true;
                LTRACEF_LEVEL(2, "possibly multifunction\n");
            }

            header_type &= PCI_HEADER_TYPE_MASK;

            LTRACEF_LEVEL(2, "base:sub class %#hhx:%hhx\n", base_class, sub_class);

            // if it's a bridge, probe that
            // PCI-PCI bridge, normal decode
            if (base_class == 0x6 && sub_class == 0x4) { // XXX replace with #define
                LTRACEF("found bridge, recursing\n");
                err = bridge::probe(loc, b, &br);
                if (err != NO_ERROR) {
                    continue;
                }

                DEBUG_ASSERT(br);

                b->add_device(br);
            } else {
                device *d;
                err = device::probe(loc, b, &d);
                if (err != NO_ERROR) {
                    continue;
                }

                DEBUG_ASSERT(d);
                b->add_device(d);
            }
        }
    }

    *out_bus = b;

    return NO_ERROR;
}

status_t bus::allocate_resources(resource_allocator &allocator) {
    LTRACEF("bus %u\n", bus_num());

#if 0
    {
        auto perdev = [](device *d) -> status_t {
            device::bar_sizes sizes = {};

            d->compute_bar_sizes(&sizes);

            char str[14];
            printf("bar sizes for device %s : io %#x align %u mmio %#x align %u mmio64 %#llx align %u prefetch %#llx align %u\n",
                    pci_loc_string(d->loc(), str), sizes.io_size, sizes.io_align, sizes.mmio_size, sizes.mmio_align,
                    sizes.mmio64_size, sizes.mmio64_align, sizes.prefetchable_size, sizes.prefetchable_align);

            return 0;
        };

        for_every_device(perdev);
    }
#endif

    // accumulate a list of allocation requests for all devices on this bus
    list_node alloc_requests;
    list_initialize(&alloc_requests);
    auto perdev = [&](device *d) -> status_t {
        d->get_bar_alloc_requests(&alloc_requests);
        return 0;
    };
    for_every_device(perdev);

    auto dump_list = [](list_node *list) {
        device::bar_alloc_request *r;
        list_for_every_entry(list, r, device::bar_alloc_request, node) {
            r->dump();
        }
    };

    // split the list into different allocations and sort by size
    auto split_and_sort_list = [](pci_resource_type type, list_node *main_list, list_node *out_list) {
        device::bar_alloc_request *r, *temp;
        list_for_every_entry_safe(main_list, r, temp, device::bar_alloc_request, node) {
            if (r->type == type) {
                list_delete(&r->node);

                // add it to the destination list, sorted by size
                bool added = false;
                device::bar_alloc_request *temp2;
                list_for_every_entry(out_list, temp2, device::bar_alloc_request, node) {
                    if (r->size > temp2->size) {
                        list_add_before(&temp2->node, &r->node);
                        added = true;
                        break;
                    }
                }
                if (!added) {
                    list_add_tail(out_list, &r->node);
                }
            }
        }
    };

    // create a sorted list of all io requests
    list_node io_requests;
    list_initialize(&io_requests);
    split_and_sort_list(PCI_RESOURCE_IO_RANGE, &alloc_requests, &io_requests);

    if (LOCAL_TRACE) {
        printf("IO requests for devices on bus %d:\n", bus_num());
        dump_list(&io_requests);
    }

    // create a sorted list of all mmio requests, 32, 64 and prefetchable combined
    list_node mmio_requests;
    list_initialize(&mmio_requests);

    split_and_sort_list(PCI_RESOURCE_MMIO_RANGE, &alloc_requests, &mmio_requests);
    split_and_sort_list(PCI_RESOURCE_MMIO64_RANGE, &alloc_requests, &mmio_requests);

    if (LOCAL_TRACE) {
        printf("MMIO combined requests on bus %d:\n", bus_num());
        dump_list(&mmio_requests);
    }

    // allocate and assign IO ranges
    device::bar_alloc_request *r, *temp;
    list_for_every_entry_safe(&io_requests, r, temp, device::bar_alloc_request, node) {
        uint32_t addr = 0;
        auto err = allocator.allocate_io(r->size, r->align, &addr);
        if (err != NO_ERROR) {
            TRACEF("ERR: error allocating resource\n");
            r->dump();
            continue;
        }

        DEBUG_ASSERT(r->dev);

        r->dev->assign_resource(r, addr);
        list_delete(&r->node);
        delete r;
    }

    // allocate and assign various kinds of mmio ranges
    list_for_every_entry_safe(&mmio_requests, r, temp, device::bar_alloc_request, node) {
        uint64_t addr = 0;
        auto type = r->type;
        const bool can_be_64bit = (type == PCI_RESOURCE_MMIO64_RANGE);

        // root busses dont need to worry about allocating from a prefetchable pool
        // in our resource allocator, so ask for non prefetchable memory
        const bool prefetchable = root_bus_ ? false : r->prefetchable;

        auto err = allocator.allocate_mmio(can_be_64bit, prefetchable, r->size, r->align, &addr);
        if (err != NO_ERROR) {
            // failed with a 32bit alloc
            panic("failed to allocate resource\n");
        }

        DEBUG_ASSERT(r->dev);

        r->dev->assign_resource(r, addr);
        list_delete(&r->node);
        delete r;
    }

    // instruct all the devices on the bus that may have children (bridges) to assign resources
    for_every_device([](device *d) -> status_t {
        d->assign_child_resources();
        return 0;
    });

    return NO_ERROR;
}

void bus::dump(size_t indent) {
    for (size_t i = 0; i < indent; i++) {
        printf(" ");
    }
    printf("bus %d\n", loc().bus);
    device *d;
    list_for_every_entry(&child_devices_, d, device, node) {
        d->dump(indent + 1);
    }
}

void bus::add_to_global_list() {
    add_to_bus_list(this);
}

} // namespace pci
