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

#include <stdio.h>
#include <string.h>
#include <lib/cksum.h>
#include <uefi/boot_service.h>
#include <uefi/types.h>

#include "boot_service_provider.h"
#include "memory_protocols.h"

struct EFI_DEVICE_PATH_FILE_PATH_PROTOCOL {
  struct EFI_DEVICE_PATH_PROTOCOL dp;
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
    return OUT_OF_RESOURCES;
  }

  memset(efi_systab_pointer, 0, sizeof(struct EfiSystemTablePointer));

  efi_systab_pointer->signature = EFI_SYSTEM_TABLE_SIGNATURE;
  efi_systab_pointer->system_table_base = system_table;

  crc = crc32(crc,
	      (uint8_t *)efi_systab_pointer,
	      sizeof(struct EfiSystemTablePointer));

  efi_systab_pointer->crc32 = crc;

  return SUCCESS;
}

static uint32_t efi_m_max_table_entries;

static constexpr size_t EFI_DEBUG_TABLE_ENTRY_SIZE = (sizeof(union EfiDebugImageInfo));

EfiStatus efi_core_new_debug_image_info_entry(uint32_t image_info_type,
					      struct EFI_LOADED_IMAGE_PROTOCOL *loaded_image,
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
    allocate_pool(BOOT_SERVICES_DATA,
		  table_size + PAGE_SIZE,
		  reinterpret_cast<void **>(&new_table));

    if (!new_table) {
      efi_m_debug_info_table_header.update_status &=
	~EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;
      return OUT_OF_RESOURCES;
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
  allocate_pool(BOOT_SERVICES_DATA,
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
    return OUT_OF_RESOURCES;
  }

  efi_m_debug_info_table_header.update_status &=
    ~EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;

  return SUCCESS;
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
			      bdev_t *dev) {
  struct EFI_LOADED_IMAGE_PROTOCOL *efiLoadedImageProtocol = nullptr;

  allocate_pool(BOOT_SERVICES_DATA,
		sizeof(struct EFI_LOADED_IMAGE_PROTOCOL),
		reinterpret_cast<void **>(&efiLoadedImageProtocol));
  memset(efiLoadedImageProtocol, 0, sizeof(struct EFI_LOADED_IMAGE_PROTOCOL));

  efiLoadedImageProtocol->Revision = EFI_LOADED_IMAGE_PROTOCOL_REVISION;
  efiLoadedImageProtocol->SystemTable = &table;
  efiLoadedImageProtocol->ImageBase = reinterpret_cast<void *>(image_base);
  efiLoadedImageProtocol->ImageSize = virtual_size;
  char *device_buf = nullptr;
  size_t fpsize = sizeof(struct EFI_DEVICE_PATH_PROTOCOL) + 2 * (strlen(dev->name) + 2);
  allocate_pool(BOOT_SERVICES_DATA,
		fpsize + sizeof(struct EFI_DEVICE_PATH_PROTOCOL),
		reinterpret_cast<void **>(&device_buf));
  memset(device_buf, 0, fpsize + sizeof(struct EFI_DEVICE_PATH_PROTOCOL));
  struct EFI_DEVICE_PATH_FILE_PATH_PROTOCOL *fp = reinterpret_cast<struct EFI_DEVICE_PATH_FILE_PATH_PROTOCOL *>(device_buf);
  struct EFI_DEVICE_PATH_PROTOCOL *dp_end = reinterpret_cast<struct EFI_DEVICE_PATH_PROTOCOL *>(device_buf + fpsize);
  fp->dp.Type = EFI_DEVICE_PATH_TYPE_MEDIA_DEVICE;
  fp->dp.SubType = EFI_DEVICE_PATH_SUB_TYPE_FILE_PATH;
  fp->dp.Length[0] = fpsize % 256;
  fp->dp.Length[1] = fpsize / 256;
  fp->str[0] = '\\';
  for (size_t i = 0; i < strlen(dev->name); i++) {
    fp->str[i+1] = dev->name[i];
  }
  dp_end->Type = EFI_DEVICE_PATH_TYPE_END;
  dp_end->SubType = EFI_DEVICE_PATH_SUB_TYPE_END;
  dp_end->Length[0] = sizeof(struct EFI_DEVICE_PATH_PROTOCOL) % 256;
  dp_end->Length[1] = sizeof(struct EFI_DEVICE_PATH_PROTOCOL) / 256;
  efiLoadedImageProtocol->FilePath = reinterpret_cast<struct EFI_DEVICE_PATH_PROTOCOL *>(device_buf);
  return efi_core_new_debug_image_info_entry(EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL,
					     efiLoadedImageProtocol,
					     image_base);
}

void teardown_debug_support(char *image_base) {
  char *device_buf = nullptr;
  struct EFI_LOADED_IMAGE_PROTOCOL *efiLoadedImageProtocol = nullptr;

  union EfiDebugImageInfo *table;
  table = efi_m_debug_info_table_header.efi_debug_image_info_table;

  for (uint32_t index = 0; index < efi_m_debug_info_table_header.table_size; index++) {
    if (table[index].normal_image &&
	table[index].normal_image->image_handle == image_base) {
      /* Found a match. Get device_buf and efiLoadedImageProtocol.
       */
      efiLoadedImageProtocol = table[index].normal_image->loaded_image_protocol_instance;
      device_buf = reinterpret_cast<char *>(efiLoadedImageProtocol->FilePath);
      break;
    }
  }

  efi_core_remove_debug_image_info_entry(image_base);
  if (device_buf) {
    free_pool(device_buf);
  }
  if (efiLoadedImageProtocol) {
    free_pool(efiLoadedImageProtocol);
  }
}
