//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <lk/cpp.h>
#include <sys/types.h>

#include "ahci.h"
#include "ahci_hw.h"


class ahci_port {
public:
    ahci_port(ahci &a, uint num) : ahci_(a), num_(num) {}
    ~ahci_port() = default;

    DISALLOW_COPY_ASSIGN_AND_MOVE(ahci_port);

    status_t probe();

private:
    uint32_t read_port_reg(ahci_port_reg reg);
    void write_port_reg(ahci_port_reg reg, uint32_t val);

    ahci &ahci_;
    uint num_;

    volatile ahci_cmd_list *cmd_list_ = nullptr;
    volatile uint8_t *fis_ = nullptr;
};

inline uint32_t ahci_port::read_port_reg(ahci_port_reg reg) {
    return ahci_.read_port_reg(num_, reg);
}

inline void ahci_port::write_port_reg(ahci_port_reg reg, uint32_t val) {
    ahci_.write_port_reg(num_, reg, val);
}

