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

// This is a custom protocol introduced by GBL.
// See gbl/docs/GBL_EFI_FASTBOOT_USB_PROTOCOL.md for details.

#ifndef __GBL_EFI_FASTBOOT_USB_H__
#define __GBL_EFI_FASTBOOT_USB_H__

#include <uefi/types.h>

EFI_ENUM(GblEfiFastbootRxMode, uint32_t, GBL_EFI_FASTBOOT_RX_MODE_SINGLE_PACKET,
         GBL_EFI_FASTBOOT_RX_MODE_FIXED_LENGTH);

static const uint64_t GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 1);

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

#endif  //__GBL_EFI_FASTBOOT_USB_H__
