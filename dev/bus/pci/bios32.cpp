/*
 * Copyright (c) 2009 Corey Tabaka
 * Copyright (c) 2020 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "bios32.h"

#include <lk/debug.h>
#include <lk/err.h>
#include <stdlib.h>
#include <string.h>
#include <kernel/thread.h>
#include <kernel/spinlock.h>
#include <dev/bus/pci.h>
#include <lk/trace.h>

#include "pci_priv.h"

#if ARCH_X86_32
// Only actually supported on x86-32

#include <arch/x86/descriptor.h>

#define LOCAL_TRACE 0

#define PCIBIOS_PRESENT                 0xB101
#define PCIBIOS_FIND_PCI_DEVICE         0xB102
#define PCIBIOS_FIND_PCI_CLASS_CODE     0xB103
#define PCIBIOS_GENERATE_SPECIAL_CYCLE  0xB106
#define PCIBIOS_READ_CONFIG_BYTE        0xB108
#define PCIBIOS_READ_CONFIG_WORD        0xB109
#define PCIBIOS_READ_CONFIG_DWORD       0xB10A
#define PCIBIOS_WRITE_CONFIG_BYTE       0xB10B
#define PCIBIOS_WRITE_CONFIG_WORD       0xB10C
#define PCIBIOS_WRITE_CONFIG_DWORD      0xB10D
#define PCIBIOS_GET_IRQ_ROUTING_OPTIONS 0xB10E
#define PCIBIOS_PCI_SET_IRQ_HW_INT      0xB10F

#define PCIBIOS_SUCCESSFUL              0x00
#define PCIBIOS_FUNC_NOT_SUPPORTED      0x81
#define PCIBIOS_BAD_VENDOR_ID           0x83
#define PCIBIOS_DEVICE_NOT_FOUND        0x86
#define PCIBIOS_BAD_REGISTER_NUMBER     0x87
#define PCIBIOS_SET_FAILED              0x88
#define PCIBIOS_BUFFER_TOO_SMALL        0x89

/*
 * BIOS32 entry header
 */
typedef struct {
    uint8_t magic[4];   // "_32_"
    void *entry;        // entry point
    uint8_t revision;
    uint8_t length;
    uint8_t checksum;
    uint8_t reserved[5];
} __PACKED pci_bios_info;

/*
 * scan for pci bios
 */
static const char *pci_bios_magic = "_32_";
static pci_bios_info *find_pci_bios_info(void) {
    uint32_t *head = (uint32_t *) (0x000e0000 + KERNEL_BASE);
    int8_t sum, *b;
    uint i;

    while (head < (uint32_t *) (0x000ffff0 + KERNEL_BASE)) {
        if (*head == *(uint32_t *) pci_bios_magic) {
            // perform the checksum
            sum = 0;
            b = (int8_t *) head;
            for (i=0; i < sizeof(pci_bios_info); i++) {
                sum += b[i];
            }

            if (sum == 0) {
                return (pci_bios_info *) head;
            }
        }

        head += 4;
    }

    return NULL;
}

/*
 * local BIOS32 PCI routines
 */
static const char *pci_signature = "PCI ";

