/*
 * Copyright (c) 2009 Corey Tabaka
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __PLATFORM_MULTIBOOT_H
#define __PLATFORM_MULTIBOOT_H

#include <sys/types.h>

/* magic number for multiboot header */
#define MULTIBOOT_HEADER_MAGIC      0x1BADB002

/* flags for multiboot header */
#ifdef __ELF__
#define MULTIBOOT_HEADER_FLAGS      0x00000003
#else
#define MULTIBOOT_HEADER_FLAGS      0x00010003
#endif

/* magic number passed by multiboot-compliant boot loaders */
#define MULTIBOOT_BOOTLOADER_MAGIC  0x2BADB002

#ifndef ASSEMBLY

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
} multiboot_info_t;

enum {
    MB_INFO_MEM_SIZE    = 0x001,
    MB_INFO_BOOT_DEV    = 0x002,
    MB_INFO_CMD_LINE    = 0x004,
    MB_INFO_MODS        = 0x008,
    MB_INFO_SYMS        = 0x010,
    MB_INFO_MMAP        = 0x020,
    MB_INFO_DRIVES      = 0x040,
    MB_INFO_CONFIG      = 0x080,
    MB_INFO_BOOT_LOADER = 0x100,
    MB_INFO_APM_TABLE   = 0x200,
    MB_INFO_VBE         = 0x400,
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
};

#endif

#endif
