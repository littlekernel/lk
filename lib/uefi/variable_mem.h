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

#ifndef __VARIABLE_MEM_
#define __VARIABLE_MEM_

#include <uefi/types.h>

static constexpr auto EFI_GLOBAL_VARIABLE_GUID =
    EfiGuid{0x8be4df61,
            0x93ca,
            0x11d2,
            {0xaa, 0x0d, 0x00, 0xe0, 0x98, 0x03, 0x2b, 0x8c}};

static constexpr uint32_t EFI_VARIABLE_NON_VOLATILE = 0x01;
static constexpr uint32_t EFI_VARIABLE_BOOTSERVICE_ACCESS = 0x02;
static constexpr uint32_t EFI_VARIABLE_RUNTIME_ACCESS = 0x04;
static constexpr uint32_t EFI_VARIABLE_HARDWARE_ERROR_RECORD = 0x08;

EfiStatus efi_get_variable(const char16_t *variable_name,
                           const EfiGuid *guid,
                           uint32_t *attribute,
                           char **data,
                           size_t *data_size);
void efi_set_variable(const char16_t *variable_name,
                      const EfiGuid *guid,
                      uint32_t attribute,
                      const char *data,
                      size_t data_len);

void efi_list_variable(void);
#endif
