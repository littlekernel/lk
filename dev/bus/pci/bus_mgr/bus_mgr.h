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

// global state 
namespace pci {
class bus;
extern bus *root;
extern list_node bus_list;
void add_to_bus_list(bus *b);
}
