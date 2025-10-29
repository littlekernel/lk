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

#ifndef __BLOCK_IO2_PROTOCOL_H__
#define __BLOCK_IO2_PROTOCOL_H__

#include <uefi/types.h>

#include "block_io_protocol.h"

typedef struct EfiBlockIoMedia EfiBlockIoMedia;
typedef struct EfiBlockIo2Protocol EfiBlockIo2Protocol;

typedef struct {
  EfiEvent event;
  EfiStatus transaction_status;
} EfiBlockIo2Token;

struct EfiBlockIo2Protocol {
  EfiBlockIoMedia* media;
  EfiStatus (*reset)(EfiBlockIo2Protocol* self, bool extended_verification);
  EfiStatus (*read_blocks_ex)(EfiBlockIo2Protocol* self, uint32_t media_id,
                              uint64_t lba, EfiBlockIo2Token* token,
                              size_t buffer_size, void* buffer);
  EfiStatus (*write_blocks_ex)(EfiBlockIo2Protocol* self, uint32_t media_id,
                               uint64_t lba, EfiBlockIo2Token* token,
                               size_t buffer_size, const void* buffer);
  EfiStatus (*flush_blocks_ex)(EfiBlockIo2Protocol* self,
                               EfiBlockIo2Token* token);
};

#endif  //__BLOCK_IO2_PROTOCOL_H__
