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
#include "boot_service_provider.h"
#include "system_table.h"
#include <string.h>

void setup_configuration_table(EfiSystemTable *table) {
  auto &rng = table->configuration_table[0];
  rng.vendor_guid = LINUX_EFI_RANDOM_SEED_TABLE_GUID;
  rng.vendor_table = alloc_page(PAGE_SIZE);
  auto rng_seed = reinterpret_cast<linux_efi_random_seed *>(rng.vendor_table);
  rng_seed->size = 512;
  memset(&rng_seed->bits, 0, rng_seed->size);
  table->number_of_table_entries = 1;
}