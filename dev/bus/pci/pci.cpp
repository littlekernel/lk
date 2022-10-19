/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <dev/bus/pci.h>

#include <lk/debug.h>
#include <lk/err.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/thread.h>
#include <kernel/spinlock.h>
#include <lk/trace.h>

#include "pci_priv.h"

#define LOCAL_TRACE 0

// Largely C level api for the PCI bus manager

namespace {
SpinLock lock;
pci_backend *pcib = nullptr;
} // namespace

/* user facing routines */
int pci_get_last_bus() {
    if (!pcib) {
        return ERR_NOT_CONFIGURED;
    }

    return pcib->get_last_bus();
}

int pci_get_last_segment() {
    // currently hard coded to 1 segment
    return 0;
}

status_t pci_read_config_byte(const pci_location_t state, uint32_t reg, uint8_t *value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->read_config_byte(state, reg, value);

    return res;
}
status_t pci_read_config_half(const pci_location_t state, uint32_t reg, uint16_t *value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->read_config_half(state, reg, value);

    return res;
}

status_t pci_read_config_word(const pci_location_t state, uint32_t reg, uint32_t *value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->read_config_word(state, reg, value);

    return res;
}

status_t pci_write_config_byte(const pci_location_t state, uint32_t reg, uint8_t value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->write_config_byte(state, reg, value);

    return res;
}

status_t pci_write_config_half(const pci_location_t state, uint32_t reg, uint16_t value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->write_config_half(state, reg, value);

    return res;
}

status_t pci_write_config_word(const pci_location_t state, uint32_t reg, uint32_t value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->write_config_word(state, reg, value);

    return res;
}

