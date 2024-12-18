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

// This is a custom protocol introduced by GBL.
// See gbl/docs/gbl_efi_os_configuration_protocol.md for details.

#ifndef __GBL_OS_CONFIGURATION_PROTOCOL_H__
#define __GBL_OS_CONFIGURATION_PROTOCOL_H__

#include "types.h"

static constexpr size_t GBL_EFI_OS_CONFIGURATION_PROTOCOL_REVISION = 0x00000000;

typedef enum GBL_EFI_DEVICE_TREE_SOURCE {
  BOOT,
  VENDOR_BOOT,
  DTBO,
  DTB
} GblEfiDeviceTreeSource;

typedef struct {
  // GblDeviceTreeSource
  uint32_t source;
  // Values are zeroed and must not be used in case of BOOT / VENDOR_BOOT source
  uint32_t id;
  uint32_t rev;
  uint32_t custom[4];
  // Make sure GblDeviceTreeMetadata size is 8-bytes aligned. Also reserved for
  // the future cases
  uint32_t reserved;
} GblEfiDeviceTreeMetadata;

typedef struct {
  GblEfiDeviceTreeMetadata metadata;
  // Base device tree / overlay buffer (guaranteed to be 8-bytes aligned),
  // cannot be NULL. Device tree size can be identified by the header totalsize
  // field
  const void *device_tree;
  // Indicates whether this device tree (or overlay) must be included in the
  // final device tree. Set to true by a FW if this component must be used
  bool selected;
} GblEfiVerifiedDeviceTree;

// Warning: API is UNSTABLE
// Documentation:
// https://cs.android.com/android/platform/superproject/main/+/main:bootable/libbootloader/gbl/docs/gbl_os_configuration_protocol.md
typedef struct GblEfiOsConfigurationProtocol {
  uint64_t revision;

  // Generates fixups for the kernel command line built by GBL.
  EfiStatus (*fixup_kernel_commandline)(
      struct GblEfiOsConfigurationProtocol *self, const char *command_line,
      char *fixup, size_t *fixup_buffer_size);

  // Generates fixups for the bootconfig built by GBL.
  EfiStatus (*fixup_bootconfig)(struct GblEfiOsConfigurationProtocol *self,
                                const char *bootconfig, size_t size,
                                char *fixup, size_t *fixup_buffer_size);

  // Selects which device trees and overlays to use from those loaded by GBL.
  EfiStatus (*select_device_trees)(struct GblEfiOsConfigurationProtocol *self,
                                   GblEfiVerifiedDeviceTree *device_trees,
                                   size_t num_device_trees);
} GblEfiOsConfigurationProtocol;

#endif //__GBL_OS_CONFIGURATION_PROTOCOL_H__