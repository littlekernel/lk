/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/debug.h>
#include <lk/err.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/thread.h>
#include <kernel/spinlock.h>
#include <dev/bus/pci.h>
#include <lk/trace.h>

#include "pci_priv.h"

#define LOCAL_TRACE 1

int last_bus = 0;
static spin_lock_t lock;

int pci_get_last_bus(void) {
    return last_bus;
}

/*
 * pointers to installed PCI routines
 */
int (*g_pci_find_pci_device)(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, uint16_t index);
int (*g_pci_find_pci_class_code)(pci_location_t *state, uint32_t class_code, uint16_t index);

int (*g_pci_read_config_byte)(const pci_location_t *state, uint32_t reg, uint8_t *value);
int (*g_pci_read_config_half)(const pci_location_t *state, uint32_t reg, uint16_t *value);
int (*g_pci_read_config_word)(const pci_location_t *state, uint32_t reg, uint32_t *value);

int (*g_pci_write_config_byte)(const pci_location_t *state, uint32_t reg, uint8_t value);
int (*g_pci_write_config_half)(const pci_location_t *state, uint32_t reg, uint16_t value);
int (*g_pci_write_config_word)(const pci_location_t *state, uint32_t reg, uint32_t value);

int (*g_pci_get_irq_routing_options)(irq_routing_options_t *options, uint16_t *pci_irqs);
int (*g_pci_set_irq_hw_int)(const pci_location_t *state, uint8_t int_pin, uint8_t irq);

/* user facing routines */
int pci_find_pci_device(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, uint16_t index) {
    if (unlikely(!g_pci_find_pci_device)) {
        return ERR_NOT_CONFIGURED;
    }

    spin_lock_saved_state_t irqstate;
    spin_lock_irqsave(&lock, irqstate);

    int res = g_pci_find_pci_device(state, device_id, vendor_id, index);

    spin_unlock_irqrestore(&lock, irqstate);

    return res;
}

int pci_find_pci_class_code(pci_location_t *state, uint32_t class_code, uint16_t index) {
    if (unlikely(!g_pci_find_pci_class_code)) {
        return ERR_NOT_CONFIGURED;
    }

    spin_lock_saved_state_t irqstate;
    spin_lock_irqsave(&lock, irqstate);

    int res = g_pci_find_pci_class_code(state, class_code, index);

    spin_unlock_irqrestore(&lock, irqstate);

    return res;
}

int pci_read_config_byte(const pci_location_t *state, uint32_t reg, uint8_t *value) {
    if (unlikely(!g_pci_read_config_byte)) {
        return ERR_NOT_CONFIGURED;
    }

    spin_lock_saved_state_t irqstate;
    spin_lock_irqsave(&lock, irqstate);

    int res = g_pci_read_config_byte(state, reg, value);

    spin_unlock_irqrestore(&lock, irqstate);

    return res;
}
int pci_read_config_half(const pci_location_t *state, uint32_t reg, uint16_t *value) {
    if (unlikely(!g_pci_read_config_half)) {
        return ERR_NOT_CONFIGURED;
    }

    spin_lock_saved_state_t irqstate;
    spin_lock_irqsave(&lock, irqstate);

    int res = g_pci_read_config_half(state, reg, value);

    spin_unlock_irqrestore(&lock, irqstate);

    return res;
}

int pci_read_config_word(const pci_location_t *state, uint32_t reg, uint32_t *value) {
    if (unlikely(!g_pci_read_config_word)) {
        return ERR_NOT_CONFIGURED;
    }

    spin_lock_saved_state_t irqstate;
    spin_lock_irqsave(&lock, irqstate);

    int res = g_pci_read_config_word(state, reg, value);

    spin_unlock_irqrestore(&lock, irqstate);

    return res;
}

int pci_write_config_byte(const pci_location_t *state, uint32_t reg, uint8_t value) {
    if (unlikely(!g_pci_write_config_byte)) {
        return ERR_NOT_CONFIGURED;
    }

    spin_lock_saved_state_t irqstate;
    spin_lock_irqsave(&lock, irqstate);

    int res = g_pci_write_config_byte(state, reg, value);

    spin_unlock_irqrestore(&lock, irqstate);

    return res;
}

int pci_write_config_half(const pci_location_t *state, uint32_t reg, uint16_t value) {
    if (unlikely(!g_pci_write_config_half)) {
        return ERR_NOT_CONFIGURED;
    }

    spin_lock_saved_state_t irqstate;
    spin_lock_irqsave(&lock, irqstate);

    int res = g_pci_write_config_half(state, reg, value);

    spin_unlock_irqrestore(&lock, irqstate);

    return res;
}

int pci_write_config_word(const pci_location_t *state, uint32_t reg, uint32_t value) {
    if (unlikely(!g_pci_write_config_word)) {
        return ERR_NOT_CONFIGURED;
    }

    spin_lock_saved_state_t irqstate;
    spin_lock_irqsave(&lock, irqstate);

    int res = g_pci_write_config_word(state, reg, value);

    spin_unlock_irqrestore(&lock, irqstate);

    return res;
}


int pci_get_irq_routing_options(irq_routing_entry *entries, uint16_t *count, uint16_t *pci_irqs) {
    if (unlikely(!g_pci_get_irq_routing_options)) {
        return ERR_NOT_CONFIGURED;
    }

    // TODO: highly bios32 specific, abstract this differently
    irq_routing_options_t options;
    options.size = sizeof(irq_routing_entry) **count;
    options.selector = 0x10; // XXX actually DATA_SELECTOR
    options.offset = entries;

    spin_lock_saved_state_t irqstate;
    spin_lock_irqsave(&lock, irqstate);

    int res = g_pci_get_irq_routing_options(&options, pci_irqs);

    spin_unlock_irqrestore(&lock, irqstate);

    *count = options.size / sizeof(irq_routing_entry);

    return res;
}

int pci_set_irq_hw_int(const pci_location_t *state, uint8_t int_pin, uint8_t irq) {
    if (unlikely(!g_pci_set_irq_hw_int)) {
        return ERR_NOT_CONFIGURED;
    }

    spin_lock_saved_state_t irqstate;
    spin_lock_irqsave(&lock, irqstate);

    int res = g_pci_set_irq_hw_int(state, int_pin, irq);

    spin_unlock_irqrestore(&lock, irqstate);

    return res;
}

void pci_init(void) {
    // try a series of detection mechanisms

    // try to BIOS32 access first, if present
    if (pci_bios_detect() >= 0) {
        dprintf(INFO, "PCI: pci bios functions installed\n");
        dprintf(INFO, "PCI: last pci bus is %d\n", last_bus);
        return;
    }

    // try type 1 access
    if (pci_type1_detect() >= 0) {
        dprintf(INFO, "PCI: pci bios functions installed\n");
        dprintf(INFO, "PCI: last pci bus is %d\n", last_bus);
        return;
    }
}
