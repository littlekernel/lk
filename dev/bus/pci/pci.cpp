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

int pci_get_last_bus() {
    if (!pcib) {
        return ERR_NOT_CONFIGURED;
    }

    return pcib->get_last_bus();
}

/* user facing routines */
status_t pci_find_pci_device(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, uint16_t index) {
    LTRACEF("device_id dev %#hx vendor %#hx index %#hx\n", device_id, vendor_id, index);

    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->find_pci_device(state, device_id, vendor_id, index);

    return res;
}

status_t pci_find_pci_class_code(pci_location_t *state, uint32_t class_code, uint16_t index) {
    LTRACEF("device_id class %#x index %#hx\n", class_code, index);

    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->find_pci_class_code(state, class_code, index);

    return res;
}

status_t pci_read_config_byte(const pci_location_t *state, uint32_t reg, uint8_t *value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->read_config_byte(state, reg, value);

    return res;
}
status_t pci_read_config_half(const pci_location_t *state, uint32_t reg, uint16_t *value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->read_config_half(state, reg, value);

    return res;
}

status_t pci_read_config_word(const pci_location_t *state, uint32_t reg, uint32_t *value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->read_config_word(state, reg, value);

    return res;
}

status_t pci_write_config_byte(const pci_location_t *state, uint32_t reg, uint8_t value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->write_config_byte(state, reg, value);

    return res;
}

status_t pci_write_config_half(const pci_location_t *state, uint32_t reg, uint16_t value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->write_config_half(state, reg, value);

    return res;
}

status_t pci_write_config_word(const pci_location_t *state, uint32_t reg, uint32_t value) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->write_config_word(state, reg, value);

    return res;
}

status_t pci_get_irq_routing_options(irq_routing_entry *entries, uint16_t *count, uint16_t *pci_irqs) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    // TODO: highly bios32 specific, abstract this differently
    pci_backend::irq_routing_options_t options;
    options.size = sizeof(irq_routing_entry) * *count;
    options.selector = 0x10; // XXX actually DATA_SELECTOR
    options.offset = entries;

    int res;
    {
        AutoSpinLock guard(&lock);

        res = pcib->get_irq_routing_options(&options, pci_irqs);
    }

    *count = options.size / sizeof(irq_routing_entry);

    return res;
}

status_t pci_set_irq_hw_int(const pci_location_t *state, uint8_t int_pin, uint8_t irq) {
    if (!pcib) return ERR_NOT_CONFIGURED;

    AutoSpinLock guard(&lock);

    int res = pcib->set_irq_hw_int(state, int_pin, irq);

    return res;
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

