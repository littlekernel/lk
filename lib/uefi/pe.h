/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __PE_HEADER_
#define __PE_HEADER_

#include <endian.h>
#include <sys/types.h>

//
// Directory Entries
//
static constexpr size_t IMAGE_DIRECTORY_ENTRY_EXPORT = 0;
static constexpr size_t IMAGE_DIRECTORY_ENTRY_IMPORT = 1;
static constexpr size_t IMAGE_DIRECTORY_ENTRY_RESOURCE = 2;
static constexpr size_t IMAGE_DIRECTORY_ENTRY_EXCEPTION = 3;
static constexpr size_t IMAGE_DIRECTORY_ENTRY_SECURITY = 4;
static constexpr size_t IMAGE_DIRECTORY_ENTRY_BASERELOC = 5;
static constexpr size_t IMAGE_DIRECTORY_ENTRY_DEBUG = 6;
static constexpr size_t IMAGE_DIRECTORY_ENTRY_COPYRIGHT = 7;
static constexpr size_t IMAGE_DIRECTORY_ENTRY_GLOBALPTR = 8;
static constexpr size_t IMAGE_DIRECTORY_ENTRY_TLS = 9;
static constexpr size_t IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG = 10;

static constexpr size_t IMAGE_NUMBEROF_DIRECTORY_ENTRIES = 16;
static constexpr size_t IMAGE_SIZEOF_SHORT_NAME = 8;

static constexpr uint32_t kPEHeader = 0x4550;
struct IMAGE_NT_HEADERS64;

struct IMAGE_DOS_HEADER { // DOS .EXE header
  u16 e_magic;            // Magic number
  u16 e_cblp;             // Bytes on last page of file
  u16 e_cp;               // Pages in file
  u16 e_crlc;             // Relocations
  u16 e_cparhdr;          // Size of header in paragraphs
  u16 e_minalloc;         // Minimum extra paragraphs needed
  u16 e_maxalloc;         // Maximum extra paragraphs needed
  u16 e_ss;               // Initial (relative) SS value
  u16 e_sp;               // Initial SP value
  u16 e_csum;             // Checksum
  u16 e_ip;               // Initial IP value
  u16 e_cs;               // Initial (relative) CS value
  u16 e_lfarlc;           // File address of relocation table
  u16 e_ovno;             // Overlay number
  u16 e_res[4];           // Reserved words
  u16 e_oemid;            // OEM identifier (for e_oeminfo)
  u16 e_oeminfo;          // OEM information; e_oemid specific
  u16 e_res2[10];         // Reserved words
  u32 e_lfanew;           // File address of new exe header

  constexpr bool CheckMagic() const { return LE32(e_magic) == 0x5A4D; }
  IMAGE_NT_HEADERS64 *GetPEHeader() {
    auto address = reinterpret_cast<char *>(this);
    const auto pe_header =
        reinterpret_cast<IMAGE_NT_HEADERS64 *>(address + e_lfanew);
    return pe_header;
  }
  const IMAGE_NT_HEADERS64 *GetPEHeader() const {
    auto address = reinterpret_cast<const char *>(this);
    const auto pe_header =
        reinterpret_cast<const IMAGE_NT_HEADERS64 *>(address + e_lfanew);
    return pe_header;
  }
} __attribute__((packed));

enum class ArchitectureType : u16 {
  Unknown = 0x00,
  ALPHAAXPOld = 0x183,
  ALPHAAXP = 0x184,
  ALPHAAXP64Bit = 0x284,
  AM33 = 0x1D3,
  AMD64 = 0x8664,
  ARM = 0x1C0,
  ARM64 = 0xAA64,
  ARMNT = 0x1C4,
  CLRPureMSIL = 0xC0EE,
  EBC = 0xEBC,
  I386 = 0x14C,
  I860 = 0x14D,
  IA64 = 0x200,
  LOONGARCH32 = 0x6232,
  LOONGARCH64 = 0x6264,
  M32R = 0x9041,
  MIPS16 = 0x266,
  MIPSFPU = 0x366,
  MIPSFPU16 = 0x466,
  MOTOROLA68000 = 0x268,
  POWERPC = 0x1F0,
  POWERPCFP = 0x1F1,
  POWERPC64 = 0x1F2,
  R3000 = 0x162,
  R4000 = 0x166,
  R10000 = 0x168,
  RISCV32 = 0x5032,
  RISCV64 = 0x5064,
  RISCV128 = 0x5128,
  SH3 = 0x1A2,
  SH3DSP = 0x1A3,
  SH4 = 0x1A6,
  SH5 = 0x1A8,
  THUMB = 0x1C2,
  WCEMIPSV2 = 0x169
};

struct IMAGE_FILE_HEADER {
  u32 Signature;
  ArchitectureType Machine;
  u16 NumberOfSections;
  u32 TimeDateStamp;
  u32 PointerToSymbolTable;
  u32 NumberOfSymbols;
  u16 SizeOfOptionalHeader;
  u16 Characteristics;
} __attribute__((packed));

struct IMAGE_DATA_DIRECTORY {
  u32 VirtualAddress;
  u32 Size;
} __attribute__((packed));

