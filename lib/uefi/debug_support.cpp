/*
 * Copyright (C) 2025 The Android Open Source Project
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

#include "debug_support.h"

#include <stdio.h>
#include <string.h>
#include <lib/cksum.h>
#include <uefi/boot_service.h>
#include <uefi/types.h>

#include "memory_protocols.h"

namespace {
  struct EfiSystemTablePointer *efi_systab_pointer = nullptr;
}

struct EfiDebugImageInfoTableHeader efi_m_debug_info_table_header = {
  0,
  0,
  nullptr
};

EfiStatus efi_initialize_system_table_pointer(struct EfiSystemTable *system_table) {
  uint32_t crc = 0;
  constexpr auto EFI_SYSTEM_TABLE_SIGNATURE =
    static_cast<u64>(0x5453595320494249ULL);

  /* Allocate efi_system_table_pointer structure with 4MB alignment. */
  efi_systab_pointer = reinterpret_cast<EfiSystemTablePointer *>(alloc_page(sizeof(struct EfiSystemTablePointer), 22));

  if (!efi_systab_pointer) {
    printf("Installing EFI system table pointer failed\n");
    return OUT_OF_RESOURCES;
  }

  memset(efi_systab_pointer, 0, sizeof(struct EfiSystemTablePointer));

  efi_systab_pointer->signature = EFI_SYSTEM_TABLE_SIGNATURE;
  efi_systab_pointer->system_table_base = system_table;

  crc = crc32(crc,
	      (uint8_t *)efi_systab_pointer,
	      sizeof(struct EfiSystemTablePointer));

  efi_systab_pointer->crc32 = crc;

  return SUCCESS;
}
