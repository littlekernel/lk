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

#ifndef __ERASE_BLOCK_PROTOCOL_H__
#define __ERASE_BLOCK_PROTOCOL_H__

#include <uefi/gbl_protocol_utils.h>
#include <uefi/types.h>

static const uint64_t EFI_ERASE_BLOCK_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(2, 60);

typedef struct EfiEraseBlockProtocol EfiEraseBlockProtocol;

typedef struct {
  EfiEvent event;
  EfiStatus transaction_status;
} EfiEraseBlockToken;

struct EfiEraseBlockProtocol {
  uint64_t revision;
  uint32_t erase_length_granularity;
  EfiStatus (*erase_blocks)(EfiEraseBlockProtocol* self, uint32_t media_id,
                            uint64_t lba, EfiEraseBlockToken* token,
                            size_t size);
};

#endif  //__ERASE_BLOCK_PROTOCOL_H__
