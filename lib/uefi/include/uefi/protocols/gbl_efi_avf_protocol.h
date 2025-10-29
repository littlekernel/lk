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
// See gbl/docs/gbl_efi_avf_protocol.md for details.

#ifndef __GBL_AVF_PROTOCOL_H__
#define __GBL_AVF_PROTOCOL_H__

#include <uefi/types.h>

static const uint64_t GBL_EFI_AVF_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 1);

typedef struct GblEfiAvfProtocol {
  uint64_t revision;

  EfiStatus (*read_vendor_dice_handover)(struct GblEfiAvfProtocol* self,
                                         /* in-out */ size_t* handover_size,
                                         /* out */ uint8_t* handover);

  EfiStatus (*read_secretkeeper_public_key)(
      struct GblEfiAvfProtocol* self,
      /* in-out */ size_t* public_key_size,
      /* out */ uint8_t* public_key);

} GblEfiAvfProtocol;

#endif  //__GBL_AVF_PROTOCOL_H__
