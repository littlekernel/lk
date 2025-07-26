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

#include <uefi/system_table.h>
#include <uefi/types.h>

struct EfiSystemTablePointer {
  uint64_t signature;
  EfiSystemTable *system_table_base;
  uint32_t crc32;
};

EfiStatus efi_initialize_system_table_pointer(struct EfiSystemTable *system_table);

#endif
