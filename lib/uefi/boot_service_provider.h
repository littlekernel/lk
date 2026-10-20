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
#ifndef __BOOT_SERVICE_PROVIDER_
#define __BOOT_SERVICE_PROVIDER_

#include <uefi/boot_service.h>
#include <uefi/system_table.h>

void setup_boot_service_table(EfiBootService *service);

static constexpr auto LINUX_EFI_LOADED_IMAGE_FIXED_GUID =
    EfiGuid{0xf5a37b6d,
            0x3344,
            0x42a5,
            {0xb6, 0xbb, 0x97, 0x86, 0x48, 0xc1, 0x89, 0x0a}};

static constexpr auto EFI_TCG2_PROTOCOL_GUID =
    EfiGuid{0x607f766c,
            0x7455,
            0x42be,
            {0x93, 0x0b, 0xe4, 0xd7, 0x6d, 0xb2, 0x72, 0x0f}};

static constexpr auto EFI_LOAD_FILE2_PROTOCOL_GUID =
    EfiGuid{0x4006c0c1,
            0xfcb3,
            0x403e,
            {0x99, 0x6d, 0x4a, 0x6c, 0x87, 0x24, 0xe0, 0x6d}};

// This function would be called from GBL before jumping into android kernel
// LK provides a default no-op implementation that is weakly linked,
// different platforms can override with their own implementation.
EfiStatus exit_boot_services(EfiHandle image_handle, size_t map_key);

#endif
