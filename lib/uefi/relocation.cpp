
#include "relocation.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "pe.h"

namespace {

constexpr size_t BIT26 = 1 << 26;
constexpr size_t BIT11 = 1 << 11;
constexpr size_t BIT10 = 1 << 10;

/**
  Pass in a pointer to an ARM MOVT or MOVW immediate instruciton and
  return the immediate data encoded in the instruction.

  @param  Instruction   Pointer to ARM MOVT or MOVW immediate instruction

  @return Immediate address encoded in the instruction

**/
uint16_t ThumbMovtImmediateAddress(const uint16_t *Instruction) {
  uint32_t Movt{};
  uint16_t Address{};

  // Thumb2 is two 16-bit instructions working together. Not a single 32-bit
  // instruction Example MOVT R0, #0 is 0x0000f2c0 or 0xf2c0 0x0000
  Movt = (*Instruction << 16) | (*(Instruction + 1));

  // imm16 = imm4:i:imm3:imm8
  //         imm4 -> Bit19:Bit16
  //         i    -> Bit26
  //         imm3 -> Bit14:Bit12
  //         imm8 -> Bit7:Bit0
  Address = static_cast<uint16_t>(Movt & 0x000000ff);         // imm8
  Address |= static_cast<uint16_t>((Movt >> 4) & 0x0000f700); // imm4 imm3
  Address |= (((Movt & BIT26) != 0) ? BIT11 : 0);             // i
  return Address;
}

/**
  Pass in a pointer to an ARM MOVW/MOVT instruciton pair and
  return the immediate data encoded in the two` instruction.

  @param  Instructions  Pointer to ARM MOVW/MOVT insturction pair

  @return Immediate address encoded in the instructions

**/
uint32_t ThumbMovwMovtImmediateAddress(uint16_t *Instructions) {
  uint16_t *Word{};
  uint16_t *Top{};

  Word = Instructions; // MOVW
  Top = Word + 2;      // MOVT

  return (ThumbMovtImmediateAddress(Top) << 16) +
         ThumbMovtImmediateAddress(Word);
}

/**
  Update an ARM MOVT or MOVW immediate instruction immediate data.

  @param  Instruction   Pointer to ARM MOVT or MOVW immediate instruction
  @param  Address       New addres to patch into the instruction
**/
void ThumbMovtImmediatePatch(uint16_t *Instruction, uint16_t Address) {
  uint16_t Patch{};

  // First 16-bit chunk of instruciton
  Patch = ((Address >> 12) & 0x000f);              // imm4
  Patch |= (((Address & BIT11) != 0) ? BIT10 : 0); // i
  // Mask out instruction bits and or in address
  *(Instruction) = (*Instruction & ~0x040f) | Patch;

  // Second 16-bit chunk of instruction
  Patch = Address & 0x000000ff;           // imm8
  Patch |= ((Address << 4) & 0x00007000); // imm3
  // Mask out instruction bits and or in address
  Instruction++;
  *Instruction = (*Instruction & ~0x70ff) | Patch;
}

/**
  Update an ARM MOVW/MOVT immediate instruction instruction pair.

  @param  Instructions  Pointer to ARM MOVW/MOVT instruction pair
  @param  Address       New addres to patch into the instructions
**/
void ThumbMovwMovtImmediatePatch(uint16_t *Instructions, uint32_t Address) {
  uint16_t *Word{};
  uint16_t *Top{};

  Word = Instructions; // MOVW
  Top = Word + 2;      // MOVT

  ThumbMovtImmediatePatch(Word, static_cast<uint16_t>(Address & 0xffff));
  ThumbMovtImmediatePatch(Top, static_cast<uint16_t>(Address >> 16));
}

}  // namespace

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
      uint32_t FixupVal = 0;
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

      case EFI_IMAGE_REL_BASED_ARM_MOV32T:
        FixupVal = ThumbMovwMovtImmediateAddress(Fixup16) +
                   static_cast<uint32_t>(Adjust);
        ThumbMovwMovtImmediatePatch(Fixup16, FixupVal);

        break;

      case EFI_IMAGE_REL_BASED_ARM_MOV32A:
        printf("Unsupported relocation type: EFI_IMAGE_REL_BASED_ARM_MOV32A\n");
        // break omitted - ARM instruction encoding not implemented
        break;

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
