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

#define GBL_EFI_FASTBOOT_PROTOCOL_REVISION 0x00000000

#define GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8 32

typedef struct GblEfiFastbootPolicy {
  // Indicates whether device can be unlocked
  bool can_unlock;
  // Device firmware supports 'critical' partition locking
  bool has_critical_lock;
  // Indicates whether device allows booting from image loaded directly from
  // RAM.
  bool can_ram_boot;
} GblEfiFastbootPolicy;

// Callback function pointer passed to GblEfiFastbootProtocol.get_var_all.
//
// context: Caller specific context.
// args: An array of NULL-terminated strings that contains the variable name
//       followed by additional arguments if any.
// val: A NULL-terminated string representing the value.
typedef void (*GetVarAllCallback)(void* context, const char* const* args,
                                  size_t num_args, const char* val);

typedef enum EFI_FASTBOOT_MESSAGE_TYPE {
  FB_OKAY,
  FB_FAIL,
  FB_INFO,
} EfiFastbootMessageType;

typedef EfiStatus (*FastbootMessageSender)(void* context,
                                           EfiFastbootMessageType msg_type,
                                           const char* msg, size_t msg_len);

typedef enum GBL_EFI_FASTBOOT_PARTITION_PERMISSION_FLAGS {
  // Firmware can read the given partition and send its data to fastboot client.
  GBL_EFI_FASTBOOT_PARTITION_READ = 0x1 << 0,
  // Firmware can overwrite the given partition.
  GBL_EFI_FASTBOOT_PARTITION_WRITE = 0x1 << 1,
  // Firmware can erase the given partition.
  GBL_EFI_FASTBOOT_PARTITION_ERASE = 0x1 << 2,
} GblEfiFastbootPartitionPermissionFlags;

typedef struct GblEfiFastbootProtocol {
  // Revision of the protocol supported.
  uint32_t revision;
  // Null-terminated UTF-8 encoded string
  char8_t serial_number[GBL_EFI_FASTBOOT_SERIAL_NUMBER_MAX_LEN_UTF8];

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
  EfiStatus (*get_policy)(struct GblEfiFastbootProtocol* self,
                          GblEfiFastbootPolicy* policy);
  EfiStatus (*set_lock)(struct GblEfiFastbootProtocol* self, bool critical,
                        bool lock);
  EfiStatus (*get_lock)(struct GblEfiFastbootProtocol* self, bool critical,
                        bool* out_lock);

  // Local session methods
  EfiStatus (*start_local_session)(struct GblEfiFastbootProtocol* self,
                                   void** ctx);
  EfiStatus (*update_local_session)(struct GblEfiFastbootProtocol* self,
                                    void* ctx, uint8_t* buf, size_t* buf_size);
  EfiStatus (*close_local_session)(struct GblEfiFastbootProtocol* self,
                                   void* ctx);

  // Misc methods
  EfiStatus (*get_partition_permissions)(struct GblEfiFastbootProtocol* self,
                                         const char8_t* part_name,
                                         size_t part_name_len,
                                         uint64_t* permissions);
  EfiStatus (*wipe_user_data)(struct GblEfiFastbootProtocol* self);
  bool (*should_stop_in_fastboot)(struct GblEfiFastbootProtocol* self);
} GblEfiFastbootProtocol;
