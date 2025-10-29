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

#ifndef __LOADED_IMAGE_PROTOCOL_H__
#define __LOADED_IMAGE_PROTOCOL_H__

#include <uefi/gbl_protocol_utils.h>
#include <uefi/system_table.h>
#include <uefi/types.h>

#include "device_path_protocol.h"

static const uint32_t EFI_LOADED_IMAGE_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(1, 0);

typedef struct {
  uint32_t revision;
  EfiHandle parent_handle;
  EfiSystemTable* system_table;
  EfiHandle device_handle;
  EfiDevicePathProtocol* file_path;
  void* reserved;
  uint32_t load_options_size;
  void* load_options;
  void* image_base;
  uint64_t image_size;
  EfiMemoryType image_code_type;
  EfiMemoryType image_data_type;

  EfiStatus (*unload)(EfiHandle img);
} EfiLoadedImageProtocol;

#endif
