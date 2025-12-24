/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#define MULTIBOOT2_SUPPORT 0

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

/* from https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html */

/* header magic for multiboot header */
#define MULTIBOOT2_HEADER_MAGIC     0xE85250D6

/* magic number passed by multiboot-compliant boot loaders */
#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289


#define MULTIBOOT2_HEADER_TAG_ADDRESS                2
#define MULTIBOOT2_HEADER_TAG_INFORMATION_REQUEST    1
#define MULTIBOOT2_HEADER_TAG_ENTRY_ADDRESS          3
#define MULTIBOOT2_HEADER_TAG_FRAMEBUFFER            5
#define MULTIBOOT2_HEADER_TAG_END                    0

#define MULTIBOOT2_HEADER_TAG_OPTIONAL               1

#define MULTIBOOT2_ARCHITECTURE_I386                 0
#define MULTIBOOT2_ARCHITECTURE_MIPS32               4

#define MULTIBOOT2_LOAD_PREFERENCE_NONE              0
#define MULTIBOOT2_LOAD_PREFERENCE_LOW               1
#define MULTIBOOT2_LOAD_PREFERENCE_HIGH              2


#define MULTIBOOT2_TAG_TYPE_END               0
#define MULTIBOOT2_TAG_TYPE_CMDLINE           1
#define MULTIBOOT2_TAG_TYPE_BOOT_LOADER_NAME  2
#define MULTIBOOT2_TAG_TYPE_MODULE            3
#define MULTIBOOT2_TAG_TYPE_BASIC_MEMINFO     4
#define MULTIBOOT2_TAG_TYPE_BOOTDEV           5
#define MULTIBOOT2_TAG_TYPE_MMAP              6
#define MULTIBOOT2_TAG_TYPE_VBE               7
#define MULTIBOOT2_TAG_TYPE_FRAMEBUFFER       8
#define MULTIBOOT2_TAG_TYPE_ELF_SECTIONS      9
#define MULTIBOOT2_TAG_TYPE_APM               10
#define MULTIBOOT2_TAG_TYPE_EFI32             11
#define MULTIBOOT2_TAG_TYPE_EFI64             12
#define MULTIBOOT2_TAG_TYPE_SMBIOS            13
#define MULTIBOOT2_TAG_TYPE_ACPI_OLD          14
#define MULTIBOOT2_TAG_TYPE_ACPI_NEW          15
#define MULTIBOOT2_TAG_TYPE_NETWORK           16
#define MULTIBOOT2_TAG_TYPE_EFI_MMAP          17
#define MULTIBOOT2_TAG_TYPE_EFI_BS            18
#define MULTIBOOT2_TAG_TYPE_EFI32_IH          19
#define MULTIBOOT2_TAG_TYPE_EFI64_IH          20
#define MULTIBOOT2_TAG_TYPE_LOAD_BASE_ADDR    21

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

struct multiboot2_info {
    uint32_t total_size;
    uint32_t reserved;
};

struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot2_tag_efi64 {
    struct multiboot2_tag tag;
    uint64_t pointer;
};

struct multiboot2_tag_mmap {
    struct multiboot2_tag tag;
    uint32_t entry_size;
    uint32_t entry_version;
};

struct multiboot2_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
};

struct multiboot2_tag_framebuffer_common {
    struct multiboot2_tag tag;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint16_t reserved;
};

struct multiboot2_framebuffer_rgb_bitmasks {
    uint8_t framebuffer_red_field_position;
    uint8_t framebuffer_red_mask_size;
    uint8_t framebuffer_green_field_position;
    uint8_t framebuffer_green_mask_size;
    uint8_t framebuffer_blue_field_position;
    uint8_t framebuffer_blue_mask_size;
};

struct multiboot2_color {
    uint8_t red_value;
    uint8_t green_value;
    uint8_t blue_value;
};

struct multiboot2_framebuffer_indexes {
    uint16_t framebuffer_palette_num_colors;
    struct multiboot2_color framebuffer_palette[0];
};

struct multiboot2_tag_framebuffer {
    struct multiboot2_tag_framebuffer_common common;
    union {
        /*
        struct {
            uint16_t framebuffer_palette_num_colors;
            struct multiboot2_color framebuffer_palette[0];
        };
        */
        struct multiboot2_framebuffer_indexes indexes;
        /*
        struct {
            uint8_t framebuffer_red_field_position;
            uint8_t framebuffer_red_mask_size;
            uint8_t framebuffer_green_field_position;
            uint8_t framebuffer_green_mask_size;
            uint8_t framebuffer_blue_field_position;
            uint8_t framebuffer_blue_mask_size;
        };
        */
        struct multiboot2_framebuffer_rgb_bitmasks rgb_bitmasks;

        // The output is 8:8:8 on GRUB (R:G:B)
        /*
        struct {
            uint8_t red_value;
            uint8_t green_value;
            uint8_t blue_value;
        };
        */
    };
};

/*
* The RSDP is a copy not point to an address.
*/
/* ACPI old RSDP tag */
struct multiboot2_tag_old_acpi {
    struct multiboot2_tag tag;
    uint8_t rsdp[0];  /* variable size RSDP table */
};

/* ACPI new RSDP tag */
struct multiboot2_tag_new_acpi {
    struct multiboot2_tag tag;
    uint8_t rsdp[0];  /* variable size RSDP table */
};

#endif // ASSEMBLY

