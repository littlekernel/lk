/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <arch/x86/descriptor.h>

#include <assert.h>
#include <lk/compiler.h>

extern uint64_t _gdt[];

void x86_set_gdt_descriptor(seg_sel_t sel, void *base, uint32_t limit,
                     uint8_t present, uint8_t ring, uint8_t sys, uint8_t type, uint8_t gran, uint8_t bits) {
    typedef struct {
        struct {
            uint16_t limit_15_0;
            uint16_t base_15_0;
            uint8_t base_23_16;

            uint8_t type : 4;
            uint8_t s : 1;
            uint8_t dpl : 2;
            uint8_t p : 1;

            uint8_t limit_19_16 : 4;
            uint8_t avl : 1;
            uint8_t reserved : 1;
            uint8_t d_b : 1;
            uint8_t g : 1;

            uint8_t base_31_24;
        } seg_desc_legacy;

#if ARCH_X86_64
        // some descriptors have additional fields for x86-64
        struct {
            uint32_t base_63_32;
            uint32_t reserved;
        } seg_desc_64;
#endif
    } seg_desc_t;

#if ARCH_X86_64
    static_assert(sizeof(seg_desc_t) == 16, "seg_desc_t size mismatch");
#else
    static_assert(sizeof(seg_desc_t) == 8, "seg_desc_t size mismatch");
#endif

    seg_desc_t desc = {0};

    desc.seg_desc_legacy.limit_15_0  = limit & 0x0000ffff;
    desc.seg_desc_legacy.limit_19_16 = (limit & 0x000f0000) >> 16;

    desc.seg_desc_legacy.base_15_0   = ((uintptr_t) base) & 0x0000ffff;
    desc.seg_desc_legacy.base_23_16  = (((uintptr_t) base) & 0x00ff0000) >> 16;
    desc.seg_desc_legacy.base_31_24  = ((uintptr_t) base) >> 24;

    desc.seg_desc_legacy.type    = type & 0x0f;  // segment type
    desc.seg_desc_legacy.s       = sys != 0;     // system / non-system
    desc.seg_desc_legacy.dpl     = ring & 0x03;  // descriptor privilege level
    desc.seg_desc_legacy.p       = present != 0; // present
    desc.seg_desc_legacy.avl     = 0;
    desc.seg_desc_legacy.reserved = 0;
    desc.seg_desc_legacy.d_b     = bits != 0;    // 16 / 32 bit
    desc.seg_desc_legacy.g       = gran != 0;    // granularity

    // convert selector into index, which are always 8 byte indexed
    uint16_t index = sel >> 3;
    seg_desc_t *entry = (seg_desc_t *)&_gdt[index];
    entry->seg_desc_legacy = desc.seg_desc_legacy;

#ifdef ARCH_X86_64
    if (sys == 0) {
        // some of the system descriptors have two more words
        switch (type) {
            case SEG_TYPE_TSS:
            case SEG_TYPE_TSS_BUSY:
            case SEG_TYPE_LDT:
            case SEG_TYPE_CALL_GATE:
                // copy the lower 32 bits of the descriptor (base and limit)
                desc.seg_desc_64.base_63_32 = (uint32_t)((uintptr_t) base >> 32);
                desc.seg_desc_64.reserved = 0;

                // copy the upper 64 bits of the descriptor
                entry->seg_desc_64 = desc.seg_desc_64;
                break;
        }
    }
#endif
}
