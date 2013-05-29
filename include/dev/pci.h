/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __PCI_H
#define __PCI_H

#include <sys/types.h>
#include <compiler.h>

/*
 * PCI access return codes
 */
#define _PCI_SUCCESSFUL             0x00
#define _PCI_FUNC_NOT_SUPPORTED     0x81
#define _PCI_BAD_VENDOR_ID          0x83
#define _PCI_DEVICE_NOT_FOUND       0x86
#define _PCI_BAD_REGISTER_NUMBER    0x87
#define _PCI_SET_FAILED             0x88
#define _PCI_BUFFER_TOO_SMALL       0x89

/*
 * PCI configuration space offsets
 */
#define PCI_CONFIG_VENDOR_ID        0x00
#define PCI_CONFIG_DEVICE_ID        0x02
#define PCI_CONFIG_COMMAND          0x04
#define PCI_CONFIG_STATUS           0x06
#define PCI_CONFIG_REVISION_ID      0x08
#define PCI_CONFIG_CLASS_CODE       0x09
#define PCI_CONFIG_CLASS_CODE_INTR  0x09
#define PCI_CONFIG_CLASS_CODE_SUB   0x0a
#define PCI_CONFIG_CLASS_CODE_BASE  0x0b
#define PCI_CONFIG_CACHE_LINE_SIZE  0x0c
#define PCI_CONFIG_LATENCY_TIMER    0x0d
#define PCI_CONFIG_HEADER_TYPE      0x0e
#define PCI_CONFIG_BIST             0x0f
#define PCI_CONFIG_BASE_ADDRESSES   0x10
#define PCI_CONFIG_CARDBUS_CIS_PTR  0x28
#define PCI_CONFIG_SUBSYS_VENDOR_ID 0x2c
#define PCI_CONFIG_SUBSYS_ID        0x2e
#define PCI_CONFIG_EXP_ROM_ADDRESS  0x30
#define PCI_CONFIG_CAPABILITIES     0x34
#define PCI_CONFIG_INTERRUPT_LINE   0x3c
#define PCI_CONFIG_INTERRUPT_PIN    0x3d
#define PCI_CONFIG_MIN_GRANT        0x3e
#define PCI_CONFIG_MAX_LATENCY      0x3f

/*
 * PCI header type register bits
 */
#define PCI_HEADER_TYPE_MASK        0x7f
#define PCI_HEADER_TYPE_MULTI_FN    0x80

/*
 * PCI header types
 */
#define PCI_HEADER_TYPE_STANDARD    0x00
#define PCI_HEADER_TYPE_PCI_BRIDGE  0x01
#define PCI_HEADER_TYPE_CARD_BUS    0x02

/*
 * PCI command register bits
 */
#define PCI_COMMAND_IO_EN           0x0001
#define PCI_COMMAND_MEM_EN          0x0002
#define PCI_COMMAND_BUS_MASTER_EN   0x0004
#define PCI_COMMAND_SPECIAL_EN      0x0008
#define PCI_COMMAND_MEM_WR_INV_EN   0x0010
#define PCI_COMMAND_PAL_SNOOP_EN    0x0020
#define PCI_COMMAND_PERR_RESP_EN    0x0040
#define PCI_COMMAND_AD_STEP_EN      0x0080
#define PCI_COMMAND_SERR_EN         0x0100
#define PCI_COMMAND_FAST_B2B_EN     0x0200

/*
 * PCI status register bits
 */
#define PCI_STATUS_NEW_CAPS         0x0010
#define PCI_STATUS_66_MHZ           0x0020
#define PCI_STATUS_FAST_B2B         0x0080
#define PCI_STATUS_MSTR_PERR        0x0100
#define PCI_STATUS_DEVSEL_MASK      0x0600
#define PCI_STATUS_TARG_ABORT_SIG   0x0800
#define PCI_STATUS_TARG_ABORT_RCV   0x1000
#define PCI_STATUS_MSTR_ABORT_RCV   0x2000
#define PCI_STATUS_SERR_SIG         0x4000
#define PCI_STATUS_PERR             0x8000

typedef struct {
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
} __PACKED pci_config_t;

/*
 * PCI address structure
 */
typedef struct {
	uint8_t bus;
	uint8_t dev_fn;
} pci_location_t;

typedef struct {
	uint8_t id;
	uint8_t next;
} __PACKED pci_capability_t;

typedef struct {
	uint8_t bus;
	uint8_t device;
	uint8_t link_int_a;
	uint16_t irq_int_a;
	uint8_t link_int_b;
	uint16_t irq_int_b;
	uint8_t link_int_c;
	uint16_t irq_int_c;
	uint8_t link_int_d;
	uint16_t irq_int_d;
	uint8_t slot;
	uint8_t reserved;
} __PACKED irq_routing_entry;

void pci_init(void);

int pci_get_last_bus(void);

int pci_find_pci_device(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, uint16_t index);
int pci_find_pci_class_code(pci_location_t *state, uint32_t class_code, uint16_t index);

int pci_read_config_byte(const pci_location_t *state, uint32_t reg, uint8_t *value);
int pci_read_config_half(const pci_location_t *state, uint32_t reg, uint16_t *value);
int pci_read_config_word(const pci_location_t *state, uint32_t reg, uint32_t *value);

int pci_write_config_byte(const pci_location_t *state, uint32_t reg, uint8_t value);
int pci_write_config_half(const pci_location_t *state, uint32_t reg, uint16_t value);
int pci_write_config_word(const pci_location_t *state, uint32_t reg, uint32_t value);

int pci_get_irq_routing_options(irq_routing_entry *entries, uint16_t *count, uint16_t *pci_irqs);
int pci_set_irq_hw_int(const pci_location_t *state, uint8_t int_pin, uint8_t irq);

#endif
