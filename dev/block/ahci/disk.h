//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <lk/cpp.h>
#include <lk/list.h>
#include <sys/types.h>

#include "port.h"

class ahci_disk {
public:
    ahci_disk(ahci_port &p);
    ~ahci_disk();

    DISALLOW_COPY_ASSIGN_AND_MOVE(ahci_disk);

    status_t identify();

    list_node node_ = LIST_INITIAL_CLEARED_VALUE;
private:
    ahci_port &port_;
};
