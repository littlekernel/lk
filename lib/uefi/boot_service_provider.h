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
    EfiGuid{0x9576e91,
            0x6d3f,
            0x11d2,
            {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

static constexpr auto LINUX_EFI_LOADED_IMAGE_FIXED_GUID =
    EfiGuid{0xf5a37b6d,
            0x3344,
            0x42a5,
            {0xb6, 0xbb, 0x97, 0x86, 0x48, 0xc1, 0x89, 0x0a}};
static constexpr auto EFI_RNG_PROTOCOL_GUID =
    EfiGuid{0x3152bca5,
            0xeade,
            0x433d,
            {0x86, 0x2e, 0xc0, 0x1c, 0xdc, 0x29, 0x1f, 0x44}};

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
static constexpr auto EFI_BLOCK_IO_PROTOCOL_GUID =
    EfiGuid{0x964e5b21,
            0x6459,
            0x11d2,
            {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

static constexpr auto EFI_BLOCK_IO2_PROTOCOL_GUID =
    EfiGuid{0xa77b2472,
            0xe282,
            0x4e9f,
            {0xa2, 0x45, 0xc2, 0xc0, 0xe2, 0x7b, 0xbc, 0xc1}};

static constexpr auto EFI_TEXT_INPUT_PROTOCOL_GUID =
    EfiGuid{0x387477c1,
            0x69c7,
            0x11d2,
            {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

static constexpr auto EFI_GBL_OS_CONFIGURATION_PROTOCOL_GUID =
    EfiGuid{0xdda0d135,
            0xaa5b,
            0x42ff,
            {0x85, 0xac, 0xe3, 0xad, 0x6e, 0xfb, 0x46, 0x19}};

static constexpr auto EFI_DT_FIXUP_PROTOCOL_GUID =
    EfiGuid{0xe617d64c,
            0xfe08,
            0x46da,
            {0xf4, 0xdc, 0xbb, 0xd5, 0x87, 0x0c, 0x73, 0x00}};

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
