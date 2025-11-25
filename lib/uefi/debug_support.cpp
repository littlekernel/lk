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

#include <lib/cksum.h>
#include <stdio.h>
#include <string.h>
#include <uefi/boot_service.h>
#include <uefi/protocols/device_path_protocol.h>
#include <uefi/protocols/loaded_image_protocol.h>
#include <uefi/types.h>

#include "charset.h"
#include "uefi_platform.h"

struct EFI_DEVICE_PATH_FILE_PATH_PROTOCOL {
  EfiDevicePathProtocol dp;
  uint16_t str[];
};

static constexpr size_t EFI_DEVICE_PATH_TYPE_END = 0x7f;
static constexpr size_t EFI_DEVICE_PATH_SUB_TYPE_END = 0xff;
static constexpr size_t EFI_DEVICE_PATH_TYPE_MEDIA_DEVICE = 0x4;
static constexpr size_t EFI_DEVICE_PATH_SUB_TYPE_FILE_PATH = 0x4;

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
    return EFI_STATUS_OUT_OF_RESOURCES;
  }

  memset(efi_systab_pointer, 0, sizeof(struct EfiSystemTablePointer));

  efi_systab_pointer->signature = EFI_SYSTEM_TABLE_SIGNATURE;
  efi_systab_pointer->system_table_base = system_table;

  crc = crc32(crc,
	      (uint8_t *)efi_systab_pointer,
	      sizeof(struct EfiSystemTablePointer));

  efi_systab_pointer->crc32 = crc;

  return EFI_STATUS_SUCCESS;
}

static uint32_t efi_m_max_table_entries;

static constexpr size_t EFI_DEBUG_TABLE_ENTRY_SIZE = (sizeof(union EfiDebugImageInfo));

EfiStatus efi_core_new_debug_image_info_entry(uint32_t image_info_type,
					      EfiLoadedImageProtocol *loaded_image,
					      EfiHandle image_handle) {
  /* Set the flag indicating that we're in the process of updating
   * the table.
   */
  efi_m_debug_info_table_header.update_status |=
    EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;

  union EfiDebugImageInfo *table;
  table = efi_m_debug_info_table_header.efi_debug_image_info_table;

  if (efi_m_debug_info_table_header.table_size >= efi_m_max_table_entries) {
    /* table is full, re-allocate the buffer increasing the size
     * by 4 KiB.
     */
    uint32_t table_size = efi_m_max_table_entries * EFI_DEBUG_TABLE_ENTRY_SIZE;
    union EfiDebugImageInfo *new_table;
    allocate_pool(EFI_MEMORY_TYPE_BOOT_SERVICES_DATA,
		  table_size + PAGE_SIZE,
		  reinterpret_cast<void **>(&new_table));

    if (!new_table) {
      efi_m_debug_info_table_header.update_status &=
	~EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;
      return EFI_STATUS_OUT_OF_RESOURCES;
    }

    memset(new_table, 0, table_size + PAGE_SIZE);

    /* Copy the old table into the new one. */
    memcpy(new_table, table, table_size);
    /* Free the old table. */
    free_pool(table);
    /* Update the table header. */
    table = new_table;
    efi_m_debug_info_table_header.efi_debug_image_info_table =
      new_table;

    /* Enlarge the max table entries and set the first empty
     * entry index to be the original max table entries.
     */
    efi_m_max_table_entries +=
      PAGE_SIZE / EFI_DEBUG_TABLE_ENTRY_SIZE;
  }

  /* We always put the next entry at the end of the currently consumed
   * table (i.e. first free entry)
   */
  uint32_t index = efi_m_debug_info_table_header.table_size;

  /* Allocate data for new entry. */
  allocate_pool(EFI_MEMORY_TYPE_BOOT_SERVICES_DATA,
		      sizeof(union EfiDebugImageInfo),
		      reinterpret_cast<void **>(&table[index].normal_image));
  if (table[index].normal_image) {
    /* Update the entry. */
    table[index].normal_image->image_info_type = image_info_type;
    table[index].normal_image->loaded_image_protocol_instance =
      loaded_image;
    table[index].normal_image->image_handle = image_handle;

    /* Increase the number of EFI_DEBUG_IMAGE_INFO elements and
     * set the efi_m_debug_info_table_header in modified status.
     */
    efi_m_debug_info_table_header.table_size++;
    efi_m_debug_info_table_header.update_status |=
      EFI_DEBUG_IMAGE_INFO_TABLE_MODIFIED;
  } else {
    return EFI_STATUS_OUT_OF_RESOURCES;
  }

  efi_m_debug_info_table_header.update_status &=
    ~EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;

  return EFI_STATUS_SUCCESS;
}