// new C++ version
pci_bios32 *pci_bios32::detect() {
    LTRACE_ENTRY;

    pci_bios_info *pci = find_pci_bios_info();
    if (!pci) {
        return nullptr;
    }

    dprintf(INFO, "PCI: found BIOS32 structure at %p\n", pci);

    LTRACEF("BIOS32 header info:\n");
    LTRACEF("magic '%c%c%c%c' entry %p len %d checksum %#hhx\n",
            pci->magic[0], pci->magic[1], pci->magic[2], pci->magic[3],
            pci->entry, pci->length * 16, pci->checksum);

    uint32_t adr, temp, len;
    uint8_t err;

    bios32_entry b32_entry;
    b32_entry.offset = (uint32_t)(uintptr_t)pci->entry + KERNEL_BASE;
    b32_entry.selector = CODE_SELECTOR;

    __asm__(
        "lcall *(%%edi)"
        : "=a"(err),    /* AL out=status */
        "=b"(adr),    /* EBX out=code segment base adr */
        "=c"(len),    /* ECX out=code segment size */
        "=d"(temp)    /* EDX out=entry pt offset in code */
        : "0"(0x49435024),/* EAX in=service="$PCI" */
        "1"(0),   /* EBX in=0=get service entry pt */
        "D"(&b32_entry)
        : "cc", "memory"
    );

    if (err == 0x80) {
        dprintf(INFO, "BIOS32 found, but no PCI BIOS\n");
        return nullptr;
    }

    if (err != 0) {
        dprintf(INFO, "BIOS32 call to locate PCI BIOS returned %x\n", err);
        return nullptr;
    }

    LTRACEF("BIOS32 entry segment base %#x offset %#x\n", adr, temp);

    b32_entry.offset = adr + temp + KERNEL_BASE;

    // now call PCI_BIOS_PRESENT to get version, hw mechanism, and last bus
    uint16_t present, version, busses;
    uint32_t signature;
    __asm__(
        "lcall *(%%edi)		\n\t"
        "jc 1f				\n\t"
        "xor %%ah,%%ah		\n"
        "1:"
        : "=a"(present),
        "=b"(version),
        "=c"(busses),
        "=d"(signature)
        : "0"(PCIBIOS_PRESENT),
        "D"(&b32_entry)
        : "cc", "memory"
    );
    LTRACEF("PCI_BIOS_PRESENT returns present %#x, version %#x, busses %#x, signature %#x\n",
            present, version, busses, signature);

    if (present & 0xff00) {
        dprintf(INFO, "PCI_BIOS_PRESENT call returned ah=%#02x\n", present >> 8);
        return nullptr;
    }

    if (signature != *(uint32_t *)pci_signature) {
        dprintf(INFO, "PCI_BIOS_PRESENT call returned edx=%#08x\n", signature);
        return nullptr;
    }

    int last_bus = busses & 0xff;

    auto b32 = new pci_bios32(b32_entry);
    b32->set_last_bus(last_bus);

    return b32;
}

int pci_bios32::find_pci_device(pci_location_t *state, uint16_t device_id, uint16_t vendor_id, uint16_t index) {
    uint32_t bx, ret;

    __asm__(
        "lcall *(%%edi)		\n\t"
        "jc 1f				\n\t"
        "xor %%ah,%%ah		\n"
        "1:"
        : "=b"(bx),
        "=a"(ret)
        : "1"(PCIBIOS_FIND_PCI_DEVICE),
        "c"(device_id),
        "d"(vendor_id),
        "S"(index),
        "D"(&bios32_entry_)
        : "cc", "memory");

    state->bus = bx >> 8;
    state->dev_fn = bx & 0xFF;

    ret >>= 8;
    return ret & 0xFF;
}

int pci_bios32::find_pci_class_code(pci_location_t *state, uint32_t class_code, uint16_t index) {
    uint32_t bx, ret;

    __asm__(
        "lcall *(%%edi)			\n\t"
        "jc 1f					\n\t"
        "xor %%ah,%%ah			\n"
        "1:"
        : "=b"(bx),
        "=a"(ret)
        : "1"(PCIBIOS_FIND_PCI_CLASS_CODE),
        "c"(class_code),
        "S"(index),
        "D"(&bios32_entry_)
        : "cc", "memory");

    state->bus = bx >> 8;
    state->dev_fn = bx & 0xFF;

    ret >>= 8;
    return ret & 0xFF;
}

int pci_bios32::read_config_byte(const pci_location_t *state, uint32_t reg, uint8_t *value) {
    uint32_t bx, ret;

    bx = state->bus;
    bx <<= 8;
    bx |= state->dev_fn;
    __asm__(
        "lcall *(%%esi)			\n\t"
        "jc 1f					\n\t"
        "xor %%ah,%%ah			\n"
        "1:"
        : "=c"(*value),
        "=a"(ret)
        : "1"(PCIBIOS_READ_CONFIG_BYTE),
        "b"(bx),
        "D"(reg),
        "S"(&bios32_entry_)
        : "cc", "memory");
    ret >>= 8;
    return ret & 0xFF;
}

int pci_bios32::read_config_half(const pci_location_t *state, uint32_t reg, uint16_t *value) {
    uint32_t bx, ret;

    bx = state->bus;
    bx <<= 8;
    bx |= state->dev_fn;
    __asm__(
        "lcall *(%%esi)			\n\t"
        "jc 1f					\n\t"
        "xor %%ah,%%ah			\n"
        "1:"
        : "=c"(*value),
        "=a"(ret)
        : "1"(PCIBIOS_READ_CONFIG_WORD),
        "b"(bx),
        "D"(reg),
        "S"(&bios32_entry_)
        : "cc", "memory");
    ret >>= 8;
    return ret & 0xFF;
}

