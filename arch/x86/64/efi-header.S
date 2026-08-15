/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <hw/multiboot.h>

/* This file emits the first 4KB of the kernel image: a hand-rolled PE/COFF
 * header (so lk.bin doubles as a UEFI application, Linux EFI-stub style)
 * followed by the multiboot 1 and 2 headers. The real code in .text.boot
 * starts at file offset 0x1000.
 *
 * The PE fields that require cross-section arithmetic (__pe_entry_rva,
 * __pe_text_size, __pe_data_*, __pe_size_of_image) are computed as absolute
 * symbols in kernel.ld. File offsets equal RVAs: the flat lk.bin is mapped
 * 1:1, SizeOfHeaders and the .text RVA are both 0x1000.
 *
 * Firmware notes (validated against edk2's BasePeCoff.c):
 * - CheckSum is never verified by the reference loader; leave it 0.
 * - No .reloc section is needed. ImageBase is 0 and RELOCS_STRIPPED is
 *   deliberately NOT set: firmware loads the image at an address of its
 *   choosing and the EFI stub copes (it is position independent and
 *   relocates the kernel to its link address itself). Setting
 *   RELOCS_STRIPPED would make loads *fail* unless ImageBase happens to
 *   be free, not force the load address.
 */

#define PHYS_LOAD_ADDRESS (MEMBASE + KERNEL_LOAD_OFFSET)
#define PHYS_ADDR_DELTA (KERNEL_BASE + KERNEL_LOAD_OFFSET - PHYS_LOAD_ADDRESS)
#define PHYS(x) ((x) - PHYS_ADDR_DELTA)

/* IMAGE_FILE_HEADER.Characteristics */
#define PE_EXECUTABLE_IMAGE   0x0002
#define PE_LINE_NUMS_STRIPPED 0x0004
#define PE_DEBUG_STRIPPED     0x0200

/* section Characteristics */
#define PE_SCN_CNT_CODE             0x00000020
#define PE_SCN_CNT_INITIALIZED_DATA 0x00000040
#define PE_SCN_MEM_EXECUTE          0x20000000
#define PE_SCN_MEM_READ             0x40000000
#define PE_SCN_MEM_WRITE            0x80000000

#define PE_SUBSYSTEM_EFI_APPLICATION 10

.section ".text.boot.header"

/* loose DOS header: just the magic and the offset of the PE header at 0x3c.
 * .Lhead is at file offset 0 (this section is placed first, at __code_start);
 * the assembler needs a local anchor for the intra-file offset arithmetic. */
.Lhead:
    .ascii "MZ"
    .org 0x3c
    .long .Lpe_header - .Lhead                  /* e_lfanew */

.Lpe_header:
    .long 0x00004550                            /* "PE\0\0" */
.Lcoff_header:
    .short 0x8664                               /* Machine: x86-64 */
    .short 2                                    /* NumberOfSections */
    .long 0                                     /* TimeDateStamp */
    .long 0                                     /* PointerToSymbolTable */
    .long 0                                     /* NumberOfSymbols */
    .short .Lsection_table - .Lopt_header       /* SizeOfOptionalHeader */
    .short PE_EXECUTABLE_IMAGE | PE_LINE_NUMS_STRIPPED | PE_DEBUG_STRIPPED
                                                /* Characteristics: note
                                                 * RELOCS_STRIPPED absent */
.Lopt_header:
    .short 0x020b                               /* Magic: PE32+ */
    .byte 0                                     /* MajorLinkerVersion */
    .byte 0                                     /* MinorLinkerVersion */
    .long __pe_text_size                        /* SizeOfCode */
    .long __pe_data_raw_size                    /* SizeOfInitializedData */
    .long 0                                     /* SizeOfUninitializedData */
    .long __pe_entry_rva                        /* AddressOfEntryPoint */
    .long 0x1000                                /* BaseOfCode */
    .quad 0                                     /* ImageBase: load anywhere */
    .long 0x1000                                /* SectionAlignment */
    .long 0x1000                                /* FileAlignment */
    .short 0                                    /* MajorOperatingSystemVersion */
    .short 0                                    /* MinorOperatingSystemVersion */
    .short 0                                    /* MajorImageVersion */
    .short 0                                    /* MinorImageVersion */
    .short 0                                    /* MajorSubsystemVersion */
    .short 0                                    /* MinorSubsystemVersion */
    .long 0                                     /* Win32VersionValue */
    .long __pe_size_of_image                    /* SizeOfImage (covers bss) */
    .long 0x1000                                /* SizeOfHeaders */
    .long 0                                     /* CheckSum (never verified) */
    .short PE_SUBSYSTEM_EFI_APPLICATION         /* Subsystem */
    .short 0                                    /* DllCharacteristics */
    .quad 0                                     /* SizeOfStackReserve */
    .quad 0                                     /* SizeOfStackCommit */
    .quad 0                                     /* SizeOfHeapReserve */
    .quad 0                                     /* SizeOfHeapCommit */
    .long 0                                     /* LoaderFlags */
    .long 6                                     /* NumberOfRvaAndSizes */
    .quad 0                                     /* ExportTable */
    .quad 0                                     /* ImportTable */
    .quad 0                                     /* ResourceTable */
    .quad 0                                     /* ExceptionTable */
    .quad 0                                     /* CertificationTable */
    .quad 0                                     /* BaseRelocationTable */

