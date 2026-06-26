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

    // receive address
    RAL0 = 0x5400, // Receive Address Low  (MAC bytes 0-3)
    RAH0 = 0x5404, // Receive Address High (MAC bytes 4-5, bit31=valid)
};

// Interrupt Cause Register (ICR) bits
#define E1000_ICR_TXDW         (1u << 0)  // Transmit descriptor write-back
#define E1000_ICR_TXQE         (1u << 1)  // Transmit queue empty
#define E1000_ICR_LSC          (1u << 2)  // Link status change
#define E1000_ICR_RXSEQ        (1u << 3)  // Receive sequence error
#define E1000_ICR_RXDMT0       (1u << 4)  // RX descriptor minimum threshold reached
#define E1000_ICR_RXO          (1u << 6)  // RX overrun
#define E1000_ICR_RXTO         (1u << 7)  // RX timer interrupt
#define E1000_ICR_MDAC         (1u << 9)  // MDI/O access completed
#define E1000_ICR_RXCFG        (1u << 10) // Receiving /C/ ordered set
#define E1000_ICR_GPI_SDP6     (1u << 12) // General purpose interrupt on SDP6
#define E1000_ICR_GPI_SDP7     (1u << 13) // General purpose interrupt on SDP7
#define E1000_ICR_GPI_SDP2     (1u << 14) // General purpose interrupt on SDP2
#define E1000_ICR_GPI_SDP3     (1u << 15) // General purpose interrupt on SDP3
#define E1000_ICR_TXD_LOW      (1u << 16) // Transmit descriptor low threshold hit
#define E1000_ICR_SRPD         (1u << 17) // Small receive packet detected
#define E1000_ICR_ACK          (1u << 20) // Receive ACK frame detected
#define E1000_ICR_MNG          (1u << 21) // Manageability event
#define E1000_ICR_DOCK         (1u << 22) // Dock/undock event
#define E1000_ICR_INT_ASSERTED (1u << 31) // Device has asserted interrupt

// receive descriptor
struct rdesc {
    uint64_t addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;

    void dump() {
        printf("rdsec %p: addr %#llx len %hu cksum %#hx stat %#hhx err %#hhx spec %#hx\n", this,
               addr, length, checksum, status, errors, special);
    }
};
static_assert(sizeof(rdesc) == 16, "");

// Legacy RX descriptor status bits (rdesc.status)
#define E1000_RXD_STAT_DD    (1u << 0) // Descriptor done
#define E1000_RXD_STAT_EOP   (1u << 1) // End of packet
#define E1000_RXD_STAT_IXSM  (1u << 2) // Ignore checksum indication
#define E1000_RXD_STAT_VP    (1u << 3) // 802.1Q VLAN packet
#define E1000_RXD_STAT_UDPCS (1u << 4) // UDP checksum calculated on packet data
#define E1000_RXD_STAT_TCPCS (1u << 5) // TCP checksum calculated on packet data
#define E1000_RXD_STAT_IPCS  (1u << 6) // IP checksum calculated on packet data
#define E1000_RXD_STAT_PIF   (1u << 7) // Passed inexact filter

// Legacy RX descriptor error bits (rdesc.errors)
#define E1000_RXD_ERR_CE   (1u << 0) // CRC or alignment error
#define E1000_RXD_ERR_SE   (1u << 1) // Symbol error
#define E1000_RXD_ERR_SEQ  (1u << 2) // Sequence error
#define E1000_RXD_ERR_CXE  (1u << 4) // Carrier extension error
#define E1000_RXD_ERR_TCPE (1u << 5) // TCP/UDP checksum error
#define E1000_RXD_ERR_IPE  (1u << 6) // IP checksum error
#define E1000_RXD_ERR_RXE  (1u << 7) // RX data error

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
        printf(
            "tdsec %p: addr %#llx len %hu cso %#hhx cmd %#hhx sta_rsv %#hhx css %#hhx spec %#hx\n",
            this, addr, length, cso, cmd, sta_rsv, css, special);
    }
};
static_assert(sizeof(tdesc) == 16, "");

// TX descriptor status bits (tdesc.sta_rsv)
#define E1000_TXD_STAT_DD (1u << 0) // Descriptor done (TX complete)
#define E1000_TXD_STAT_EC (1u << 1) // Excess collisions
#define E1000_TXD_STAT_LC (1u << 2) // Late collision
#define E1000_TXD_STAT_TU (1u << 3) // Transmit underrun

// efficient copy for rx/tx descriptors out/into uncached memory
template <typename T> inline void copy(T *_dst, const T *_src) {
    // only allow this for structs that are precisely 16 bytes long
    static_assert(sizeof(T) == 16, "");

    // treat as two 8 byte copies
    volatile uint64_t *dst = (volatile uint64_t *)_dst;
    const volatile uint64_t *src = (const volatile uint64_t *)_src;

    dst[0] = src[0];
    dst[1] = src[1];
}
