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

// This is a protocol proposed by U-boot and being used by Kernel UEFI stub.
// https://github.com/U-Boot-EFI/EFI_DT_FIXUP_PROTOCOL
// https://github.com/u-boot/u-boot/blob/master/include/efi_dt_fixup.h

#ifndef __EFI_DT_FIXUP_PROTOCOL_H__
#define __EFI_DT_FIXUP_PROTOCOL_H__

#include "types.h"

constexpr uint64_t EFI_DT_FIXUP_PROTOCOL_REVISION = 0x00010000;

// Add nodes and update properties
constexpr uint32_t EFI_DT_APPLY_FIXUPS = 0x00000001;
// Reserve memory according to the /reserved-memory node and the memory
// reservation block
constexpr uint32_t EFI_DT_RESERVE_MEMORY = 0x00000002;
// Install the device-tree as configuration table
constexpr uint32_t EFI_DT_INSTALL_TABLE = 0x00000004;
constexpr uint32_t EFI_DT_ALL =
    EFI_DT_APPLY_FIXUPS | EFI_DT_RESERVE_MEMORY | EFI_DT_INSTALL_TABLE;

typedef struct EfiDtFixupProtocol {
  uint64_t revision;
  EfiStatus (*fixup)(struct EfiDtFixupProtocol *self, void *fdt,
                     size_t *buffer_size, uint32_t flags);
} EfiDtFixupProtocol;

#endif // __EFI_DT_FIXUP_PROTOCOL_H__
