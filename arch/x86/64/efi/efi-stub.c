/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/* In-kernel UEFI boot stub.
 *
 * The firmware loads the flat kernel image (which carries a PE/COFF header,
 * see arch/x86/64/efi-header.S) at an address of its choosing, with paging
 * enabled and every range in the UEFI memory map identity mapped, and calls
 * efi_entry (efi-entry.S), which lands here. This code gathers everything
 * the kernel needs from the firmware (command line, framebuffer, ACPI RSDP,
 * memory map), reserves the kernel's link-time physical address range, exits
 * boot services, and jumps to efi_handoff, which copies the image to its
 * link address and enters the regular boot path at real_start.
 *
 * POSITION INDEPENDENCE RULES -- this module runs at whatever address the
 * firmware picked, long before the kernel runs at its link address:
 *  - Compiled -fpie -mcmodel=small (rules.mk); everything is rip-relative.
 *  - No calls to any code outside this module: the rest of the kernel is
 *    compiled -mcmodel=kernel -fno-pic and assumes it runs at its link
 *    address. That includes libc: local mem/str helpers only.
 *  - No pointer-valued initialized globals (they would need relocation).
 *  - Linker asserts .got is empty; keep it that way (hidden visibility,
 *    no jump tables).
 *  - Data recorded for the kernel goes into lk_efi_boot_info, which lives
 *    in .data and travels with the image copy.
 */

#include "efi-stub.h"

#include <stdbool.h>
#include <lk/efi_boot_info.h>

#define PHYS_LOAD_ADDRESS (MEMBASE + KERNEL_LOAD_OFFSET)
#define EFI_PAGE_SIZE 4096

/* NOTE: the image's runtime base and size come in as arguments from the asm
 * entry. Do not reference the linker-provided bounds symbols (__code_start
 * etc.) from C in this module: depending on compiler and linker relaxation
 * they materialize as either the runtime or the link-time address. */

/* the boot info handed to platform code. Explicitly placed in .data: a
 * zero-initialized global would land in .bss, which real_start clears. */
struct lk_efi_boot_info lk_efi_boot_info __attribute__((section(".data"))) = {
    .version = LK_EFI_BOOT_VERSION,
};

/* ---- tiny freestanding helpers (no libc calls allowed here) ---- */

static bool guid_eq(const efi_guid *a, const efi_guid *b) {
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    for (size_t i = 0; i < sizeof(efi_guid); i++) {
        if (pa[i] != pb[i]) return false;
    }
    return true;
}

/* print an ascii string via the firmware console (usable until
 * exit_boot_services succeeds) */
static void efi_puts(efi_system_table *systab, const char *str) {
    if (!systab->con_out || !systab->con_out->output_string) return;

    efi_char16 buf[64];
    size_t pos = 0;
    while (*str) {
        if (*str == '\n') {
            buf[pos++] = u'\r';
        }
        buf[pos++] = (efi_char16)(uint8_t)*str++;
        if (pos >= (sizeof(buf) / sizeof(buf[0])) - 2) {
            buf[pos] = u'\0';
            systab->con_out->output_string(systab->con_out, buf);
            pos = 0;
        }
    }
    buf[pos] = u'\0';
    if (pos > 0) {
        systab->con_out->output_string(systab->con_out, buf);
    }
}

static void efi_put_hex64(efi_system_table *systab, uint64_t val) {
    char buf[19];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 16; i++) {
        buf[2 + i] = "0123456789abcdef"[(val >> (60 - i * 4)) & 0xf];
    }
    buf[18] = '\0';
    efi_puts(systab, buf);
}

/* ---- firmware information gathering ---- */

static void gather_cmdline(efi_boot_services *bs, efi_handle image_handle) {
    static const efi_guid loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    void *interface = NULL;
    efi_status status = bs->handle_protocol(image_handle, &loaded_image_guid, &interface);
    if (status != EFI_SUCCESS || !interface) return;

    const efi_loaded_image *li = interface;
    const efi_char16 *opts = li->load_options;
    if (!opts || li->load_options_size < sizeof(efi_char16)) return;

    /* LoadOptions is conventionally a UCS-2 command line, but the boot
     * manager may hand over arbitrary binary data; bail on anything that
     * doesn't look like printable text. Note when run from the EFI shell
     * the string includes the image name as its first token. */
    size_t max_chars = li->load_options_size / sizeof(efi_char16);
    size_t out = 0;
    for (size_t i = 0; i < max_chars && out < LK_EFI_CMDLINE_LEN - 1; i++) {
        efi_char16 c = opts[i];
        if (c == u'\0') break;
        if (c < 0x20 || c > 0x7e) {
            lk_efi_boot_info.cmdline[0] = '\0';
            return;
        }
        lk_efi_boot_info.cmdline[out++] = (char)c;
    }
    lk_efi_boot_info.cmdline[out] = '\0';
}

