/*
 * Copyright (c) 2021 Travis Geiseblrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lk/cpp.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <sys/types.h>
#include <dev/bus/pci.h>

namespace pci {

// global state
class bus;
class root;
extern list_node root_list;
extern list_node bus_list;
void add_to_bus_list(bus *b);

// get a pointer to a bus based on segment and number
bus *lookup_bus(uint16_t segment, uint8_t bus_num);

} // namespace pci
