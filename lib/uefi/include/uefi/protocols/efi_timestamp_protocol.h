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
 */

// Reference:
// https://uefi.org/specs/UEFI/2.10/39_Micellaneous_Protocols.html#efi-timestamp-protocol

#pragma once

#include <uefi/types.h>

typedef struct EfiTimestampProperties {
  uint64_t frequency;
  uint64_t end_value;
} EfiTimestampProperties;

typedef struct EfiTimestampProtocol {
  uint64_t (*get_timestamp)();
  EfiStatus (*get_properties)(EfiTimestampProperties *properties);
} EfiTimestampProtocol;
