/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stdint.h>
#include <lk/compiler.h>

/* Boot information recorded by the in-kernel UEFI stub before it calls
 * ExitBootServices and hands off to the regular boot path. Deliberately
 * arch-neutral: other architectures' stubs are expected to fill the same
 * structure.
 *
 * The instance lives in the kernel image's .data section (it must survive
 * the bss clear in early boot) and travels with the image when the stub
 * relocates it to its link address. Platform code checks magic to decide
 * whether the kernel was entered via UEFI.
 */

#define LK_EFI_BOOT_MAGIC   0x4c4b4546U  /* 'LKEF' */
#define LK_EFI_BOOT_VERSION 1

/* raw EFI memory map buffer; OVMF maps run 6-12KB, leave headroom */
#define LK_EFI_MMAP_BUF_SIZE (16*1024)

#define LK_EFI_CMDLINE_LEN 256

__BEGIN_CDECLS

/* layout of the entries in the mmap buffer below: EFI_MEMORY_DESCRIPTOR.
 * number_of_pages is always in 4KB units, regardless of kernel page size. */
struct lk_efi_memory_descriptor {
    uint32_t type;
    uint32_t pad;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attributes;
};

#define LK_EFI_PAGE_SIZE 4096

/* EFI_MEMORY_TYPE values consumers care about */
#define LK_EFI_MEM_LOADER_CODE          1
#define LK_EFI_MEM_LOADER_DATA          2
#define LK_EFI_MEM_BOOT_SERVICES_CODE   3
#define LK_EFI_MEM_BOOT_SERVICES_DATA   4
#define LK_EFI_MEM_CONVENTIONAL         7
#define LK_EFI_MEM_ACPI_RECLAIM         9

struct lk_efi_boot_info {
    uint32_t magic;
    uint32_t version;

    uint64_t efi_system_table;  /* physical address */
    uint64_t image_base;        /* where the firmware loaded the image (diagnostic) */
    uint64_t rsdp_phys;         /* ACPI RSDP from the EFI config table, 0 if none */

    /* raw EFI memory map, captured at ExitBootServices time. Walk it in
     * mmap_desc_size steps: firmware descriptors may be larger than the
     * struct definition. */
    uint32_t mmap_desc_size;
    uint32_t mmap_desc_version;
    uint32_t mmap_entries;

    /* GOP framebuffer, if the firmware provided one */
    uint8_t  fb_present;
    uint8_t  fb_bpp;
    uint16_t _pad0;
    uint64_t fb_base;
    uint32_t fb_pitch;          /* bytes per row */
    uint32_t fb_width;
    uint32_t fb_height;
    uint32_t fb_pixel_format;   /* raw EFI_GRAPHICS_PIXEL_FORMAT value */

    /* kernel command line from EFI LoadOptions, converted to ASCII */
    char cmdline[LK_EFI_CMDLINE_LEN];

    uint8_t mmap[LK_EFI_MMAP_BUF_SIZE];
};

extern struct lk_efi_boot_info lk_efi_boot_info;

__END_CDECLS