status_t pci_read_config(const pci_location_t loc, pci_config_t *config) {
    status_t err;

    *config = {};

    // TODO: handle endian swapping (if necessary)

    // define some helper routines to read config offsets in the proper unit
    size_t next_index;
    auto read_byte = [&]() -> uint8_t {
        uint8_t val;
        err = pci_read_config_byte(loc, next_index, &val);
        next_index++;
        if (err < 0) return 0;
        return val;
    };

    auto read_half = [&]() -> uint16_t {
        uint16_t val;
        err = pci_read_config_half(loc, next_index, &val);
        next_index += 2;
        if (err < 0) return 0;
        return val;
    };

    auto read_word = [&]() -> uint32_t {
        uint32_t val;
        err = pci_read_config_word(loc, next_index, &val);
        next_index += 4;
        if (err < 0) return 0;
        return val;
    };

    // shared, standard part of the pci config space
    /*
        uint16_t vendor_id;
        uint16_t device_id;
        uint16_t command;
        uint16_t status;
        uint8_t revision_id_0;
        uint8_t program_interface;
        uint8_t sub_class;
        uint8_t base_class;
        uint8_t cache_line_size;
        uint8_t latency_timer;
        uint8_t header_type;
        uint8_t bist;
    */
    next_index = 0;
    config->vendor_id = read_half(); if (err < 0) return err;
    config->device_id = read_half(); if (err < 0) return err;
    config->command = read_half(); if (err < 0) return err;
    config->status = read_half(); if (err < 0) return err;
    config->revision_id_0 = read_byte(); if (err < 0) return err;
    config->program_interface = read_byte(); if (err < 0) return err;
    config->sub_class = read_byte(); if (err < 0) return err;
    config->base_class = read_byte(); if (err < 0) return err;
    config->cache_line_size = read_byte(); if (err < 0) return err;
    config->latency_timer = read_byte(); if (err < 0) return err;
    config->header_type = read_byte(); if (err < 0) return err;
    config->bist = read_byte(); if (err < 0) return err;
    DEBUG_ASSERT(next_index == 0x10); // should have read this many bytes at this point

    // based on the type field, read two different types
    const uint8_t type = config->header_type & 0x7f;
    if (type == 0) {
        /*
            uint32_t base_addresses[6];
            uint32_t cardbus_cis_ptr;
            uint16_t subsystem_vendor_id;
            uint16_t subsystem_id;
            uint32_t expansion_rom_address;
            uint8_t capabilities_ptr;
            uint8_t reserved_0[3];
            uint32_t reserved_1;
            uint8_t interrupt_line;
            uint8_t interrupt_pin;
            uint8_t min_grant;
            uint8_t max_latency;
        */
        config->type0.base_addresses[0] = read_word(); if (err < 0) return err;
        config->type0.base_addresses[1] = read_word(); if (err < 0) return err;
        config->type0.base_addresses[2] = read_word(); if (err < 0) return err;
        config->type0.base_addresses[3] = read_word(); if (err < 0) return err;
        config->type0.base_addresses[4] = read_word(); if (err < 0) return err;
        config->type0.base_addresses[5] = read_word(); if (err < 0) return err;
        config->type0.cardbus_cis_ptr = read_word(); if (err < 0) return err;
        config->type0.subsystem_vendor_id = read_half(); if (err < 0) return err;
        config->type0.subsystem_id = read_half(); if (err < 0) return err;
        config->type0.expansion_rom_address = read_word(); if (err < 0) return err;
        config->type0.capabilities_ptr = read_byte(); if (err < 0) return err;
        next_index += 3 + 4; // 7 bytes of reserved space
        config->type0.interrupt_line = read_byte(); if (err < 0) return err;
        config->type0.interrupt_pin = read_byte(); if (err < 0) return err;
        config->type0.min_grant = read_byte(); if (err < 0) return err;
        config->type0.max_latency = read_byte(); if (err < 0) return err;
        DEBUG_ASSERT(next_index == 0x40); // should have read this many bytes at this point
    } else if (type == 1) {
        /*
            uint32_t base_addresses[2];
            uint8_t primary_bus;
            uint8_t secondary_bus;
            uint8_t subordinate_bus;
            uint8_t secondary_latency_timer;
            uint8_t io_base;
            uint8_t io_limit;
            uint16_t secondary_status;
            uint16_t memory_base;
            uint16_t memory_limit;
            uint16_t prefetchable_memory_base;
            uint16_t prefetchable_memory_limit;
            uint32_t prefetchable_base_upper;
            uint32_t prefetchable_limit_upper;
            uint16_t io_base_upper;
            uint16_t io_limit_upper;
            uint8_t capabilities_ptr;
            uint8_t reserved_0[3];
            uint32_t expansion_rom_address;
            uint8_t interrupt_line;
            uint8_t interrupt_pin;
            uint16_t bridge_control;
        */
        config->type1.base_addresses[0] = read_word(); if (err < 0) return err;
        config->type1.base_addresses[1] = read_word(); if (err < 0) return err;
        config->type1.primary_bus = read_byte(); if (err < 0) return err;
        config->type1.secondary_bus = read_byte(); if (err < 0) return err;
        config->type1.subordinate_bus = read_byte(); if (err < 0) return err;
        config->type1.secondary_latency_timer = read_byte(); if (err < 0) return err;
        config->type1.io_base = read_byte(); if (err < 0) return err;
        config->type1.io_limit = read_byte(); if (err < 0) return err;
        config->type1.secondary_status = read_half(); if (err < 0) return err;
        config->type1.memory_base = read_half(); if (err < 0) return err;
        config->type1.memory_limit = read_half(); if (err < 0) return err;
        config->type1.prefetchable_memory_base = read_half(); if (err < 0) return err;
        config->type1.prefetchable_memory_limit = read_half(); if (err < 0) return err;
        config->type1.prefetchable_base_upper = read_word(); if (err < 0) return err;
        config->type1.prefetchable_limit_upper = read_word(); if (err < 0) return err;
        config->type1.io_base_upper = read_half(); if (err < 0) return err;
        config->type1.io_limit_upper = read_half(); if (err < 0) return err;
        config->type1.capabilities_ptr = read_byte(); if (err < 0) return err;
        next_index += 3; // 3 reserved bytes
        config->type1.expansion_rom_address = read_word(); if (err < 0) return err;
        config->type1.interrupt_line = read_byte(); if (err < 0) return err;
        config->type1.interrupt_pin = read_byte(); if (err < 0) return err;
        config->type1.bridge_control = read_half(); if (err < 0) return err;

        DEBUG_ASSERT(next_index == 0x40); // should have read this many bytes at this point
    } else {
        // cant handle other types
        return ERR_NOT_VALID;
    }

    return NO_ERROR;
}

status_t pci_init_legacy() {
    LTRACE_ENTRY;

    DEBUG_ASSERT(pcib == nullptr);

    // try a series of detection mechanisms based on legacy PCI access on x86 PCs

    // try to BIOS32 access first, if present
    if ((pcib = pci_bios32::detect())) {
        dprintf(INFO, "PCI: pci bios functions installed\n");
        dprintf(INFO, "PCI: last pci bus is %d\n", pcib->get_last_bus());
        return NO_ERROR;
    }

    // try type 1 access
    if ((pcib = pci_type1::detect())) {
        dprintf(INFO, "PCI: pci type1 functions installed\n");
        dprintf(INFO, "PCI: last pci bus is %d\n", pcib->get_last_bus());
        return NO_ERROR;
    }

    // if we couldn't find anything, leave pcib null
    return ERR_NOT_FOUND;
}

status_t pci_init_ecam(paddr_t ecam_base, uint16_t segment, uint8_t start_bus, uint8_t end_bus) {
    LTRACEF("base %#lx, segment %hu, bus [%hhu...%hhu]\n", ecam_base, segment, start_bus, end_bus);

    DEBUG_ASSERT(pcib == nullptr);

    if ((pcib = pci_ecam::detect(ecam_base, segment, start_bus, end_bus))) {
        dprintf(INFO, "PCI: pci ecam functions installed\n");
        dprintf(INFO, "PCI: last pci bus is %d\n", pcib->get_last_bus());
        return NO_ERROR;
    }

    // if we couldn't find anything, leave pcib null
    return ERR_NOT_FOUND;
}

