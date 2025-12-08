
#include "relocation.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "pe.h"

int relocate_image(char *image) {
  const auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(image);
  const auto pe_header = dos_header->GetPEHeader();
  const auto optional_header = &pe_header->OptionalHeader;
  const auto reloc_directory =
      optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
  if (reloc_directory.Size == 0) {
    printf("Relocation section empty\n");
    return 0;
  }
  auto RelocBase = reinterpret_cast<EFI_IMAGE_BASE_RELOCATION *>(
      image + reloc_directory.VirtualAddress);
  const auto RelocBaseEnd = reinterpret_cast<EFI_IMAGE_BASE_RELOCATION *>(
      reinterpret_cast<char *>(RelocBase) + reloc_directory.Size);
  const auto Adjust =
      reinterpret_cast<size_t>(image - optional_header->ImageBase);
  //
  // Run this relocation record
  //
  while (RelocBase < RelocBaseEnd) {
    auto Reloc =
        reinterpret_cast<uint16_t *>(reinterpret_cast<char *>(RelocBase) +
                                     sizeof(EFI_IMAGE_BASE_RELOCATION));
    auto RelocEnd = reinterpret_cast<uint16_t *>(
        reinterpret_cast<char *>(RelocBase) + RelocBase->SizeOfBlock);
    if (RelocBase->SizeOfBlock == 0) {
      printf("Found relocation block of size 0, this is wrong\n");
      return -1;
    }
    while (Reloc < RelocEnd) {
      auto Fixup = image + RelocBase->VirtualAddress + (*Reloc & 0xFFF);
      if (Fixup == nullptr) {
        return 0;
      }

      auto Fixup16 = reinterpret_cast<uint16_t *>(Fixup);
      auto Fixup32 = reinterpret_cast<uint32_t *>(Fixup);
      auto Fixup64 = reinterpret_cast<uint64_t *>(Fixup);
      switch ((*Reloc) >> 12) {
      case EFI_IMAGE_REL_BASED_ABSOLUTE:
        break;

      case EFI_IMAGE_REL_BASED_HIGH:
        *Fixup16 = static_cast<uint16_t>(
            *Fixup16 +
            (static_cast<uint16_t>(static_cast<uint32_t>(Adjust) >> 16)));

        break;

      case EFI_IMAGE_REL_BASED_LOW:
        *Fixup16 = static_cast<uint16_t>(
            *Fixup16 + (static_cast<uint16_t>(Adjust) & 0xffff));

        break;

      case EFI_IMAGE_REL_BASED_HIGHLOW:
        *Fixup32 = *Fixup32 + static_cast<uint32_t>(Adjust);
        break;

      case EFI_IMAGE_REL_BASED_DIR64:
        *Fixup64 = *Fixup64 + static_cast<uint64_t>(Adjust);
        break;

      case EFI_IMAGE_REL_BASED_ARM_MOV32A:
        printf("Unsupported relocation type: EFI_IMAGE_REL_BASED_ARM_MOV32A\n");
        // break omitted - ARM instruction encoding not implemented
        break;
      case EFI_IMAGE_REL_BASED_LOONGARCH64_MARK_LA: {
        // The next four instructions are used to load a 64 bit address,
        // relocate all of them
        uint64_t Value =
            (*Fixup32 & 0x1ffffe0) << 7 |           // lu12i.w 20bits from bit5
            (*(Fixup32 + 1) & 0x3ffc00) >> 10;      // ori     12bits from bit10
        uint64_t Tmp1 = *(Fixup32 + 2) & 0x1ffffe0; // lu32i.d 20bits from bit5
        uint64_t Tmp2 = *(Fixup32 + 3) & 0x3ffc00;  // lu52i.d 12bits from bit10
        Value = Value | (Tmp1 << 27) | (Tmp2 << 42);
        Value += Adjust;

        *Fixup32 = (*Fixup32 & ~0x1ffffe0) | (((Value >> 12) & 0xfffff) << 5);

        Fixup += sizeof(uint32_t);
        *Fixup32 = (*Fixup32 & ~0x3ffc00) | ((Value & 0xfff) << 10);

        Fixup += sizeof(uint32_t);
        *Fixup32 = (*Fixup32 & ~0x1ffffe0) | (((Value >> 32) & 0xfffff) << 5);

        Fixup += sizeof(uint32_t);
        *Fixup32 = (*Fixup32 & ~0x3ffc00) | (((Value >> 52) & 0xfff) << 10);
      }
      default:
        printf("Unsupported relocation type: %d\n", (*Reloc) >> 12);
        return -1;
      }

      //
      // Next relocation record
      //
      Reloc += 1;
    }
    RelocBase = reinterpret_cast<EFI_IMAGE_BASE_RELOCATION *>(RelocEnd);
  }
  optional_header->ImageBase = reinterpret_cast<size_t>(image);
  return 0;
}
