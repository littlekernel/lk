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
status_t bus::probe(pci_location_t loc, bridge *bridge, bus **out_bus) {
    char str[14];
    LTRACEF("%s\n", pci_loc_string(loc, str));

    status_t err;

    // create a bus to hold any devices we find
    bus *b = new bus(loc, bridge);

    // probe all functions on all 32 devices on this bus.
    // add any devices found to this bus
    for (uint dev = 0; dev < 32; dev++) {
        loc.dev = dev;
        for (uint fn = 0; fn < 8; fn++) {
            loc.fn = fn;

            device *d;
            bool multifunction;
            err = device::probe(loc, b, &d, &multifunction);
            if (err < 0) {
                break;
            }

            b->add_device(d);

            // move on to the next device
            if (fn == 0 && !multifunction) {
                break;
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
