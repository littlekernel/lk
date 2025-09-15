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

#include "types.h"

#define GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8 32

// Callback function pointer passed to GblEfiFastbootProtocol.get_var_all.
//
// context: Caller specific context.
// args: An array of NULL-terminated strings that contains the variable name
//       followed by additional arguments if any.
// val: A NULL-terminated string representing the value.
typedef void (*GetVarAllCallback)(void* context, const char* const* args,
                                  size_t num_args, const char* val);

EFI_ENUM(GBL_GBL_EFI_FASTBOOT_MESSAGE_TYPE, GblEfiFastbootMessageType, uint32_t,
         GBL_EFI_FASTBOOT_MESSAGE_TYPE_OKAY, GBL_EFI_FASTBOOT_MESSAGE_TYPE_FAIL,
         GBL_EFI_FASTBOOT_MESSAGE_TYPE_INFO);

typedef EfiStatus (*FastbootMessageSender)(void* context,
                                           GblEfiFastbootMessageType msg_type,
                                           const char* msg, size_t msg_len);

static const uint64_t GBL_EFI_FASTBOOT_PROTOCOL_REVISION =
    GBL_PROTOCOL_REVISION(0, 1);

EFI_ENUM(GBL_EFI_FASTBOOT_ERASE_ACTION, GblEfiFastbootEraseAction, uint32_t,
         // Treats the partition as a physical on disk partition and erases it.
         GBL_EFI_FASTBOOT_ERASE_ACTION_ERASE_AS_PHYSICAL_PARTITION,
         // Ignores the partition.
         GBL_EFI_FASTBOOT_ERASE_ACTION_NOOP);

typedef struct GblEfiFastbootProtocol {
  uint64_t revision;
  // Null-terminated UTF-8 encoded string
  uint8_t serial_number[GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8];

  // Fastboot variable methods
  EfiStatus (*get_var)(struct GblEfiFastbootProtocol* self,
                       const char* const* args, size_t num_args, uint8_t* out,
                       size_t* out_size);
  EfiStatus (*get_var_all)(struct GblEfiFastbootProtocol* self, void* ctx,
                           GetVarAllCallback cb);

  // Fastboot oem function methods
  EfiStatus (*run_oem_function)(struct GblEfiFastbootProtocol* self,
                                const char* cmd, size_t len,
                                uint8_t* download_buffer,
                                size_t download_data_size,
                                FastbootMessageSender sender, void* ctx);

  // Fastboot get_staged backend
  EfiStatus (*get_staged)(struct GblEfiFastbootProtocol* self, uint8_t* out,
                          size_t* out_size, size_t* out_remain);

  // Device lock methods
  EfiStatus (*set_lock)(struct GblEfiFastbootProtocol* self, bool critical,
                        bool lock);
  EfiStatus (*get_lock)(struct GblEfiFastbootProtocol* self, bool critical,
                        bool* out_lock);

  // Misc methods
  EfiStatus (*vendor_erase)(struct GblEfiFastbootProtocol *self,
                            const uint8_t *part_name, size_t part_name_len,
                            GblEfiFastbootEraseAction *action);
  bool (*should_stop_in_fastboot)(struct GblEfiFastbootProtocol* self);
  EfiStatus (*is_command_allowed)(struct GblEfiFastbootProtocol* self,
                                  size_t num_args, const char* const* args,
                                  size_t download_data_len,
                                  uint8_t* download_data, bool* allowed,
                                  size_t msg_buf_size, uint8_t* msg_buf);

  // Local session methods
  EfiStatus (*start_local_session)(struct GblEfiFastbootProtocol* self,
                                   void** ctx);
  EfiStatus (*update_local_session)(struct GblEfiFastbootProtocol* self,
                                    void* ctx, uint8_t* buf, size_t* buf_size);
  EfiStatus (*close_local_session)(struct GblEfiFastbootProtocol* self,
                                   void* ctx);
} GblEfiFastbootProtocol;

#endif  // __GBL_EFI_FASTBOOT_PROTOCOL_H__
