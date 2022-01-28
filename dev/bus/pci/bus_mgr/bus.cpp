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

namespace pci {

bus::bus(pci_location_t loc, bridge *b) : loc_(loc), b_(b) {}

void bus::add_device(device *d) {
    // TODO: assert that no two devices have the same address
    list_add_tail(&child_devices_, &d->node);
}

// walk all devices on a bus recursively walking into any bridge and scanning those busses
status_t bus::probe(pci_location_t loc, bridge *br, bus **out_bus) {
    char str[14];
    LTRACEF("%s\n", pci_loc_string(loc, str));

    status_t err;

    // create a bus to hold any devices we find
    bus *b = new bus(loc, br);

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
