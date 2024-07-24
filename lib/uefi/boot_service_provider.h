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

#include "boot_service.h"
#include "system_table.h"

void setup_boot_service_table(EfiBootService *service);

static constexpr auto LOADED_IMAGE_PROTOCOL_GUID =
    EfiGuid{0x5b1b31a1,
            0x9562,
            0x11d2,
            {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

static constexpr auto EFI_DEVICE_PATH_PROTOCOL_GUID =
    EfiGuid{0x09576e91,
            0x6d3f,
            0x11d2,
            {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

static constexpr auto LINUX_EFI_LOADED_IMAGE_FIXED_GUID =
    EfiGuid{0xf5a37b6d,
            0x3344,
            0x42a5,
            {0xb6, 0xbb, 0x97, 0x86, 0x48, 0xc1, 0x89, 0x0a}};

using EFI_IMAGE_UNLOAD = EfiStatus (*)(EfiHandle);

//******************************************************
// EFI_DEVICE_PATH_PROTOCOL
//******************************************************
struct EFI_DEVICE_PATH_PROTOCOL {
  uint8_t Type;
  uint8_t SubType;
  uint8_t Length[2];
};

struct EFI_LOADED_IMAGE_PROTOCOL {
  uint32_t Revision;
  EfiHandle ParentHandle;
  EfiSystemTable *SystemTable;

  // Source location of the image
  EfiHandle DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL *FilePath;
  void *Reserved;

  // Image’s load options
  uint32_t LoadOptionsSize;
  void *LoadOptions;

  // Location where image was loaded
  void *ImageBase;
  uint64_t ImageSize;
  EFI_MEMORY_TYPE ImageCodeType;
  EFI_MEMORY_TYPE ImageDataType;
  EFI_IMAGE_UNLOAD Unload;
};

static constexpr size_t EFI_LOADED_IMAGE_PROTOCOL_REVISION = 0x1000;

#endif
