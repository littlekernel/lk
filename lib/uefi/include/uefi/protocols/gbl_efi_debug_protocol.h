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

#ifndef __GBL_EFI_DEBUG_PROTOCOL_H__
#define __GBL_EFI_DEBUG_PROTOCOL_H__

#include <uefi/gbl_protocol_utils.h>
#include <uefi/types.h>

static const uint64_t GBL_EFI_DEBUG_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 2);

// TODO (b/446226293): add additional tags.
EFI_ENUM(GblEfiDebugErrorTag, uint64_t, GBL_EFI_DEBUG_ERROR_TAG_ASSERTION_ERROR,
         GBL_EFI_DEBUG_ERROR_TAG_PARTITION, GBL_EFI_DEBUG_ERROR_TAG_LOAD_IMAGE,
         GBL_EFI_DEBUG_ERROR_TAG_BOOT_ERROR);

typedef struct GblEfiDebugProtocol {
  uint64_t revision;

  EfiStatus (*fatal_error)(struct GblEfiDebugProtocol* self,
                           const void* frame_ptr, GblEfiDebugErrorTag tag);
} GblEfiDebugProtocol;

#endif  // __GBL_EFI_DEBUG_PROTOCOL_H__ */
