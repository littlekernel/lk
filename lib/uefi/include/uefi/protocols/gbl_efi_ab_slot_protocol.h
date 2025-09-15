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

#ifndef __GBL_EFI_AB_SLOT_PROTOCOL_H__
#define __GBL_EFI_AB_SLOT_PROTOCOL_H__

#include <stdint.h>

#include "system_table.h"
#include "types.h"

EFI_ENUM(GBL_EFI_SLOT_MERGE_STATUS, GblEfiSlotMergeStatus, uint8_t,
         GBL_EFI_SLOT_MERGE_STATUS_NONE, GBL_EFI_SLOT_MERGE_STATUS_UNKNOWN,
         GBL_EFI_SLOT_MERGE_STATUS_SNAPSHOTTED,
         GBL_EFI_SLOT_MERGE_STATUS_MERGING,
         GBL_EFI_SLOT_MERGE_STATUS_CANCELLED);

EFI_ENUM(GBL_EFI_UNBOOTABLE_REASON, GblEfiUnbootableReason, uint8_t,
         GBL_EFI_UNBOOTABLE_REASON_UNKNOWN_REASON,
         GBL_EFI_UNBOOTABLE_REASON_NO_MORE_TRIES,
         GBL_EFI_UNBOOTABLE_REASON_SYSTEM_UPDATE,
         GBL_EFI_UNBOOTABLE_REASON_USER_REQUESTED,
         GBL_EFI_UNBOOTABLE_REASON_VERIFICATION_FAILURE);

EFI_ENUM(GBL_EFI_BOOT_MODE, GblEfiBootMode, uint32_t, GBL_EFI_BOOT_MODE_NORMAL,
         GBL_EFI_BOOT_MODE_RECOVERY, GBL_EFI_BOOT_MODE_FASTBOOTD,
         GBL_EFI_BOOT_MODE_BOOTLOADER);

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
  // Value of 1 if persistent metadata tracks slot unbootable reasons.
  uint8_t unbootable_metadata;
  uint8_t max_retries;
  uint8_t slot_count;
  GblEfiSlotMergeStatus merge_status;
} GblEfiSlotMetadataBlock;

static const uint64_t GBL_EFI_AB_SLOT_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 1);

typedef struct GblEfiABSlotProtocol {
  uint64_t revision;
  // Slot metadata query methods
  EfiStatus (*load_boot_data)(struct GblEfiABSlotProtocol* self,
                              /* out */ GblEfiSlotMetadataBlock* metadata);
  EfiStatus (*get_slot_info)(struct GblEfiABSlotProtocol* self,
                             /* in */ uint8_t index,
                             /* out */ GblEfiSlotInfo* info);
  EfiStatus (*get_current_slot)(struct GblEfiABSlotProtocol* self,
                                /* out */ GblEfiSlotInfo* info);
  void *_reserved;
  // Slot metadata manipulation methods
  EfiStatus (*set_active_slot)(struct GblEfiABSlotProtocol* self,
                               /* in */ uint8_t index);
  EfiStatus (*set_slot_unbootable)(
      struct GblEfiABSlotProtocol* self,
      /* in */ uint8_t index,
      /* in */ GblEfiUnbootableReason unbootable_reason);
  EfiStatus (*reinitialize)(struct GblEfiABSlotProtocol* self);
  // Boot mode
  EfiStatus (*get_boot_mode)(struct GblEfiABSlotProtocol* self,
                             /* out */ GblEfiBootMode* mode);
  EfiStatus (*set_boot_mode)(struct GblEfiABSlotProtocol* self,
                             /* in */ GblEfiBootMode mode);
  // Miscellaneous methods
  EfiStatus (*flush)(struct GblEfiABSlotProtocol* self);
} GblEfiABSlotProtocol;

#endif  // __GBL_EFI_AB_SLOT_PROTOCOL_H__
