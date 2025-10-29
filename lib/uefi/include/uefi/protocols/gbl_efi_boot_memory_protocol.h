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
 * SPDX-License-Identifier: Apache-2.0 OR BSD-2-Clause-Patent
 *
 * You may choose to use or redistribute this file under
 *  (a) the Apache License, Version 2.0, or
 *  (b) the BSD 2-Clause Patent license.
 *
 * Unless you expressly elect the BSD-2-Clause-Patent terms, the Apache-2.0
 * terms apply by default.
 */

#ifndef __GBL_EFI_BOOT_MEMORY_PROTOCOL_H__
#define __GBL_EFI_BOOT_MEMORY_PROTOCOL_H__

#include <stddef.h>

#include <uefi/types.h>

static const uint64_t GBL_EFI_BOOT_MEMORY_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 1);

EFI_ENUM(GblEfiBootBufferType, uint32_t, GBL_EFI_BOOT_BUFFER_TYPE_GENERAL_LOAD,
         GBL_EFI_BOOT_BUFFER_TYPE_KERNEL, GBL_EFI_BOOT_BUFFER_TYPE_RAMDISK,
         GBL_EFI_BOOT_BUFFER_TYPE_FDT, GBL_EFI_BOOT_BUFFER_TYPE_PVMFW_DATA,
         GBL_EFI_BOOT_BUFFER_TYPE_FASTBOOT_DOWNLOAD);

EFI_ENUM(GblEfiPartitionBufferFlag, uint32_t,
         GBL_EFI_PARTITION_BUFFER_FLAG_PRELOADED = 1 << 0);

typedef struct GblEfiBootMemoryProtocol {
  uint64_t revision;
  EfiStatus (*get_partition_buffer)(struct GblEfiBootMemoryProtocol* self,
                                    /* in */ const uint8_t* base_name,
                                    /* out */ size_t* size,
                                    /* out */ void** addr,
                                    /* out */ GblEfiPartitionBufferFlag* flag);
  EfiStatus (*sync_partition_buffer)(struct GblEfiBootMemoryProtocol* self,
                                     /* in */ bool sync_preloaded);
  EfiStatus (*get_boot_buffer)(struct GblEfiBootMemoryProtocol* self,
                               /* in */ GblEfiBootBufferType buf_type,
                               /* out */ size_t* size,
                               /* out */ void** addr);
} GblEfiBootMemoryProtocol;

#endif  //__GBL_EFI_BOOT_MEMORY_PROTOCOL_H__
