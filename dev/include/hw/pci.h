/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2021 Travis Geiseblrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <assert.h>
#include <sys/types.h>
#include <lk/compiler.h>

// This file contains defines and structures for the PCI bus independent
// of any devices or drivers that may use them.

__BEGIN_CDECLS

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
/* Type 0 */
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

/* structure version of the standard pci config space */
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
    /* offset 0x10 */
    union {
        struct {
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
        } type0; // configuration for normal devices
        struct {
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
        } type1; // configuration for bridge devices
    };
} pci_config_t;
static_assert(sizeof(pci_config_t) == 0x40, "");

/* Class/subclass codes (incomplete) */
#define PCI_SUBCLASS_OTHER 0x80 // common 'other' in many of the subclasses

#define PCI_CLASS_UNCLASSIFIED 0x0
#define PCI_SUBCLASS_NON_VGA_UNCLASSIFIED 0x0
#define PCI_SUBCLASS_VGA_UNCLASSIFIED     0x1

#define PCI_CLASS_MASS_STORAGE 0x1
#define PCI_SUBCLASS_SCSI 0x0
#define PCI_SUBCLASS_IDE 0x1
#define PCI_SUBCLASS_FLOPPY 0x2
#define PCI_SUBCLASS_IPI 0x3
#define PCI_SUBCLASS_RAID 0x4
#define PCI_SUBCLASS_ATA 0x5
#define PCI_SUBCLASS_SERIAL_ATA 0x6
#define PCI_SUBCLASS_SAS 0x7
#define PCI_SUBCLASS_NON_VOLATILE 0x8

#define PCI_CLASS_NETWORK      0x2
#define PCI_SUBCLASS_ETHERNET 0x0
#define PCI_SUBCLASS_TOKEN_RING 0x1
#define PCI_SUBCLASS_FDDI 0x2
#define PCI_SUBCLASS_ATM 0x3
#define PCI_SUBCLASS_ISDN 0x4
#define PCI_SUBCLASS_WORLDFIP 0x5
#define PCI_SUBCLASS_PICMG 0x6
#define PCI_SUBCLASS_INFINIBAND 0x7

#define PCI_CLASS_DISPLAY      0x3
#define PCI_SUBCLASS_VGA 0x0
#define PCI_SUBCLASS_XGA 0x1
#define PCI_SUBCLASS_3D 0x2

#define PCI_CLASS_MULTIMEDIA   0x4
#define PCI_CLASS_MEMORY       0x5

#define PCI_CLASS_BRIDGE       0x6
#define PCI_SUBCLASS_HOST_BRIDGE 0x0
#define PCI_SUBCLASS_ISA_BRIDGE 0x1
#define PCI_SUBCLASS_EISA_BRIDGE 0x2
#define PCI_SUBCLASS_MCA_BRIDGE 0x3
#define PCI_SUBCLASS_PCI_PCI_BRIDGE 0x4
#define PCI_SUBCLASS_PCMCIA_BRIDGE 0x5
#define PCI_SUBCLASS_NUBUS_BRIDGE 0x6
#define PCI_SUBCLASS_CARDBUS_BRIDGE 0x7
#define PCI_SUBCLASS_RACEWAY_BRIDGE 0x8
#define PCI_SUBCLASS_PCI_PCI_BRIDGE2 0x9
#define PCI_SUBCLASS_INFINIBAND_TO_PCI_BRIDGE 0xa

#define PCI_CLASS_SIMPLE_COMMS 0x7
#define PCI_CLASS_BASE_PERIPH  0x8
#define PCI_CLASS_INPUT        0x9
#define PCI_CLASS_DOCKING_STATION 0xa
#define PCI_CLASS_PROCESSOR    0xb
#define PCI_CLASS_SERIAL_BUS   0xc
#define PCI_CLASS_WIRELESS     0xd
#define PCI_CLASS_INTELLIGENT_CONTROLLER 0xe
#define PCI_CLASS_SATELLITE    0xf
#define PCI_CLASS_ENCRYPTION   0x10
#define PCI_CLASS_SIGNAL_PROCESSING 0x11
#define PCI_CLASS_PROCESSING_ACCEL  0x12
#define PCI_CLASS_COPROCESSOR  0x40
#define PCI_CLASS_UNASSIGNED   0xff

__END_CDECLS

