/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include "pci_backend.h"

#include <stdint.h>

class pci_bios32 final : public pci_backend {
#if ARCH_X86_32
public:
    virtual ~pci_bios32() = default;

    // factory to detect and create an instance
    static pci_bios32 *detect();

    // a few overridden methods
    virtual int find_pci_device(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, uint16_t index) override;
    virtual int find_pci_class_code(pci_location_t *state, uint32_t class_code, uint16_t index) override;

    virtual int read_config_byte(const pci_location_t *state, uint32_t reg, uint8_t *value) override;
    virtual int read_config_half(const pci_location_t *state, uint32_t reg, uint16_t *value) override;
    virtual int read_config_word(const pci_location_t *state, uint32_t reg, uint32_t *value) override;

    virtual int write_config_byte(const pci_location_t *state, uint32_t reg, uint8_t value) override;
    virtual int write_config_half(const pci_location_t *state, uint32_t reg, uint16_t value) override;
    virtual int write_config_word(const pci_location_t *state, uint32_t reg, uint32_t value) override;

    virtual int get_irq_routing_options(irq_routing_options_t *options, uint16_t *pci_irqs) override;
    virtual int set_irq_hw_int(const pci_location_t *state, uint8_t int_pin, uint8_t irq) override;

private:
    // far call structure used by BIOS32 routines
    struct bios32_entry {
        uint32_t offset;
        uint16_t selector;
    } __PACKED;

    // only created via the detect() factory
    explicit pci_bios32(bios32_entry b32_entry) : bios32_entry_(b32_entry) {}

    bios32_entry bios32_entry_ {};

#else // !ARCH_X86_32

    // not present on anything but x86-32
public:
    static pci_bios32 *detect() { return nullptr; }
#endif
};


