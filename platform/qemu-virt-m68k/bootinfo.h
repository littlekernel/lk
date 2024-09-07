/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>

// parse bootinfo
struct bootinfo_item {
    uint16_t tag;
    uint16_t size;
    uint32_t data[0];
};

// bootinfo tags (from QEMU)
enum BOOTINFO_TAGS {
    BOOTINFO_TAG_END = 0,
    BOOTINFO_TAG_MACHTYPE = 1,
    BOOTINFO_TAG_CPUTYPE = 2,
    BOOTINFO_TAG_FPUTYPE = 3,
    BOOTINFO_TAG_MMUTYPE = 4,
    BOOTINFO_TAG_MEMCHUNK = 5,
    BOOTINFO_TAG_RAMDISK = 6,
    BOOTINFO_TAG_COMMAND_LINE = 7,
    BOOTINFO_TAG_RNG_SEED = 8,
    BOOTINFO_TAG_VIRT_QEMU_VERSION = 0x8000,
    BOOTINFO_TAG_VIRT_GF_PIC_BASE = 0x8001,
    BOOTINFO_TAG_VIRT_GF_RTC_BASE = 0x8002,
    BOOTINFO_TAG_VIRT_GF_TTY_BASE = 0x8003,
    BOOTINFO_TAG_VIRT_VIRTIO_BASE = 0x8004,
    BOOTINFO_TAG_VIRT_CTRL_BASE = 0x8005,
};

void dump_all_bootinfo_records(void);
const void *bootinfo_find_record(uint16_t id, uint16_t *size_out);

// for BOOTINFO_TAG_*TYPE tags
struct bootinfo_item_type {
    uint32_t type;
};

// for BOOTINFO_TAG_VIRT_QEMU_VERSION
struct bootinfo_item_qemu_version {
    uint8_t major;
    uint8_t minor;
    uint8_t micro;
    uint8_t unused__;
};

// for BOOTINFO_TAG_MEMCHUNK
struct bootinfo_item_memchunk {
    uint32_t base;
    uint32_t size;
};

// for VIRT_*_BASE tags
struct bootinfo_item_device {
    uint32_t base;
    uint32_t irq_base;
};