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
extern bus *root;
extern list_node bus_list;
void add_to_bus_list(bus *b);

// set the last bus seen
void set_last_bus(uint8_t bus);
uint8_t get_last_bus();

// allocate the next bus (used when assigning busses to bridges)
uint8_t allocate_next_bus();

// get a pointer to a bus based on number
bus *lookup_bus(uint8_t bus_num);

} // namespace pci
