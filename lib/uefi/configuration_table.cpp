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

#include "configuration_table.h"

#include <libfdt.h>
#include <string.h>
#include <uefi/system_table.h>

#include "debug_support.h"
#include "platform.h"
#include "uefi_platform.h"

void setup_configuration_table(EfiSystemTable *table) {
  auto &rng = table->configuration_table[table->number_of_table_entries++];
  rng.vendor_guid = LINUX_EFI_RANDOM_SEED_TABLE_GUID;
  rng.vendor_table = alloc_page(PAGE_SIZE);
  auto rng_seed = reinterpret_cast<linux_efi_random_seed *>(rng.vendor_table);
  rng_seed->size = 512;
  memset(&rng_seed->bits, 0, rng_seed->size);

  const void *fdt = get_fdt();
  if (fdt != nullptr) {
    auto &dtb = table->configuration_table[table->number_of_table_entries++];
    dtb.vendor_guid = DEVICE_TREE_GUID;
    const auto fdt_size = fdt_totalsize(fdt);
    dtb.vendor_table = alloc_page(fdt_size);
    memcpy(dtb.vendor_table, fdt, fdt_size);
  }

  auto &debug_image_info_table =
      table->configuration_table[table->number_of_table_entries++];
  debug_image_info_table.vendor_guid = EFI_DEBUG_IMAGE_INFO_TABLE_GUID;
  debug_image_info_table.vendor_table = &efi_m_debug_info_table_header;
}
