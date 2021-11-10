/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>
#include <lk/compiler.h>
#include <lk/cpp.h>
#include <lk/err.h>
#include <dev/bus/pci.h>

// Default implementation of a particular PCI bus/config accessor method.
// Intended to be subclassed and specialized based on specific method.

class pci_backend {
public:
    constexpr pci_backend() = default;
    virtual ~pci_backend() = default;

    DISALLOW_COPY_ASSIGN_AND_MOVE(pci_backend);

public:
    int get_last_bus() const { return last_bus_; }

    // virtuals that a concrete implementation should override
    virtual int find_pci_device(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, uint16_t index) {
        return ERR_NOT_CONFIGURED;
    }

    virtual int find_pci_class_code(pci_location_t *state, uint32_t class_code, uint16_t index) {
        return ERR_NOT_CONFIGURED;
    }

    virtual int read_config_byte(const pci_location_t *state, uint32_t reg, uint8_t *value) {
        return ERR_NOT_CONFIGURED;
    }
    virtual int read_config_half(const pci_location_t *state, uint32_t reg, uint16_t *value) {
        return ERR_NOT_CONFIGURED;
    }
    virtual int read_config_word(const pci_location_t *state, uint32_t reg, uint32_t *value) {
        return ERR_NOT_CONFIGURED;
    }

    virtual int write_config_byte(const pci_location_t *state, uint32_t reg, uint8_t value) {
        return ERR_NOT_CONFIGURED;
    }
    virtual int write_config_half(const pci_location_t *state, uint32_t reg, uint16_t value) {
        return ERR_NOT_CONFIGURED;
    }
    virtual int write_config_word(const pci_location_t *state, uint32_t reg, uint32_t value) {
        return ERR_NOT_CONFIGURED;
    }

    // TODO: highly bios32 specific
    struct irq_routing_options_t {
        uint16_t size;
        void *offset;
        uint16_t selector;
    } __PACKED;

    virtual int get_irq_routing_options(irq_routing_options_t *options, uint16_t *pci_irqs) {
        return ERR_NOT_CONFIGURED;
    }
    virtual int set_irq_hw_int(const pci_location_t *state, uint8_t int_pin, uint8_t irq) {
        return ERR_NOT_CONFIGURED;
    }

protected:
    // detection should find the last bus
    void set_last_bus(int last_bus) { last_bus_ = last_bus; }
    int last_bus_ = -1;
};
