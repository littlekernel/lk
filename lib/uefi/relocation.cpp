
#include "relocation.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "pe.h"

int relocate_image(char *image, size_t image_size) {
  const auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(image);
  const auto pe_header = dos_header->GetPEHeader();
  const auto optional_header = &pe_header->OptionalHeader;
  // Compute the load adjustment using integers because pointer arithmetic on
  // pointers from different allocations is undefined behavior in C++.
  const auto Adjust =
      reinterpret_cast<size_t>(image) - optional_header->ImageBase;

  // DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] is only valid when the
  // optional header both declares (NumberOfRvaAndSizes) and physically contains
  // (SizeOfOptionalHeader) that entry; otherwise indexing it would read
  // section-table bytes past the header and mis-parse them.
  constexpr size_t kRelocDirEnd =
      offsetof(IMAGE_OPTIONAL_HEADER64, DataDirectory) +
      (IMAGE_DIRECTORY_ENTRY_BASERELOC + 1) * sizeof(IMAGE_DATA_DIRECTORY);
  const bool have_reloc_dir =
      optional_header->NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC &&
      pe_header->FileHeader.SizeOfOptionalHeader >= kRelocDirEnd;
  const auto reloc_directory =
      have_reloc_dir
          ? optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]
          : IMAGE_DATA_DIRECTORY{};
  if (!have_reloc_dir || reloc_directory.Size == 0) {
    // No relocations to apply. That is safe only if the image already sits at
    // its preferred base or is position independent. An image whose relocations
    // were stripped (IMAGE_FILE_RELOCS_STRIPPED: it must load at ImageBase) that
    // landed elsewhere would run with unadjusted absolute addresses, so reject
    // it instead of reporting success.
    if (Adjust != 0 &&
        (pe_header->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED)) {
      printf("Image relocations stripped but not loaded at its ImageBase\n");
      return -1;
    }
    printf("%s\n", have_reloc_dir ? "Relocation directory empty"
                                  : "No base relocation directory present");
    return 0;
  }
  // The relocation directory is attacker-controlled. Keep the whole region
  // within the allocated image so a malformed PE cannot walk out of bounds.
  const uint64_t reloc_end = static_cast<uint64_t>(reloc_directory.VirtualAddress) +
                             static_cast<uint64_t>(reloc_directory.Size);
  if (reloc_end > image_size) {
    printf("Relocation directory out of bounds\n");
    return -1;
  }
  auto RelocBase = reinterpret_cast<EFI_IMAGE_BASE_RELOCATION *>(
      image + reloc_directory.VirtualAddress);
  const auto RelocBaseEnd = reinterpret_cast<EFI_IMAGE_BASE_RELOCATION *>(
      reinterpret_cast<char *>(RelocBase) + reloc_directory.Size);
  //
  // Run this relocation record
  //
  while (reinterpret_cast<char *>(RelocBase) <
         reinterpret_cast<char *>(RelocBaseEnd)) {
    // Each block must hold its header and whole relocation entries, and fit
    // within the directory.
    const size_t block_space = reinterpret_cast<char *>(RelocBaseEnd) -
                               reinterpret_cast<char *>(RelocBase);
    if (block_space < sizeof(EFI_IMAGE_BASE_RELOCATION)) {
      printf("Truncated relocation block header\n");
      return -1;
    }
    const uint32_t size_of_block = RelocBase->SizeOfBlock;
    if (size_of_block < sizeof(EFI_IMAGE_BASE_RELOCATION) ||
        size_of_block > block_space ||
        (size_of_block - sizeof(EFI_IMAGE_BASE_RELOCATION)) % sizeof(uint16_t) != 0) {
      printf("Found relocation block of invalid size %u\n", size_of_block);
      return -1;
    }
    auto Reloc =
        reinterpret_cast<uint16_t *>(reinterpret_cast<char *>(RelocBase) +
                                     sizeof(EFI_IMAGE_BASE_RELOCATION));
    auto RelocEnd = reinterpret_cast<uint16_t *>(
        reinterpret_cast<char *>(RelocBase) + size_of_block);
    while (Reloc < RelocEnd) {
      const size_t fixup_off =
          static_cast<size_t>(RelocBase->VirtualAddress) + (*Reloc & 0xFFF);
      const auto type = static_cast<uint16_t>(*Reloc >> 12);

      // Resolve the write width first so the target can be bounds-checked
      // before any dereference; unsupported types are rejected outright.
      size_t width = 0;
      switch (type) {
      case EFI_IMAGE_REL_BASED_ABSOLUTE:
        width = 0;
        break;
      case EFI_IMAGE_REL_BASED_HIGH:
      case EFI_IMAGE_REL_BASED_LOW:
        width = sizeof(uint16_t);
        break;
      case EFI_IMAGE_REL_BASED_HIGHLOW:
        width = sizeof(uint32_t);
        break;
      case EFI_IMAGE_REL_BASED_DIR64:
        width = sizeof(uint64_t);
        break;
      case EFI_IMAGE_REL_BASED_LOONGARCH64_MARK_LA:
        // Loads a 64-bit address across the next four instructions.
        width = 4 * sizeof(uint32_t);
        break;
      case EFI_IMAGE_REL_BASED_ARM_MOV32A:
        // ARM MOVW/MOVT instruction encoding is not implemented. Reject the
        // image rather than letting it reach its entry point with this
        // relocation left unapplied.
        printf("Unsupported relocation type: EFI_IMAGE_REL_BASED_ARM_MOV32A\n");
        return -1;
      default:
        printf("Unsupported relocation type: %d\n", type);
        return -1;
      }
      if (fixup_off > image_size || width > image_size - fixup_off) {
        printf("Relocation out of bounds\n");
        return -1;
      }

      char *const Fixup = image + fixup_off;
      auto Fixup16 = reinterpret_cast<uint16_t *>(Fixup);
      auto Fixup32 = reinterpret_cast<uint32_t *>(Fixup);
      auto Fixup64 = reinterpret_cast<uint64_t *>(Fixup);
      switch (type) {
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
      case EFI_IMAGE_REL_BASED_LOONGARCH64_MARK_LA: {
        uint64_t Value =
            (*(Fixup32 + 0) & 0x1ffffe0) << 7 |           // lu12i.w 20bits from bit5
            (*(Fixup32 + 1) & 0x3ffc00) >> 10;      // ori     12bits from bit10
        uint64_t Tmp1 = *(Fixup32 + 2) & 0x1ffffe0; // lu32i.d 20bits from bit5
        uint64_t Tmp2 = *(Fixup32 + 3) & 0x3ffc00;  // lu52i.d 12bits from bit10
        Value = Value | (Tmp1 << 27) | (Tmp2 << 42);
        Value += Adjust;

        *(Fixup32 + 0) = (*(Fixup32 + 0) & ~0x1ffffe0) | (((Value >> 12) & 0xfffff) << 5);
        *(Fixup32 + 1) = (*(Fixup32 + 1) & ~0x3ffc00) | ((Value & 0xfff) << 10);
        *(Fixup32 + 2) = (*(Fixup32 + 2) & ~0x1ffffe0) | (((Value >> 32) & 0xfffff) << 5);
        *(Fixup32 + 3) = (*(Fixup32 + 3) & ~0x3ffc00) | (((Value >> 52) & 0xfff) << 10);
        break;
      }
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
