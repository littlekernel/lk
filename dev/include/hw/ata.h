//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
#pragma once

#include <stdint.h>

// portions from https://wiki.osdev.org/AHCI
//
// TODO: reconsider use of bitfields, endian

// disable clang-tidy warnings about using vs typedef enum for C compatibility
// NOLINTBEGIN(modernize-use-using)

typedef enum {
    FIS_TYPE_REG_H2D = 0x27,   // Register FIS - host to device
    FIS_TYPE_REG_D2H = 0x34,   // Register FIS - device to host
    FIS_TYPE_DMA_ACT = 0x39,   // DMA activate FIS - device to host
    FIS_TYPE_DMA_SETUP = 0x41, // DMA setup FIS - bidirectional
    FIS_TYPE_DATA = 0x46,      // Data FIS - bidirectional
    FIS_TYPE_BIST = 0x58,      // BIST activate FIS - bidirectional
    FIS_TYPE_PIO_SETUP = 0x5F, // PIO setup FIS - device to host
    FIS_TYPE_DEV_BITS = 0xA1,  // Set device bits FIS - device to host
} FIS_TYPE;

typedef enum {
    ATA_CMD_FLUSH_CACHE = 0xe7,
    ATA_CMD_FLUSH_CACHE_EXT = 0xea,
    ATA_CMD_READ_DMA_EXT = 0x25,
    ATA_CMD_WRITE_DMA_EXT = 0x35,
    ATA_CMD_IDENTIFY = 0xec,
    ATA_CMD_PACKET = 0xa0,
    ATA_CMD_PACKET_IDENTIFY = 0xa1,
} ATA_CMD;

typedef struct tagFIS_REG_H2D {
    // DWORD 0
    uint8_t fis_type; // FIS_TYPE_REG_H2D

    uint8_t pmport : 4; // Port multiplier
    uint8_t rsv0 : 3;   // Reserved
    uint8_t c : 1;      // 1: Command, 0: Control

    uint8_t command;  // Command register
    uint8_t featurel; // Feature register, 7:0

    // DWORD 1
    uint8_t lba0;   // LBA low register, 7:0
    uint8_t lba1;   // LBA mid register, 15:8
    uint8_t lba2;   // LBA high register, 23:16
    uint8_t device; // Device register

    // DWORD 2
    uint8_t lba3;     // LBA register, 31:24
    uint8_t lba4;     // LBA register, 39:32
    uint8_t lba5;     // LBA register, 47:40
    uint8_t featureh; // Feature register, 15:8

    // DWORD 3
    uint8_t countl;  // Count register, 7:0
    uint8_t counth;  // Count register, 15:8
    uint8_t icc;     // Isochronous command completion
    uint8_t control; // Control register

    // DWORD 4
    uint8_t rsv1[4]; // Reserved
} FIS_REG_H2D;

typedef struct tagFIS_REG_D2H {
    // DWORD 0
    uint8_t fis_type; // FIS_TYPE_REG_D2H

    uint8_t pmport : 4; // Port multiplier
    uint8_t rsv0 : 2;   // Reserved
    uint8_t i : 1;      // Interrupt bit
    uint8_t rsv1 : 1;   // Reserved

    uint8_t status; // Status register
    uint8_t error;  // Error register

    // DWORD 1
    uint8_t lba0;   // LBA low register, 7:0
    uint8_t lba1;   // LBA mid register, 15:8
    uint8_t lba2;   // LBA high register, 23:16
    uint8_t device; // Device register

    // DWORD 2
    uint8_t lba3; // LBA register, 31:24
    uint8_t lba4; // LBA register, 39:32
    uint8_t lba5; // LBA register, 47:40
    uint8_t rsv2; // Reserved

    // DWORD 3
    uint8_t countl;  // Count register, 7:0
    uint8_t counth;  // Count register, 15:8
    uint8_t rsv3[2]; // Reserved

    // DWORD 4
    uint8_t rsv4[4]; // Reserved
} FIS_REG_D2H;

typedef struct tagFIS_DATA {
    // DWORD 0
    uint8_t fis_type; // FIS_TYPE_DATA

    uint8_t pmport : 4; // Port multiplier
    uint8_t rsv0 : 4;   // Reserved

    uint8_t rsv1[2]; // Reserved

    // DWORD 1 ~ N
    uint32_t data[1]; // Payload
} FIS_DATA;

typedef struct tagFIS_PIO_SETUP {
    // DWORD 0
    uint8_t fis_type; // FIS_TYPE_PIO_SETUP

    uint8_t pmport : 4; // Port multiplier
    uint8_t rsv0 : 1;   // Reserved
    uint8_t d : 1;      // Data transfer direction, 1 - device to host
    uint8_t i : 1;      // Interrupt bit
    uint8_t rsv1 : 1;

    uint8_t status; // Status register
    uint8_t error;  // Error register

    // DWORD 1
    uint8_t lba0;   // LBA low register, 7:0
    uint8_t lba1;   // LBA mid register, 15:8
    uint8_t lba2;   // LBA high register, 23:16
    uint8_t device; // Device register

    // DWORD 2
    uint8_t lba3; // LBA register, 31:24
    uint8_t lba4; // LBA register, 39:32
    uint8_t lba5; // LBA register, 47:40
    uint8_t rsv2; // Reserved

    // DWORD 3
    uint8_t countl;   // Count register, 7:0
    uint8_t counth;   // Count register, 15:8
    uint8_t rsv3;     // Reserved
    uint8_t e_status; // New value of status register

    // DWORD 4
    uint16_t tc;     // Transfer count
    uint8_t rsv4[2]; // Reserved
} FIS_PIO_SETUP;

typedef struct tagFIS_DMA_SETUP {
    // DWORD 0
    uint8_t fis_type; // FIS_TYPE_DMA_SETUP

    uint8_t pmport : 4; // Port multiplier
    uint8_t rsv0 : 1;   // Reserved
    uint8_t d : 1;      // Data transfer direction, 1 - device to host
    uint8_t i : 1;      // Interrupt bit
    uint8_t a : 1;      // Auto-activate. Specifies if DMA Activate FIS is needed

    uint8_t rsved[2]; // Reserved

    // DWORD 1&2
    uint64_t DMAbufferID; // DMA Buffer Identifier. Used to Identify DMA buffer in host memory.
                          // SATA Spec says host specific and not in Spec. Trying AHCI spec might work.

    // DWORD 3
    uint32_t rsvd; // Reserved

    // DWORD 4
    uint32_t DMAbufOffset; // Byte offset into buffer. First 2 bits must be 0

    // DWORD 5
    uint32_t TransferCount; // Number of bytes to transfer. Bit 0 must be 0

    // DWORD 6
    uint32_t resvd; // Reserved
} FIS_DMA_SETUP;

// NOLINTEND(modernize-use-using)
