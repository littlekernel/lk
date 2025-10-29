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
 * SPDX-License-Identifier: Apache-2.0 OR BSD-2-Clause-Patent
 *
 * You may choose to use or redistribute this file under
 *  (a) the Apache License, Version 2.0, or
 *  (b) the BSD 2-Clause Patent license.
 *
 * Unless you expressly elect the BSD-2-Clause-Patent terms, the Apache-2.0
 * terms apply by default.
 */

#ifndef __GBL_EFI_BOOT_CONTROL_PROTOCOL_H__
#define __GBL_EFI_BOOT_CONTROL_PROTOCOL_H__

#include <stdint.h>

#include <uefi/system_table.h>
#include <uefi/types.h>

EFI_ENUM(GblEfiUnbootableReason, uint8_t,
         GBL_EFI_UNBOOTABLE_REASON_UNKNOWN_REASON,
         GBL_EFI_UNBOOTABLE_REASON_NO_MORE_TRIES,
         GBL_EFI_UNBOOTABLE_REASON_SYSTEM_UPDATE,
         GBL_EFI_UNBOOTABLE_REASON_USER_REQUESTED,
         GBL_EFI_UNBOOTABLE_REASON_VERIFICATION_FAILURE);

EFI_ENUM(GblEfiOneShotBootMode, uint32_t, GBL_EFI_ONE_SHOT_BOOT_MODE_NONE,
         GBL_EFI_ONE_SHOT_BOOT_MODE_BOOTLOADER,
         GBL_EFI_ONE_SHOT_BOOT_MODE_RECOVERY);

typedef struct {
  // One UTF-8 encoded single character
  uint32_t suffix;
  // Any value other than those explicitly enumerated in EFI_UNBOOTABLE_REASON
  // will be interpreted as UNKNOWN_REASON.
  GblEfiUnbootableReason unbootable_reason;
  uint8_t priority;
  uint8_t tries;
  // Value of 1 if slot has successfully booted.
  uint8_t successful;
} GblEfiSlotInfo;

typedef struct {
  size_t kernel_size;
  EfiPhysicalAddr kernel;
  size_t ramdisk_size;
  EfiPhysicalAddr ramdisk;
  size_t device_tree_size;
  EfiPhysicalAddr device_tree;
  uint64_t reserved[8];
} GblEfiLoadedOs;

typedef void (*OsEntryPoint)(size_t descriptor_size,
                             uint32_t descriptor_version,
                             size_t num_descriptors,
                             const EfiMemoryDescriptor* memory_map,
                             const GblEfiLoadedOs* os);

static const uint64_t GBL_EFI_BOOT_CONTROL_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 2);

typedef struct GblEfiBootControlProtocol {
  uint64_t revision;
  // Slot metadata query methods
  EfiStatus (*get_slot_count)(struct GblEfiBootControlProtocol* self,
                              /* out */ uint8_t* slot_count);
  EfiStatus (*get_slot_info)(struct GblEfiBootControlProtocol* self,
                             /* in */ uint8_t index,
                             /* out */ GblEfiSlotInfo* info);
  EfiStatus (*get_current_slot)(struct GblEfiBootControlProtocol* self,
                                /* out */ GblEfiSlotInfo* info);
  // Slot metadata manipulation methods
  EfiStatus (*set_active_slot)(struct GblEfiBootControlProtocol* self,
                               /* in */ uint8_t index);
  // Boot control methods
  EfiStatus (*get_one_shot_boot_mode)(struct GblEfiBootControlProtocol* self,
                                      /* out */ GblEfiOneShotBootMode* mode);
  EfiStatus (*handle_loaded_os)(struct GblEfiBootControlProtocol* self,
                                /* in */ const GblEfiLoadedOs* os,
                                /* out */ OsEntryPoint* entry_point);
} GblEfiBootControlProtocol;

#endif  // __GBL_EFI_BOOT_CONTROL_PROTOCOL_H__
