/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include "pci_backend.h"

class pci_ecam final : public pci_backend {
public:
    virtual ~pci_ecam();

    // factory to detect and create an instance
    static pci_ecam *detect(paddr_t ecam_base, uint16_t segment, uint8_t start_bus, uint8_t end_bus);

    // a few overridden methods
    int read_config_byte(pci_location_t state, uint32_t reg, uint8_t *value) override;
    int read_config_half(pci_location_t state, uint32_t reg, uint16_t *value) override;
    int read_config_word(pci_location_t state, uint32_t reg, uint32_t *value) override;
    int write_config_byte(pci_location_t state, uint32_t reg, uint8_t value) override;
    int write_config_half(pci_location_t state, uint32_t reg, uint16_t value) override;
    int write_config_word(pci_location_t state, uint32_t reg, uint32_t value) override;

private:
    // only created via the detect() factory
    pci_ecam(paddr_t base, uint16_t segment, uint8_t start_bus, uint8_t end_bus);

    // allocate vm resources for the object
    status_t initialize();

    paddr_t base_;
    uint16_t segment_;
    uint16_t start_bus_;
    uint16_t end_bus_;

    // vm region where the ecam is mapped
    uint8_t *ecam_ptr_ = nullptr;
};

