/*
 * Copyright (C) 2023 The Android Open Source Project
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
 * SPDX-License-Identifier: Apache-2.0 OR BSD-2-Clause-Patent
 *
 * You may choose to use or redistribute this file under
 *  (a) the Apache License, Version 2.0, or
 *  (b) the BSD 2-Clause Patent license.
 *
 * Unless you expressly elect the BSD-2-Clause-Patent terms, the Apache-2.0
 * terms apply by default.
 */

#ifndef __DEVICE_PATH_PROTOCOL_H__
#define __DEVICE_PATH_PROTOCOL_H__

#include <uefi/types.h>

typedef struct EfiDevicePathProtocol {
  uint8_t type;
  uint8_t sub_type;
  uint8_t length[2];
} EfiDevicePathProtocol;

#define EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID \
  EFI_GUID(0x8b843e20, 0x8132, 0x4852, 0x90, 0xcc, 0x55, 0x1a, 0x4e, 0x4a, 0x7f, 0x1c)

typedef struct EfiDevicePathToTextProtocol {
  uint16_t* (*convert_device_node_to_text)(struct EfiDevicePathProtocol* device_node,
                                           bool display_only, bool allow_shortcuts);
  uint16_t* (*convert_device_path_to_text)(struct EfiDevicePathProtocol* device_path,
                                           bool display_only, bool allow_shortcuts);
} EfiDevicePathToTextProtocol;

static const uint8_t EFI_DEVICE_PATH_TYPE_END_OF_HARDWARE_DEVICE_PATH = 0x7F;

static const uint8_t EFI_DEVICE_PATH_TYPE_MEDIA_DEVICE_PATH = 0x04;

static const uint8_t EFI_END_OF_HARDWARE_DEVICE_PATH_SUB_TYPE_END_ENTIRE_DEVICE_PATH = 0xFF;

static const uint8_t EFI_MEDIA_DEVICE_PATH_SUB_TYPE_VENDOR = 0x03;

#endif  //__DEVICE_PATH_PROTOCOL_H__
