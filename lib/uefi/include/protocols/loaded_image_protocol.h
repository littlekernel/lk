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
 */

#include "system_table.h"
#include "types.h"

#include "protocols/device_path_protocol.h"

typedef struct {
  uint32_t revision;
  EfiHandle parent_handle;
  EfiSystemTable* system_table;
  EfiHandle device_handle;
  EfiDevicePathToTextProtocol* file_path;
  void* reserved;
  uint32_t load_options_size;
  void* load_options;
  void* image_base;
  uint64_t image_size;
  EfiMemoryType image_code_type;
  EfiMemoryType image_data_type;

  EfiStatus (*unload)(EfiHandle img);
} EfiLoadedImageProtocol;