int pci_bios32::read_config_word(const pci_location_t *state, uint32_t reg, uint32_t *value) {
    uint32_t bx, ret;

    bx = state->bus;
    bx <<= 8;
    bx |= state->dev_fn;
    __asm__(
        "lcall *(%%esi)			\n\t"
        "jc 1f					\n\t"
        "xor %%ah,%%ah			\n"
        "1:"
        : "=c"(*value),
        "=a"(ret)
        : "1"(PCIBIOS_READ_CONFIG_DWORD),
        "b"(bx),
        "D"(reg),
        "S"(&bios32_entry_)
        : "cc", "memory");
    ret >>= 8;
    return ret & 0xFF;
}

int pci_bios32::write_config_byte(const pci_location_t *state, uint32_t reg, uint8_t value) {
    uint32_t bx, ret;

    bx = state->bus;
    bx <<= 8;
    bx |= state->dev_fn;
    __asm__(
        "lcall *(%%esi)			\n\t"
        "jc 1f					\n\t"
        "xor %%ah,%%ah			\n"
        "1:"
        : "=a"(ret)
        : "0"(PCIBIOS_WRITE_CONFIG_BYTE),
        "c"(value),
        "b"(bx),
        "D"(reg),
        "S"(&bios32_entry_)
        : "cc", "memory");
    ret >>= 8;
    return ret & 0xFF;
}

int pci_bios32::write_config_half(const pci_location_t *state, uint32_t reg, uint16_t value) {
    uint32_t bx, ret;

    bx = state->bus;
    bx <<= 8;
    bx |= state->dev_fn;
    __asm__(
        "lcall *(%%esi)	\n\t"
        "jc 1f					\n\t"
        "xor %%ah,%%ah			\n"
        "1:"
        : "=a"(ret)
        : "0"(PCIBIOS_WRITE_CONFIG_WORD),
        "c"(value),
        "b"(bx),
        "D"(reg),
        "S"(&bios32_entry_)
        : "cc", "memory");
    ret >>= 8;
    return ret & 0xFF;
}

int pci_bios32::write_config_word(const pci_location_t *state, uint32_t reg, uint32_t value) {
    uint32_t bx, ret;

    bx = state->bus;
    bx <<= 8;
    bx |= state->dev_fn;
    __asm__(
        "lcall *(%%esi)			\n\t"
        "jc 1f					\n\t"
        "xor %%ah,%%ah			\n"
        "1:"
        : "=a"(ret)
        : "0"(PCIBIOS_WRITE_CONFIG_DWORD),
        "c"(value),
        "b"(bx),
        "D"(reg),
        "S"(&bios32_entry_)
        : "cc", "memory");
    ret >>= 8;
    return ret & 0xFF;
}

int pci_bios32::get_irq_routing_options(irq_routing_options_t *options, uint16_t *pci_irqs) {
    uint32_t ret;

    __asm__(
        "lcall *(%%esi)			\n\t"
        "jc 1f					\n\t"
        "xor %%ah,%%ah			\n"
        "1:"
        : "=b"(*pci_irqs),
        "=a"(ret)
        : "1"(PCIBIOS_GET_IRQ_ROUTING_OPTIONS),
        "b"(0),
        "D"(options),
        "S"(&bios32_entry_)
        : "cc", "memory");
    ret >>= 8;
    return ret & 0xff;
}

int pci_bios32::set_irq_hw_int(const pci_location_t *state, uint8_t int_pin, uint8_t irq) {
    uint32_t bx, cx, ret;

    bx = state->bus;
    bx <<= 8;
    bx |= state->dev_fn;
    cx = irq;
    cx <<= 8;
    cx |= int_pin;
    __asm__(
        "lcall *(%%esi)			\n\t"
        "jc 1f					\n\t"
        "xor %%ah,%%ah			\n"
        "1:"
        : "=a"(ret)
        : "0"(PCIBIOS_PCI_SET_IRQ_HW_INT),
        "b"(bx),
        "c"(cx),
        "S"(&bios32_entry_)
        : "cc", "memory");
    ret >>= 8;
    return ret & 0xFF;
}

#endif