.Lsection_table:
    .ascii ".text\0\0\0"
    .long __pe_text_size                        /* VirtualSize */
    .long 0x1000                                /* VirtualAddress */
    .long __pe_text_size                        /* SizeOfRawData */
    .long 0x1000                                /* PointerToRawData */
    .long 0                                     /* PointerToRelocations */
    .long 0                                     /* PointerToLineNumbers */
    .short 0                                    /* NumberOfRelocations */
    .short 0                                    /* NumberOfLineNumbers */
    .long PE_SCN_CNT_CODE | PE_SCN_MEM_EXECUTE | PE_SCN_MEM_READ

    .ascii ".data\0\0\0"
    .long __pe_data_virt_size                   /* VirtualSize (extends over bss,
                                                 * which the loader zero fills) */
    .long __pe_data_rva                         /* VirtualAddress */
    .long __pe_data_raw_size                    /* SizeOfRawData */
    .long __pe_data_rva                         /* PointerToRawData */
    .long 0                                     /* PointerToRelocations */
    .long 0                                     /* PointerToLineNumbers */
    .short 0                                    /* NumberOfRelocations */
    .short 0                                    /* NumberOfLineNumbers */
    .long PE_SCN_CNT_INITIALIZED_DATA | PE_SCN_MEM_READ | PE_SCN_MEM_WRITE

/* multiboot 1 and 2 headers, moved here from start.S so that this file owns
 * the entire first-4KB layout. Loaders search for them within the first 8KB
 * (MB1) / 32KB (MB2) of the file, 4/8 byte aligned; sitting after the PE
 * header is fine. */
.align 8
/* NOTE: This MB2 header only advertises the legacy i386 handoff path.
 * GRUB UEFI/PXE may require EFI amd64 MB2 entry tags and a 64-bit EFI-aware
 * entry stub; use MB1 or the native UEFI PE path until LK grows that support.
 */
.type multiboot2_header, STT_OBJECT
multiboot2_header:
    .long MULTIBOOT2_HEADER_MAGIC               /* Multiboot header magic */
    .long MULTIBOOT2_ARCHITECTURE_I386          /* i386 */
    .long header_end - multiboot2_header        /* length */

    /* checksum */
    .long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT2_ARCHITECTURE_I386 + (header_end - multiboot2_header))

.align 8
address_tag_start:
    .short MULTIBOOT2_HEADER_TAG_ADDRESS        /* MULTIBOOT_HEADER_TAG_ADDRESS */
    .short MULTIBOOT2_HEADER_TAG_OPTIONAL       /* MULTIBOOT_HEADER_TAG_OPTIONAL */
    .long address_tag_end - address_tag_start
    .long PHYS(multiboot2_header)               /* Header address */
    .long PHYS(__code_start)                    /* Load start */
    .long PHYS(__data_end)                      /* DATA END */
    .long PHYS(__bss_end)                       /* BSS END */
address_tag_end:

.align 8
entry_address_tag_start:
    .short MULTIBOOT2_HEADER_TAG_ENTRY_ADDRESS  /* Entry address tag */
    .short MULTIBOOT2_HEADER_TAG_OPTIONAL       /* Optional flag */
    .long entry_address_tag_end - entry_address_tag_start
    .long PHYS(real_start)                      /* Real start address */
entry_address_tag_end:

/* Header end */
.align 8
    .short MULTIBOOT2_HEADER_TAG_END            /* End flag */
    .short 0
    .long 8                                     /* align 8? */
header_end:

#define MULTIBOOT_HEADER_FLAGS (MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_AOUT_KLUDGE)

.type multiboot_header,STT_OBJECT
multiboot_header:
    /* magic */
    .int MULTIBOOT_HEADER_MAGIC
    /* flags */
    .int MULTIBOOT_HEADER_FLAGS
    /* checksum */
    .int -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

    /* header_addr */
    .int PHYS(multiboot_header)
    /* load_addr */
    .int PHYS(__code_start)
    /* load_end_addr */
    .int PHYS(__data_end)
    /* bss_end_addr */
    .int PHYS(__bss_end)
    /* entry_addr */
    .int PHYS(real_start)

/* pad the header block out to exactly 4KB so .text.boot lands at file
 * offset 0x1000 == the PE .text section RVA */
    .org 0x1000
