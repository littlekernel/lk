//
// Copyright (c) 2021 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#pragma once

#include <stdint.h>
#include <stdio.h>

// from 8254x Family SDM Table 13-2
enum class e1000_reg {
    // general
    CTRL = 0x0,
    STATUS = 0x8,
    EECD = 0x10,
    EERD = 0x14,
    CTL_EXT = 0x18,
    MDIC = 0x20,
    FACL = 0x28,
    FACH = 0x2c,
    FCT = 0x30,
    VET = 0x38,
    FCTTV = 0x170,
    TXCW = 0x178,
    RXCW = 0x180,
    LEDCTL = 0xe00,

    // DMA
    DMA = 0x1000,

    // interrupt
    ICR = 0xc0,
    ITR = 0xc4,
    IMS = 0xd0,
    IMC = 0xd8,
    IAM = 0xe0,
    EITR0 = 0x1680, // e1000e only (i210)+
    EITR1 = 0x1684,
    EITR2 = 0x1688,
    EITR3 = 0x168c,
    EITR4 = 0x1690,

    // receive
    RCTL = 0x100,
    FCRTL = 0x2160,
    FCRTH = 0x2168,
    RDBAL = 0x2800,
    RDBAH = 0x2804,
    RDLEN = 0x2808,
    RDH = 0x2810,
    RDT = 0x2818,
    RDTR = 0x2820,
    RADV = 0x282c,
    RSRPD = 0x2c00,

    // transmit
    TCTL = 0x400,
    TIPG = 0x410,
    AIFS = 0x458,
    TDBAL = 0x3800,
    TDBAH = 0x3804,
    TDLEN = 0x3808,
    TDH = 0x3810,
    TDT = 0x3818,
    TIDV = 0x3820,

    // tx dma
    TXDMAC = 0x3000,
    TXDCTL = 0x3828,
    TADV = 0x382c,
    TSPMT = 0x3830,

    // rx dma
    RXDCTL = 0x2828,
    RXCSUM = 0x5000,
};

// receive descriptor
struct rdesc {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;

    void dump() {
        printf("rdsec %p: addr %#llx len %hu cksum %#hx stat %#hhx err %#hhx spec %#hx\n",
               this, addr, length, checksum, status, errors, special);
    }

};
static_assert(sizeof(rdesc) == 16, "");

// transmit descriptor (legacy)
struct tdesc {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t sta_rsv;
    uint8_t css;
    uint16_t special;

    void dump() {
        printf("tdsec %p: addr %#llx len %hu cso %#hhx cmd %#hhx sta_rsv %#hhx css %#hhx spec %#hx\n",
               this, addr, length, cso, cmd, sta_rsv, css, special);
    }
};
static_assert(sizeof(tdesc) == 16, "");

// efficient copy for rx/tx descriptors out/into uncached memory
template <typename T>
inline void copy(T *_dst, const T *_src) {
    // only allow this for structs that are precisely 16 bytes long
    static_assert(sizeof(T) == 16, "");

    // treat as two 8 byte copies
    uint64_t *dst = (uint64_t *)_dst;
    const uint64_t *src = (uint64_t *)_src;

    dst[0] = src[0];
    dst[1] = src[1];
}

