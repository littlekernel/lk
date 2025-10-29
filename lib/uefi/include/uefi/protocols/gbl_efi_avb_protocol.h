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

// This is a custom protocol introduced by GBL.
// See gbl/docs/gbl_efi_avb_protocol.md for details.

#ifndef __GBL_AVB_PROTOCOL_H__
#define __GBL_AVB_PROTOCOL_H__

#include <uefi/types.h>

static const uint64_t GBL_EFI_AVB_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 3);

typedef uint64_t GblEfiAvbDeviceStatus;
// Indicates device is unlocked.
static const GblEfiAvbDeviceStatus GBL_EFI_AVB_DEVICE_STATUS_UNLOCKED = 0x1
                                                                        << 0;
// Indecated dm-verity error is occurred.
static const GblEfiAvbDeviceStatus GBL_EFI_AVB_DEVICE_STATUS_DM_VERITY_FAILED =
    0x1 << 1;

// Os boot state color flags.
//
// https://source.android.com/docs/security/features/verifiedboot/boot-flow#communicating-verified-boot-state-to-users
typedef uint64_t GblEfiAvbBootColorFlags;
static const GblEfiAvbBootColorFlags GBL_EFI_AVB_BOOT_COLOR_RED = 0x1 << 0;
static const GblEfiAvbBootColorFlags GBL_EFI_AVB_BOOT_COLOR_ORANGE = 0x1 << 1;
static const GblEfiAvbBootColorFlags GBL_EFI_AVB_BOOT_COLOR_YELLOW = 0x1 << 2;
static const GblEfiAvbBootColorFlags GBL_EFI_AVB_BOOT_COLOR_GREEN = 0x1 << 3;
static const GblEfiAvbBootColorFlags GBL_EFI_AVB_BOOT_COLOR_RED_EIO = 0x1 << 4;

// Vbmeta key validation status.
//
// https://source.android.com/docs/security/features/verifiedboot/boot-flow#locked-devices-with-custom-root-of-trust
EFI_ENUM(GblEfiAvbKeyValidationStatus, uint32_t,
         GBL_EFI_AVB_KEY_VALIDATION_STATUS_INVALID,
         GBL_EFI_AVB_KEY_VALIDATION_STATUS_VALID_CUSTOM_KEY,
         GBL_EFI_AVB_KEY_VALIDATION_STATUS_VALID);

typedef uint64_t GblEfiAvbPartitionFlags;
static const GblEfiAvbPartitionFlags GBL_EFI_AVB_PARTITION_OPTIONAL = 0x1 << 0;

typedef struct {
  // On input - `base_name` buffer size
  // On output - actual `base_name` length
  size_t base_name_len;
  uint8_t* base_name;
  GblEfiAvbPartitionFlags flags;
} GblEfiAvbPartition;

typedef struct {
  // UTF-8, null terminated
  const uint8_t* base_name;
  size_t data_size;
  const uint8_t* data;
} GblEfiAvbLoadedPartition;

typedef struct {
  // UTF-8, null terminated
  const uint8_t* base_partition_name;
  // UTF-8, null terminated
  const uint8_t* key;
  // Excluding null terminator
  size_t value_size;
  const uint8_t* value;
} GblEfiAvbProperty;

typedef struct {
  GblEfiAvbBootColorFlags color_flags;
  // Pointer to nul-terminated ASCII hex digest calculated by libavb. May be
  // null in case of verification failed (RED boot state color).
  const uint8_t* digest;
  size_t num_partitions;
  const GblEfiAvbLoadedPartition* partitions;
  size_t num_properties;
  const GblEfiAvbProperty* properties;
  uint64_t reserved[8];
} GblEfiAvbVerificationResult;

typedef struct GblEfiAvbProtocol {
  uint64_t revision;

  EfiStatus (*read_partitions_to_verify)(
      struct GblEfiAvbProtocol* self,
      /* in-out */ size_t* num_partitions,
      /* in-out */ GblEfiAvbPartition* partitions);

  EfiStatus (*read_device_status)(
      struct GblEfiAvbProtocol* self,
      /* out */ GblEfiAvbDeviceStatus* status_flags);

  EfiStatus (*validate_vbmeta_public_key)(
      struct GblEfiAvbProtocol* self,
      /* in */ size_t public_key_length,
      /* in */ const uint8_t* public_key_data,
      /* in */ size_t public_key_metadata_length,
      /* in */ const uint8_t* public_key_metadata,
      /* out */ GblEfiAvbKeyValidationStatus* validation_status);

  EfiStatus (*read_rollback_index)(struct GblEfiAvbProtocol* self,
                                   /* in */ size_t index_location,
                                   /* out */ uint64_t* rollback_index);

  EfiStatus (*write_rollback_index)(struct GblEfiAvbProtocol* self,
                                    /* in */ size_t index_location,
                                    /* in */ uint64_t rollback_index);

  EfiStatus (*read_persistent_value)(struct GblEfiAvbProtocol* self,
                                     /* in */ const uint8_t* name,
                                     /* in-out */ size_t* value_size,
                                     /* out */ uint8_t* value);

  EfiStatus (*write_persistent_value)(struct GblEfiAvbProtocol* self,
                                      /* in */ const uint8_t* name,
                                      /* in */ size_t value_size,
                                      /* in */ const uint8_t* value);

  EfiStatus (*handle_verification_result)(
      struct GblEfiAvbProtocol* self,
      /* in */ const GblEfiAvbVerificationResult* result);

} GblEfiAvbProtocol;

#endif  //__GBL_AVB_PROTOCOL_H__