void efi_core_remove_debug_image_info_entry(EfiHandle image_handle)
{
  efi_m_debug_info_table_header.update_status |=
    EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;

  union EfiDebugImageInfo *table;
  table = efi_m_debug_info_table_header.efi_debug_image_info_table;

  for (uint32_t index = 0; index < efi_m_max_table_entries; index++) {
    if (table[index].normal_image &&
	table[index].normal_image->image_handle == image_handle) {
      /* Found a match. Free up the table entry.
       * Move the tail of the table one slot to the front.
       */
      free_pool(table[index].normal_image);

      memmove(&table[index],
	      &table[index + 1],
	      (efi_m_debug_info_table_header.table_size -
	       index - 1) * EFI_DEBUG_TABLE_ENTRY_SIZE);

      /* Decrease the number of EFI_DEBUG_IMAGE_INFO
       * elements and set the efi_m_debug_info_table_header
       * in modified status.
       */
      efi_m_debug_info_table_header.table_size--;
      table[efi_m_debug_info_table_header.table_size].normal_image =
	nullptr;
      efi_m_debug_info_table_header.update_status |=
	EFI_DEBUG_IMAGE_INFO_TABLE_MODIFIED;
      break;
    }
  }

  efi_m_debug_info_table_header.update_status &=
    ~EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;
}

EfiStatus setup_debug_support(EfiSystemTable &table,
			      char *image_base,
			      size_t virtual_size,
			      const char *dev_name) {
  EfiLoadedImageProtocol *efiLoadedImageProtocol = nullptr;

  allocate_pool(EFI_MEMORY_TYPE_BOOT_SERVICES_DATA,
		sizeof(EfiLoadedImageProtocol),
		reinterpret_cast<void **>(&efiLoadedImageProtocol));
  memset(efiLoadedImageProtocol, 0, sizeof(EfiLoadedImageProtocol));

  efiLoadedImageProtocol->revision = EFI_LOADED_IMAGE_PROTOCOL_REVISION;
  efiLoadedImageProtocol->system_table = &table;
  efiLoadedImageProtocol->image_base = reinterpret_cast<void *>(image_base);
  efiLoadedImageProtocol->image_size = virtual_size;
  char *device_buf = nullptr;
  /* fpsize is the size of the struct that EfiDevicePathProtocol stores the
     dev_name. And dev_name will be converted to UTF-16, and ended by null
     character. So the size is the length of dev_name plus one, and multiply
     by 2 */
  size_t fpsize = sizeof(EfiDevicePathProtocol) + 2 * (strlen(dev_name) + 1);
  allocate_pool(EFI_MEMORY_TYPE_BOOT_SERVICES_DATA,
		fpsize + sizeof(EfiDevicePathProtocol),
		reinterpret_cast<void **>(&device_buf));
  memset(device_buf, 0, fpsize + sizeof(EfiDevicePathProtocol));
  struct EFI_DEVICE_PATH_FILE_PATH_PROTOCOL *fp = reinterpret_cast<struct EFI_DEVICE_PATH_FILE_PATH_PROTOCOL *>(device_buf);
  struct EfiDevicePathProtocol *dp_end = reinterpret_cast<EfiDevicePathProtocol *>(device_buf + fpsize);
  fp->dp.type = EFI_DEVICE_PATH_TYPE_MEDIA_DEVICE;
  fp->dp.sub_type = EFI_DEVICE_PATH_SUB_TYPE_FILE_PATH;
  fp->dp.length[0] = fpsize % 256;
  fp->dp.length[1] = fpsize / 256;
  utf8_to_utf16(reinterpret_cast<char16_t *>(&fp->str), dev_name, strlen(dev_name) + 1);
  dp_end->type = EFI_DEVICE_PATH_TYPE_END;
  dp_end->sub_type = EFI_DEVICE_PATH_SUB_TYPE_END;
  dp_end->length[0] = sizeof(EfiDevicePathProtocol) % 256;
  dp_end->length[1] = sizeof(EfiDevicePathProtocol) / 256;
  efiLoadedImageProtocol->file_path = reinterpret_cast<EfiDevicePathProtocol *>(device_buf);
  return efi_core_new_debug_image_info_entry(EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL,
					     efiLoadedImageProtocol,
					     image_base);
}

void teardown_debug_support(char *image_base) {
  union EfiDebugImageInfo *table = efi_m_debug_info_table_header.efi_debug_image_info_table;

  for (uint32_t index = 0; index < efi_m_debug_info_table_header.table_size; index++) {
    if (table[index].normal_image &&
	table[index].normal_image->image_handle == image_base) {
      /* Found a match. Get device_buf and efiLoadedImageProtocol. */
      EfiLoadedImageProtocol *efiLoadedImageProtocol = table[index].normal_image->loaded_image_protocol_instance;
      char *device_buf = reinterpret_cast<char *>(efiLoadedImageProtocol->file_path);
      /* Free resources */
      efi_core_remove_debug_image_info_entry(image_base);
      free_pool(device_buf);
      free_pool(efiLoadedImageProtocol);

      return;
    }
  }
}
