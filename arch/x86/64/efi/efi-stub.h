/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

/* Minimal firmware-facing UEFI definitions for the boot stub.
 *
 * Deliberately self-contained: lib/uefi implements the *provider* side of
 * UEFI (LK acting as firmware for a loaded .efi payload), so its tables
 * declare function pointers with the native SysV ABI. Calling real x86-64
 * firmware requires the Microsoft x64 calling convention, so every function
 * pointer here is declared EFIAPI. Only the small subset of services the
 * stub actually uses is typed; everything else is padded with void *.
 */

/* On x86-64 UEFI uses the MS x64 calling convention; other architectures
 * use their native ABI, so this becomes a no-op when this stub grows
 * arm64/riscv versions. */
#if ARCH_X86_64
#define EFIAPI __attribute__((ms_abi))
#else
#define EFIAPI
#endif

typedef void *efi_handle;
typedef uintptr_t efi_status;
typedef uint16_t efi_char16;

#define EFI_SUCCESS             0
#define EFI_ERR(x)              (((efi_status)1 << 63) | (x))
#define EFI_LOAD_ERROR          EFI_ERR(1)
#define EFI_INVALID_PARAMETER   EFI_ERR(2)
#define EFI_UNSUPPORTED         EFI_ERR(3)
#define EFI_BUFFER_TOO_SMALL    EFI_ERR(5)
#define EFI_NOT_FOUND           EFI_ERR(14)

typedef struct efi_guid {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
} efi_guid;

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    { 0x5b1b31a1, 0x9562, 0x11d2, { 0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    { 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } }
#define EFI_ACPI_20_TABLE_GUID \
    { 0x8868e871, 0xe4f1, 0x11d3, { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }
#define EFI_ACPI_10_TABLE_GUID \
    { 0xeb9d2d30, 0x2d88, 0x11d3, { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

/* memory */
enum efi_memory_type {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
};

enum efi_allocate_type {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
};

typedef struct efi_memory_descriptor {
    uint32_t type;
    uint32_t pad;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attributes;
} efi_memory_descriptor;

typedef struct efi_table_header {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
} efi_table_header;

/* simple text output: only OutputString is used */
typedef struct efi_simple_text_output {
    void *reset;
    efi_status (EFIAPI *output_string)(struct efi_simple_text_output *self,
                                       const efi_char16 *string);
    /* remaining members unused */
} efi_simple_text_output;

/* boot services: slots the stub does not call are untyped */
typedef struct efi_boot_services {
    efi_table_header hdr;
    void *raise_tpl;
    void *restore_tpl;
    efi_status (EFIAPI *allocate_pages)(int /* efi_allocate_type */ type,
                                        int /* efi_memory_type */ memory_type,
                                        size_t pages, uint64_t *memory);
    efi_status (EFIAPI *free_pages)(uint64_t memory, size_t pages);
    efi_status (EFIAPI *get_memory_map)(size_t *memory_map_size,
                                        efi_memory_descriptor *memory_map,
                                        size_t *map_key, size_t *descriptor_size,
                                        uint32_t *descriptor_version);
    void *allocate_pool;
    void *free_pool;
    void *create_event;
    void *set_timer;
    void *wait_for_event;
    void *signal_event;
    void *close_event;
    void *check_event;
    void *install_protocol_interface;
    void *reinstall_protocol_interface;
    void *uninstall_protocol_interface;
    efi_status (EFIAPI *handle_protocol)(efi_handle handle, const efi_guid *protocol,
                                         void **interface);
    void *reserved;
    void *register_protocol_notify;
    void *locate_handle;
    void *locate_device_path;
    void *install_configuration_table;
    void *load_image;
    void *start_image;
    void *exit;
    void *unload_image;
    efi_status (EFIAPI *exit_boot_services)(efi_handle image_handle, size_t map_key);
    void *get_next_monotonic_count;
    void *stall;
    efi_status (EFIAPI *set_watchdog_timer)(size_t timeout, uint64_t watchdog_code,
                                            size_t data_size,
                                            const efi_char16 *watchdog_data);
    void *connect_controller;
    void *disconnect_controller;
    void *open_protocol;
    void *close_protocol;
    void *open_protocol_information;
    void *protocols_per_handle;
    void *locate_handle_buffer;
    efi_status (EFIAPI *locate_protocol)(const efi_guid *protocol, void *registration,
                                         void **interface);
    /* remaining members unused */
} efi_boot_services;

typedef struct efi_configuration_table {
    efi_guid vendor_guid;
    void *vendor_table;
} efi_configuration_table;

typedef struct efi_system_table {
    efi_table_header hdr;
    const efi_char16 *firmware_vendor;
    uint32_t firmware_revision;
    efi_handle console_in_handle;
    void *con_in;
    efi_handle console_out_handle;
    efi_simple_text_output *con_out;
    efi_handle standard_error_handle;
    efi_simple_text_output *std_err;
    void *runtime_services;
    efi_boot_services *boot_services;
    size_t number_of_table_entries;
    efi_configuration_table *configuration_table;
} efi_system_table;

#define EFI_SYSTEM_TABLE_SIGNATURE 0x5453595320494249ULL /* "IBI SYST" */

typedef struct efi_loaded_image {
    uint32_t revision;
    efi_handle parent_handle;
    efi_system_table *system_table;
    efi_handle device_handle;
    void *file_path;
    void *reserved;
    uint32_t load_options_size;
    void *load_options;
    void *image_base;
    uint64_t image_size;
    int image_code_type;
    int image_data_type;
    void *unload;
} efi_loaded_image;

/* graphics output protocol */
typedef struct efi_gop_mode_info {
    uint32_t version;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t pixel_format;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
    uint32_t reserved_mask;
    uint32_t pixels_per_scan_line;
} efi_gop_mode_info;

enum efi_gop_pixel_format {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
};

typedef struct efi_gop_mode {
    uint32_t max_mode;
    uint32_t mode;
    efi_gop_mode_info *info;
    size_t size_of_info;
    uint64_t frame_buffer_base;
    size_t frame_buffer_size;
} efi_gop_mode;

typedef struct efi_gop {
    void *query_mode;
    void *set_mode;
    void *blt;
    efi_gop_mode *mode;
} efi_gop;

/* entry points implemented in efi-entry.S. image_base is the runtime
 * address the firmware loaded the image at and image_size the page-aligned
 * in-memory image size (including bss); both are computed by the asm entry,
 * where their materialization is unambiguous (see the comment there). */
efi_status efi_main(efi_handle image_handle, efi_system_table *systab,
                    uint64_t image_base, uint64_t image_size);
void efi_handoff(uint64_t image_base) __attribute__((noreturn));
