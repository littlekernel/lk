/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

/* from https://www.gnu.org/software/grub/manual/multiboot/multiboot.html */

/* magic number for multiboot header */
#define MULTIBOOT_HEADER_MAGIC      0x1BADB002

/* magic number passed by multiboot-compliant boot loaders */
#define MULTIBOOT_BOOTLOADER_MAGIC  0x2BADB002

/* Alignment of multiboot modules. */
#define MULTIBOOT_MOD_ALIGN         0x00001000

/* Alignment of the multiboot info structure. */
#define MULTIBOOT_INFO_ALIGN        0x00000004

/* Flags set in the ’flags’ member of the multiboot header. */

/* Align all boot modules on i386 page (4KB) boundaries. */
#define MULTIBOOT_PAGE_ALIGN        0x00000001

/* Must pass memory information to OS. */
#define MULTIBOOT_MEMORY_INFO       0x00000002

/* Must pass video information to OS. */
#define MULTIBOOT_VIDEO_MODE        0x00000004

/* This flag indicates the use of the address fields in the header. */
#define MULTIBOOT_AOUT_KLUDGE       0x00010000

#ifndef ASSEMBLY

#include <sys/types.h>
#include <assert.h>

/* multiboot header */
typedef struct multiboot_header {
    uint32_t magic;
    uint32_t flags;
    uint32_t checksum;
    uint32_t header_addr;
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
    uint32_t entry_addr;
} multiboot_header_t;

/* symbol table for a.out */
typedef struct aout_symbol_table {
    uint32_t tabsize;
    uint32_t strsize;
    uint32_t addr;
    uint32_t reserved;
} aout_symbol_table_t;

/* section header table for ELF */
typedef struct elf_section_header_table {
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
} elf_section_header_table_t;

/* multiboot info */
typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    union {
        aout_symbol_table_t aout_sym;
        elf_section_header_table_t elf_sec;
    } u;

    uint32_t mmap_length;
    uint32_t mmap_addr;

    uint32_t drives_length;
    uint32_t drives_addr;

    uint32_t config_table;

    uint32_t boot_loader_name;

    uint32_t apm_table;

    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    union {
        struct {
            uint32_t framebuffer_palette_addr;
            uint16_t framebuffer_palette_num_colors;
        };
        struct {
            uint8_t framebuffer_red_field_position;
            uint8_t framebuffer_red_mask_size;
            uint8_t framebuffer_green_field_position;
            uint8_t framebuffer_green_mask_size;
            uint8_t framebuffer_blue_field_position;
            uint8_t framebuffer_blue_mask_size;
        };
    };
} multiboot_info_t;

enum {
    MB_INFO_MEM_SIZE    = 0x001,
    MB_INFO_BOOT_DEV    = 0x002,
    MB_INFO_CMD_LINE    = 0x004,
    MB_INFO_MODS        = 0x008,
    MB_INFO_SYMS        = 0x010,
    MB_INFO_MMAP        = 0x040,
    MB_INFO_DRIVES      = 0x080,
    MB_INFO_CONFIG      = 0x100,
    MB_INFO_BOOT_LOADER = 0x200,
    MB_INFO_APM_TABLE   = 0x400,
    MB_INFO_VBE         = 0x800,
    MB_INFO_FRAMEBUFFER = 0x1000,
};

/* module structure */
typedef struct module {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;
    uint32_t reserved;
} module_t;

/* memory map - be careful that the offset 0 is base_addr_low without size */
typedef struct memory_map {
    uint32_t size;
    uint32_t base_addr_low;
    uint32_t base_addr_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
} memory_map_t;

/* memory map entry types */
enum {
    MB_MMAP_TYPE_AVAILABLE      = 0x01,
    MB_MMAP_TYPE_RESERVED       = 0x02,
    MB_MMAP_TYPE_ACPI_RECLAIM   = 0x03,
    MB_MMAP_TYPE_ACPI_NVS       = 0x04,
    MB_MMAP_TYPE_BADRAM         = 0x05,
};

/* framebuffer types */
enum {
    MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED  = 0,
    MULTIBOOT_FRAMEBUFFER_TYPE_RGB      = 1,
    MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT = 2,
};

#endif // ASSEMBLY

