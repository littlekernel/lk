//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

// From serial-ata-ahci-spec-rev-1-3-1.pdf
#pragma once

#include <stdint.h>

// registers relative to the base of the ABAR
enum class ahci_reg {
    CAP = 0x0, // capability
    GHC = 0x4, // global HBA control
    IS = 0x8, // interrupt status
    PI = 0xc, // ports implemented
    VS = 0x10, // version
    CCC_CTL = 0x14, // command completion coalescing control
    CCC_PORTS = 0x18, // command completion coalescing ports
    EM_LOC = 0x1c, // enclosure management location
    EM_CTL = 0x20, // enclosure management control
    CAP2 = 0x24, // HBA capabilities extended
    BOHC = 0x28, // BIOS/OS handoff control and status

    // registers 0xa0 to 0xff are vendor specific

    // port specific registers are enumerated below,
    // repeated every 0x80 starting at 0x100
};

enum class ahci_port_reg {
    PxCLB = 0x0,
    PxCLBU = 0x4,
    PxFB = 0x8,
    PxFBU = 0xc,
    PxIS = 0x10,
    PxIE = 0x14,
    PxCMD = 0x18,

    PxTFD = 0x20,
    PxSIG = 0x24,
    PxSSTS = 0x28,
    PxSCTL = 0x2c,
    PxSERR = 0x30,
    PxSACT = 0x34,
    PxCI = 0x38,
    PxSNTF = 0x3c,
    PxFBS = 0x40,
    PxDEVSLP = 0x44,

    PxVS = 0x70,
};

// command header
struct ahci_cmd_header {
    union {
        uint32_t dw[8]; // raw 8 byte words
        struct {
            uint16_t cmd;   // raw command bits
            uint16_t prdtl; // physical region descriptor entry count
            uint32_t prdbc; // physical region descriptor byte count
            uint32_t ctba;  // command table base address
            uint32_t ctbau; // command table base address upper
        };
    };
};

static_assert(sizeof(ahci_cmd_header) == 0x20, "");

// physical region descriptor (PRDT entry)
struct ahci_prd {
    union {
        uint32_t dw[4]; // raw 4 byte words
        struct {
            uint32_t dba; // data base address
            uint32_t dbau; // data base address upper
            uint32_t _reserved;
            uint32_t byte_count_ioc; // byte count [0:21], interrupt on completion [31]
        };
    };
};

static_assert(sizeof(ahci_prd) == 0x10, "");

struct ahci_cmd_table {
    uint8_t cfis[64];

    // offset 0x40
    uint8_t acmd[16];

    // offset 0x80
    uint8_t _reserved[0x80 - 0x50];

    ahci_prd pdrt[0];
};

static_assert(sizeof(ahci_cmd_table) == 0x80, "");
