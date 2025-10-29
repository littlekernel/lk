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
// See gbl/docs/gbl_efi_os_configuration_protocol.md for details.

#ifndef __GBL_OS_CONFIGURATION_PROTOCOL_H__
#define __GBL_OS_CONFIGURATION_PROTOCOL_H__

#include <uefi/types.h>

EFI_ENUM(GblEfiDeviceTreeType, uint32_t,
         // HLOS device tree.
         GBL_EFI_DEVICE_TREE_TYPE_DEVICE_TREE,
         // HLOS device tree overlay.
         GBL_EFI_DEVICE_TREE_TYPE_OVERLAY,
         // pVM device assignment overlay.
         GBL_EFI_DEVICE_TREE_TYPE_PVM_DA_OVERLAY);

EFI_ENUM(GblEfiDeviceTreeSource, uint32_t,
         // Device tree loaded from boot partition.
         GBL_EFI_DEVICE_TREE_SOURCE_BOOT,
         // Device tree loaded from vendor_boot partition.
         GBL_EFI_DEVICE_TREE_SOURCE_VENDOR_BOOT,
         // Device tree loaded from dtbo partition.
         GBL_EFI_DEVICE_TREE_SOURCE_DTBO,
         // Device tree loaded from dtb partition.
         GBL_EFI_DEVICE_TREE_SOURCE_DTB);

typedef struct {
  GblEfiDeviceTreeSource source;
  GblEfiDeviceTreeType type;
  // Values are zeroed and must not be used in case of BOOT / VENDOR_BOOT source
  uint32_t id;
  uint32_t rev;
  uint32_t custom[4];
} GblEfiDeviceTreeMetadata;

typedef struct {
  GblEfiDeviceTreeMetadata metadata;
  // Base device tree / overlay buffer (guaranteed to be 8-bytes aligned),
  // cannot be NULL. Device tree size can be identified by the header totalsize
  // field
  const void* device_tree;
  // Indicates whether this device tree (or overlay) must be included in the
  // final device tree. Set to true by a FW if this component must be used
  bool selected;
} GblEfiVerifiedDeviceTree;

static const uint64_t GBL_EFI_OS_CONFIGURATION_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 1);

typedef struct GblEfiOsConfigurationProtocol {
  uint64_t revision;

  // Generates fixups for the bootconfig built by GBL.
  EfiStatus (*fixup_bootconfig)(struct GblEfiOsConfigurationProtocol* self,
                                const uint8_t* bootconfig, size_t size,
                                uint8_t* fixup, size_t* fixup_buffer_size);

  // Selects which device trees and overlays to use from those loaded by GBL.
  EfiStatus (*select_device_trees)(struct GblEfiOsConfigurationProtocol* self,
                                   GblEfiVerifiedDeviceTree* device_trees,
                                   size_t num_device_trees);

  // Selects FIT configuration to be used.
  EfiStatus (*select_fit_configuration)(
      struct GblEfiOsConfigurationProtocol* self, size_t fit_size,
      const uint8_t* fit, size_t metadata_size, const uint8_t* metadata,
      size_t* selected_configuration_offset);

} GblEfiOsConfigurationProtocol;

#endif  //__GBL_OS_CONFIGURATION_PROTOCOL_H__
