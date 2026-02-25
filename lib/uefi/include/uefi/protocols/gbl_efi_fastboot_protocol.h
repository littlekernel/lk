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
// See gbl/docs/GBL_EFI_FASTBOOT_PROTOCOL.md for details.

#ifndef __GBL_EFI_FASTBOOT_PROTOCOL_H__
#define __GBL_EFI_FASTBOOT_PROTOCOL_H__

#include <uefi/types.h>

#define GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8 32

static const uint64_t GBL_EFI_FASTBOOT_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 256);

static const size_t GBL_EFI_FASTBOOT_PARTITION_TYPE_BUF_LEN = 56;

// Callback function pointer passed to GblEfiFastbootProtocol.get_var_all.
//
// context: Caller specific context.
// args: An array of NULL-terminated strings that contains the variable name
//       followed by additional arguments if any.
// val: A NULL-terminated string representing the value.
typedef void (*GetVarAllCallback)(void* context, size_t num_args,
                                  const EfiChar8* const* args,
                                  const EfiChar8* val);

EFI_ENUM(GblEfiFastbootMessageType, uint32_t,
         GBL_EFI_FASTBOOT_MESSAGE_TYPE_OKAY, GBL_EFI_FASTBOOT_MESSAGE_TYPE_FAIL,
         GBL_EFI_FASTBOOT_MESSAGE_TYPE_INFO);

typedef EfiStatus (*FastbootMessageSender)(void* context,
                                           GblEfiFastbootMessageType msg_type,
                                           size_t msg_len, const EfiChar8* msg);

EFI_ENUM(GblEfiFastbootCommandExecResult, uint32_t,
         GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_PROHIBITED,
         GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_DEFAULT_IMPL,
         GBL_EFI_FASTBOOT_COMMAND_EXEC_RESULT_CUSTOM_IMPL);

typedef struct GblEfiFastbootProtocol {
  uint64_t revision;
  // Null-terminated UTF-8 encoded string
  EfiChar8 serial_number[GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8];

  // Fastboot variable methods
  EfiStatus (*get_var)(struct GblEfiFastbootProtocol* self, size_t num_args,
                       const EfiChar8* const* args, size_t* buffer_size,
                       EfiChar8* buffer);
  EfiStatus (*get_var_all)(struct GblEfiFastbootProtocol* self, void* context,
                           GetVarAllCallback cb);

  EfiStatus (*get_staged)(struct GblEfiFastbootProtocol* self,
                          size_t* buffer_size, size_t* buffer_remains,
                          uint8_t* buffer);

  EfiStatus (*command_exec)(struct GblEfiFastbootProtocol* self,
                            size_t num_args, const EfiChar8* const* args,
                            size_t download_buffer_size,
                            size_t download_buffer_used_size,
                            uint8_t* download_buffer,
                            GblEfiFastbootCommandExecResult* implementation,
                            FastbootMessageSender sender, void* context);
  EfiStatus (*get_partition_type)(struct GblEfiFastbootProtocol* self,
                                  const EfiChar8* part_name,
                                  size_t* part_type_len, EfiChar8* part_type);
} GblEfiFastbootProtocol;

#endif  // __GBL_EFI_FASTBOOT_PROTOCOL_H__
