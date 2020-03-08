/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2020 Travis Geiseblrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

extern int last_bus;

// TODO: highly bios32 specific
typedef struct {
    uint16_t size;
    void *offset;
    uint16_t selector;
} __PACKED irq_routing_options_t;

/*
 * pointers to installed PCI routines
 */
extern int (*g_pci_find_pci_device)(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, uint16_t index);
extern int (*g_pci_find_pci_class_code)(pci_location_t *state, uint32_t class_code, uint16_t index);

extern int (*g_pci_read_config_byte)(const pci_location_t *state, uint32_t reg, uint8_t *value);
extern int (*g_pci_read_config_half)(const pci_location_t *state, uint32_t reg, uint16_t *value);
extern int (*g_pci_read_config_word)(const pci_location_t *state, uint32_t reg, uint32_t *value);

extern int (*g_pci_write_config_byte)(const pci_location_t *state, uint32_t reg, uint8_t value);
extern int (*g_pci_write_config_half)(const pci_location_t *state, uint32_t reg, uint16_t value);
extern int (*g_pci_write_config_word)(const pci_location_t *state, uint32_t reg, uint32_t value);

extern int (*g_pci_get_irq_routing_options)(irq_routing_options_t *options, uint16_t *pci_irqs);
extern int (*g_pci_set_irq_hw_int)(const pci_location_t *state, uint8_t int_pin, uint8_t irq);

int pci_bios_detect(void);
int pci_type1_detect(void);

