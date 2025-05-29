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

#ifndef __CONFIGURATION_TABLE_
#define __CONFIGURATION_TABLE_

#include <uefi/system_table.h>
#include <uefi/types.h>

struct linux_efi_random_seed {
  uint32_t size;
  uint8_t bits[];
};

static constexpr auto LINUX_EFI_RANDOM_SEED_TABLE_GUID =
    EfiGuid{0x1ce1e5bc,
            0x7ceb,
            0x42f2,
            {0x81, 0xe5, 0x8a, 0xad, 0xf1, 0x80, 0xf5, 0x7b}};
static constexpr auto DEVICE_TREE_GUID =
    EfiGuid{0xb1b621d5,
            0xf19c,
            0x41a5,
            {0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0}};

void setup_configuration_table(EfiSystemTable *table);

#endif
