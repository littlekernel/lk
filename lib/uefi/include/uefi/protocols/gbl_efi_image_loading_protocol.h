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

#ifndef __GBL_EFI_IMAGE_LOADING_PROTOCOL_H__
#define __GBL_EFI_IMAGE_LOADING_PROTOCOL_H__

#include <stddef.h>

#include <uefi/types.h>

static const uint64_t GBL_EFI_IMAGE_LOADING_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 1);

#define PARTITION_NAME_LEN_U16 36

//******************************************************
// GBL reserved image types
//******************************************************
// Buffer for loading, verifying and fixing up OS images.
#define GBL_IMAGE_TYPE_OS_LOAD L"os_load"
// Buffer for use as finalized kernel load buffer.
#define GBL_IMAGE_TYPE_KERNEL_LOAD L"kernel_load"
// Buffer for use as finalized ramdisk load buffer.
#define GBL_IMAGE_TYPE_RAMDISK_LOAD L"ramdisk_load"
// Buffer for use as finalized fdt load buffer.
#define GBL_IMAGE_TYPE_FDT_LOAD L"fdt_load"
// Buffer for use as fastboot download buffer.
#define GBL_IMAGE_TYPE_FASTBOOT L"fastboot"
// Buffer reserved for pvmfw binary and configuration (must be 4KiB-aligned).
#define GBL_IMAGE_TYPE_PVMFW_DATA L"pvmfw_data"

typedef struct GblEfiImageInfo {
  uint16_t ImageType[PARTITION_NAME_LEN_U16];
  size_t SizeBytes;
} GblEfiImageInfo;

typedef struct GblEfiImageBuffer {
  void* Memory;
  size_t SizeBytes;
} GblEfiImageBuffer;

typedef struct GblEfiImageLoadingProtocol {
  uint64_t revision;
  EfiStatus (*get_buffer)(struct GblEfiImageLoadingProtocol* self,
                          const GblEfiImageInfo* ImageInfo,
                          GblEfiImageBuffer* Buffer);
} GblEfiImageLoadingProtocol;

#endif  //__GBL_EFI_IMAGE_LOADING_PROTOCOL_H__
