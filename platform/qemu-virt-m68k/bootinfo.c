/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "bootinfo.h"

#include <lk/compiler.h>
#include <lk/trace.h>
#include <lk/debug.h>
#include <stdio.h>

#define LOCAL_TRACE 0

extern uint8_t __bss_end;

static const char *bootinfo_tag_to_string(enum BOOTINFO_TAGS tag) {
    switch (tag) {
        case BOOTINFO_TAG_END: return "END";
        case BOOTINFO_TAG_MACHTYPE: return "MACHTYPE";
        case BOOTINFO_TAG_CPUTYPE: return "CPUTYPE";
        case BOOTINFO_TAG_FPUTYPE: return "FPUTYPE";
        case BOOTINFO_TAG_MMUTYPE: return "MMUTYPE";
        case BOOTINFO_TAG_MEMCHUNK: return "MEMCHUNK";
        case BOOTINFO_TAG_RAMDISK: return "RAMDISK";
        case BOOTINFO_TAG_COMMAND_LINE: return "COMMAND_LINE";
        case BOOTINFO_TAG_RNG_SEED: return "RNG_SEED";
        case BOOTINFO_TAG_VIRT_QEMU_VERSION: return "VIRT_QEMU_VERSION";
        case BOOTINFO_TAG_VIRT_GF_PIC_BASE: return "VIRT_GF_PIC_BASE";
        case BOOTINFO_TAG_VIRT_GF_RTC_BASE: return "VIRT_GF_RTC_BASE";
        case BOOTINFO_TAG_VIRT_GF_TTY_BASE: return "VIRT_GF_TTY_BASE";
        case BOOTINFO_TAG_VIRT_VIRTIO_BASE: return "VIRT_VIRTIO_BASE";
        case BOOTINFO_TAG_VIRT_CTRL_BASE: return "VIRT_CTRL_BASE";
        default: return "UNKNOWN";
    }
}

static void dump_bootinfo_record(const struct bootinfo_item *item) {
    printf("item %p: tag %hx (%s), size %hu\n", item, item->tag, bootinfo_tag_to_string(item->tag), item->size);
    if (item->size > 4) {
        hexdump8(item->data, item->size - 4);
    }
}

void dump_all_bootinfo_records(void) {
    const uint8_t *ptr = &__bss_end;

    printf("bootinfo records at %p:\n", ptr);
    for (;;) {
        const struct bootinfo_item *item = (const struct bootinfo_item *)ptr;
        if (item->tag == BOOTINFO_TAG_END) {
            break;
        }

        dump_bootinfo_record(item);

        // move to the next field
        ptr += item->size;
    }
}

// look for tags that qemu left at the end of the kernel that hold various
// pieces of system configuration info.
const void *bootinfo_find_record(uint16_t id, uint16_t *size_out) {
    const uint8_t *ptr = &__bss_end;

    *size_out = 0;
    for (;;) {
        const struct bootinfo_item *item = (const struct bootinfo_item *)ptr;
        if (item->tag == BOOTINFO_TAG_END) {
            return NULL;
        }

        if (LOCAL_TRACE > 2) {
            dump_bootinfo_record(item);
        }

        if (item->tag == id) {
            *size_out = item->size - 4;
            return item->data;
        } else if (item->tag == 0) { // end token
            return NULL;
        }

        // move to the next field
        ptr += item->size;
    }
}

