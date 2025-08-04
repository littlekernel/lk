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
 * SPDX-License-Identifier: Apache-2.0 OR BSD-2-Clause-Patent
 *
 * You may choose to use or redistribute this file under
 *  (a) the Apache License, Version 2.0, or
 *  (b) the BSD 2-Clause Patent license.
 *
 * Unless you expressly elect the BSD-2-Clause-Patent terms, the Apache-2.0
 * terms apply by default.
 */

#pragma once

#include <uefi/types.h>

#define GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_REVISION 0x00000000

typedef enum GBL_EFI_FASTBOOT_RX_MODE {
  SINGLE_PACKET = 0,
  FIXED_LENGTH,
} GblEfiFastbootRxMode;

typedef struct GblEfiFastbootTransportProtocol {
  uint64_t revision;
  const char* description;
  EfiStatus (*start)(struct GblEfiFastbootTransportProtocol* self);
  EfiStatus (*stop)(struct GblEfiFastbootTransportProtocol* self);
  EfiStatus (*receive)(struct GblEfiFastbootTransportProtocol* self,
                       size_t* buffer_size, void* buffer,
                       GblEfiFastbootRxMode mode);
  EfiStatus (*send)(struct GblEfiFastbootTransportProtocol* self,
                    size_t* buffer_size, const void* buffer);
  EfiStatus (*flush)(struct GblEfiFastbootTransportProtocol* self);
} GblEfiFastbootTransportProtocol;