enum SubsystemType : u16 {
  Unknown = 0x00,
  Native = 0x01,
  WindowsGUI = 0x02,
  WindowsCUI = 0x03,
  OS2CUI = 0x05,
  POSIXCUI = 0x07,
  Windows9xNative = 0x08,
  WindowsCEGUI = 0x09,
  EFIApplication = 0x0A,
  EFIBootServiceDriver = 0x0B,
  EFIRuntimeDriver = 0x0C,
  EFIROM = 0x0D,
  Xbox = 0x0E,
  WindowsBootApplication = 0x10
};

constexpr const char *ToString(SubsystemType type) {
  switch (type) {
  case Native:
    return "Native";
  case WindowsGUI:
    return "WindowsGUI";
  case WindowsCUI:
    return "WindowsCUI";
  case OS2CUI:
    return "OS2CUI";
  case POSIXCUI:
    return "POSIXCUI";
  case Windows9xNative:
    return "Windows9xNative";
  case WindowsCEGUI:
    return "WindowsCEGUI";
  case EFIApplication:
    return "EFIApplication";
  case EFIBootServiceDriver:
    return "EFIBootServiceDriver";
  case EFIRuntimeDriver:
    return "EFIRuntimeDriver";
  case EFIROM:
    return "EFIROM";
  case Xbox:
    return "Xbox";
  case WindowsBootApplication:
    return "WindowsBootApplication";
  default:
    return "Unknown";
  }
}

struct IMAGE_OPTIONAL_HEADER64 {
  u16 Magic;
  u8 MajorLinkerVersion;
  u8 MinorLinkerVersion;
  u32 SizeOfCode;
  u32 SizeOfInitializedData;
  u32 SizeOfUninitializedData;
  u32 AddressOfEntryPoint;
  u32 BaseOfCode;
  u64 ImageBase;
  u32 SectionAlignment;
  u32 FileAlignment;
  u16 MajorOperatingSystemVersion;
  u16 MinorOperatingSystemVersion;
  u16 MajorImageVersion;
  u16 MinorImageVersion;
  u16 MajorSubsystemVersion;
  u16 MinorSubsystemVersion;
  u32 Win32VersionValue;
  u32 SizeOfImage;
  u32 SizeOfHeaders;
  u32 CheckSum;
  SubsystemType Subsystem;
  u16 DllCharacteristics;
  u64 SizeOfStackReserve;
  u64 SizeOfStackCommit;
  u64 SizeOfHeapReserve;
  u64 SizeOfHeapCommit;
  u32 LoaderFlags;
  u32 NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} __attribute__((packed));

struct IMAGE_OPTIONAL_HEADER32 {
  u16 Magic;
  u8 MajorLinkerVersion;
  u8 MinorLinkerVersion;
  u32 SizeOfCode;
  u32 SizeOfInitializedData;
  u32 SizeOfUninitializedData;
  u32 AddressOfEntryPoint;
  u32 BaseOfCode;
  u32 BaseOfData;
  u32 ImageBase;
  u32 SectionAlignment;
  u32 FileAlignment;
  u16 MajorOperatingSystemVersion;
  u16 MinorOperatingSystemVersion;
  u16 MajorImageVersion;
  u16 MinorImageVersion;
  u16 MajorSubsystemVersion;
  u16 MinorSubsystemVersion;
  u32 Win32VersionValue;
  u32 SizeOfImage;
  u32 SizeOfHeaders;
  u32 CheckSum;
  u16 Subsystem;
  u16 DllCharacteristics;
  u32 SizeOfStackReserve;
  u32 SizeOfStackCommit;
  u32 SizeOfHeapReserve;
  u32 SizeOfHeapCommit;
  u32 LoaderFlags;
  u32 NumberOfRvaAndSizes;
  IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
} __attribute__((packed));

struct IMAGE_SECTION_HEADER {
  char Name[IMAGE_SIZEOF_SHORT_NAME];
  union {
    u32 PhysicalAddress;
    u32 VirtualSize;
  } Misc;
  u32 VirtualAddress;
  u32 SizeOfRawData;
  u32 PointerToRawData;
  u32 PointerToRelocations;
  u32 PointerToLinenumbers;
  u16 NumberOfRelocations;
  u16 NumberOfLinenumbers;
  u32 Characteristics;
} __attribute__((packed));

struct IMAGE_NT_HEADERS64 {
  IMAGE_FILE_HEADER FileHeader;
  IMAGE_OPTIONAL_HEADER64 OptionalHeader;
} __attribute__((packed));

struct EFI_IMAGE_BASE_RELOCATION {
  uint32_t VirtualAddress;
  uint32_t SizeOfBlock;
};

static constexpr size_t EFI_IMAGE_REL_BASED_ABSOLUTE = 0;
static constexpr size_t EFI_IMAGE_REL_BASED_HIGH = 1;
static constexpr size_t EFI_IMAGE_REL_BASED_LOW = 2;
static constexpr size_t EFI_IMAGE_REL_BASED_HIGHLOW = 3;
static constexpr size_t EFI_IMAGE_REL_BASED_HIGHADJ = 4;
static constexpr size_t EFI_IMAGE_REL_BASED_MIPS_JMPADDR = 5;
static constexpr size_t EFI_IMAGE_REL_BASED_ARM_MOV32A = 5;
static constexpr size_t EFI_IMAGE_REL_BASED_ARM_MOV32T = 7;
static constexpr size_t EFI_IMAGE_REL_BASED_IA64_IMM64 = 9;
static constexpr size_t EFI_IMAGE_REL_BASED_MIPS_JMPADDR16 = 9;
static constexpr size_t EFI_IMAGE_REL_BASED_DIR64 = 10;

#endif
