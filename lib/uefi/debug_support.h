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

#ifndef __DEBUG_SUPPORT_
#define __DEBUG_SUPPORT_

#include <lib/bio.h>
#include <uefi/protocols/loaded_image_protocol.h>
#include <uefi/system_table.h>
#include <uefi/types.h>

static constexpr auto EFI_DEBUG_IMAGE_INFO_TABLE_GUID =
    EfiGuid{0x49152e77,
            0x1ada,
            0x4764,
            {0xb7, 0xa2, 0x7a, 0xfe, 0xfe, 0xd9, 0x5e, 0x8b}};

static constexpr size_t EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS = 0x01;
static constexpr size_t EFI_DEBUG_IMAGE_INFO_TABLE_MODIFIED = 0x02;

static constexpr size_t EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL = 0x01;

struct EfiSystemTablePointer {
  uint64_t signature;
  EfiSystemTable *system_table_base;
  uint32_t crc32;
};

struct EfiDebugImageInfoNormal {
  uint32_t image_info_type;
  EfiLoadedImageProtocol *loaded_image_protocol_instance;
  EfiHandle image_handle;
};

union EfiDebugImageInfo {
  uint32_t *image_info_type;
  struct EfiDebugImageInfoNormal *normal_image;
};

struct EfiDebugImageInfoTableHeader {
  volatile uint32_t update_status;
  uint32_t table_size;
  union EfiDebugImageInfo *efi_debug_image_info_table;
};

EfiStatus efi_initialize_system_table_pointer(struct EfiSystemTable *system_table);
EfiStatus efi_core_new_debug_image_info_entry(uint32_t image_info_type,
                                              EfiLoadedImageProtocol *loaded_image,
                                              EfiHandle image_handle);
void efi_core_remove_debug_image_info_entry(EfiHandle image_handle);
EfiStatus setup_debug_support(EfiSystemTable &table,
			      char *image_base,
			      size_t virtual_size,
			      const char *dev_name);

void teardown_debug_support(char *image_base);

extern struct EfiDebugImageInfoTableHeader efi_m_debug_info_table_header;

#endif