static void gather_framebuffer(efi_boot_services *bs) {
    static const efi_guid gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

    void *interface = NULL;
    efi_status status = bs->locate_protocol(&gop_guid, NULL, &interface);
    if (status != EFI_SUCCESS || !interface) return;

    const efi_gop *gop = interface;
    const efi_gop_mode *mode = gop->mode;
    if (!mode || !mode->info || mode->frame_buffer_base == 0) return;

    const efi_gop_mode_info *info = mode->info;
    switch (info->pixel_format) {
        case PixelRedGreenBlueReserved8BitPerColor:
        case PixelBlueGreenRedReserved8BitPerColor:
            break;
        default:
            /* PixelBitMask would need mask plumbing, PixelBltOnly has no
             * linear framebuffer; skip both */
            return;
    }

    lk_efi_boot_info.fb_present = 1;
    lk_efi_boot_info.fb_base = mode->frame_buffer_base;
    lk_efi_boot_info.fb_width = info->horizontal_resolution;
    lk_efi_boot_info.fb_height = info->vertical_resolution;
    lk_efi_boot_info.fb_pitch = info->pixels_per_scan_line * 4;
    lk_efi_boot_info.fb_bpp = 32;
    lk_efi_boot_info.fb_pixel_format = info->pixel_format;
}

static void gather_rsdp(efi_system_table *systab) {
    static const efi_guid acpi20_guid = EFI_ACPI_20_TABLE_GUID;
    static const efi_guid acpi10_guid = EFI_ACPI_10_TABLE_GUID;

    void *rsdp10 = NULL;
    for (size_t i = 0; i < systab->number_of_table_entries; i++) {
        const efi_configuration_table *ct = &systab->configuration_table[i];
        if (guid_eq(&ct->vendor_guid, &acpi20_guid)) {
            lk_efi_boot_info.rsdp_phys = (uint64_t)(uintptr_t)ct->vendor_table;
            return;
        }
        if (guid_eq(&ct->vendor_guid, &acpi10_guid)) {
            rsdp10 = ct->vendor_table;
        }
    }
    lk_efi_boot_info.rsdp_phys = (uint64_t)(uintptr_t)rsdp10;
}

/* ---- main ---- */

efi_status efi_main(efi_handle image_handle, efi_system_table *systab,
                    uint64_t image_base, uint64_t image_size) {
    if (!systab || systab->hdr.signature != EFI_SYSTEM_TABLE_SIGNATURE ||
        !systab->boot_services) {
        return EFI_INVALID_PARAMETER;
    }
    efi_boot_services *bs = systab->boot_services;

    if (bs->set_watchdog_timer) {
        bs->set_watchdog_timer(0, 0, 0, NULL);
    }

    efi_puts(systab, "lk: UEFI boot stub, image loaded at ");
    efi_put_hex64(systab, image_base);
    efi_puts(systab, "\n");

    gather_cmdline(bs, image_handle);
    gather_framebuffer(bs);
    gather_rsdp(systab);
    lk_efi_boot_info.efi_system_table = (uint64_t)(uintptr_t)systab;
    lk_efi_boot_info.image_base = image_base;

    /* Reserve the kernel's link-time physical address range so nothing in
     * the firmware owns it; efi_handoff copies the image there after boot
     * services end. Allocated as LoaderCode: firmware keeps loader code
     * mapped writable and executable (Linux relies on the same behavior).
     * Cannot overlap the running image, or this allocation would have
     * failed. */
    uint64_t target = PHYS_LOAD_ADDRESS;
    efi_status status = bs->allocate_pages(AllocateAddress, EfiLoaderCode,
                                           image_size / EFI_PAGE_SIZE, &target);
    if (status != EFI_SUCCESS) {
        efi_puts(systab, "lk: physical range at ");
        efi_put_hex64(systab, PHYS_LOAD_ADDRESS);
        efi_puts(systab, " is not free, cannot boot\n");
        return EFI_LOAD_ERROR;
    }

    /* Fetch the memory map and exit boot services. Per spec, if the map
     * changes underneath us exit_boot_services fails with INVALID_PARAMETER
     * and we may only call get_memory_map (no allocations) and retry once. */
    size_t map_size = LK_EFI_MMAP_BUF_SIZE;
    size_t map_key = 0, desc_size = 0;
    uint32_t desc_version = 0;
    status = bs->get_memory_map(&map_size, (efi_memory_descriptor *)lk_efi_boot_info.mmap,
                                &map_key, &desc_size, &desc_version);
    if (status != EFI_SUCCESS) {
        efi_puts(systab, "lk: GetMemoryMap failed (buffer too small?)\n");
        return EFI_LOAD_ERROR;
    }

    status = bs->exit_boot_services(image_handle, map_key);
    if (status == EFI_INVALID_PARAMETER) {
        map_size = LK_EFI_MMAP_BUF_SIZE;
        status = bs->get_memory_map(&map_size, (efi_memory_descriptor *)lk_efi_boot_info.mmap,
                                    &map_key, &desc_size, &desc_version);
        if (status == EFI_SUCCESS) {
            status = bs->exit_boot_services(image_handle, map_key);
        }
    }
    if (status != EFI_SUCCESS) {
        /* firmware console is in an undefined state now; nothing to do */
        for (;;) {
            __asm__ volatile("cli; hlt");
        }
    }

    /* boot services are gone; finalize the handoff record */
    lk_efi_boot_info.mmap_desc_size = (uint32_t)desc_size;
    lk_efi_boot_info.mmap_desc_version = desc_version;
    lk_efi_boot_info.mmap_entries = (uint32_t)(map_size / desc_size);
    lk_efi_boot_info.magic = LK_EFI_BOOT_MAGIC;

    efi_handoff(image_base);
}
