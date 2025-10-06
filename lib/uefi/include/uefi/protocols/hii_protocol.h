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

#pragma once

#include <lk/compiler.h>
#include <stddef.h>
#include <stdint.h>
#include <uefi/types.h>

#define EFI_HII_PACKAGE_TYPE_ALL 0x00
#define EFI_HII_PACKAGE_TYPE_GUID 0x01
#define EFI_HII_PACKAGE_FORMS 0x02
#define EFI_HII_PACKAGE_STRINGS 0x04
#define EFI_HII_PACKAGE_FONTS 0x05
#define EFI_HII_PACKAGE_IMAGES 0x06
#define EFI_HII_PACKAGE_SIMPLE_FONTS 0x07
#define EFI_HII_PACKAGE_DEVICE_PATH 0x08
#define EFI_HII_PACKAGE_KEYBOARD_LAYOUT 0x09
#define EFI_HII_PACKAGE_ANIMATIONS 0x0a
#define EFI_HII_PACKAGE_END 0xdf
#define EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN 0xe0
#define EFI_HII_PACKAGE_TYPE_SYSTEM_END 0xff

typedef void *EfiHiiHandle;
typedef size_t EfiHiiDatabaseNotifyType;

typedef struct EfiHiiPackageListHeader {
  EfiGuid package_list_guid;
  uint32_t package_length;
} __PACKED EfiHiiPackageListHeader;

typedef struct EfiHiiPackageHeader {
  uint32_t fields;
} __PACKED EfiHiiPackageHeader;

typedef struct EfiHiiKeyboardLayout {
  uint16_t layout_length;
  EfiGuid guid;
  uint32_t layout_descriptor_string_offset;
  uint8_t descriptor_count;
} __PACKED EfiHiiKeyboardLayout;

typedef EfiStatus (*EfiHiiDatabaseNotify)(uint8_t package_type,
                                          const EfiGuid *package_guid,
                                          const EfiHiiPackageHeader *package,
                                          EfiHiiHandle handle,
                                          EfiHiiDatabaseNotifyType notify_type);

typedef struct EfiHiiDatabaseProtocol EfiHiiDatabaseProtocol;

struct EfiHiiDatabaseProtocol {
  EfiStatus (*new_package_list)(const EfiHiiDatabaseProtocol *self,
                                const EfiHiiPackageListHeader *package_list,
                                EfiHandle driver_handle, EfiHiiHandle *handle);
  EfiStatus (*remove_package_list)(const EfiHiiDatabaseProtocol *self,
                                   EfiHiiHandle handle);
  EfiStatus (*update_package_list)(const EfiHiiDatabaseProtocol *self,
                                   EfiHiiHandle handle,
                                   const EfiHiiPackageListHeader *package_list);
  EfiStatus (*list_package_lists)(const EfiHiiDatabaseProtocol *self,
                                  uint8_t package_type,
                                  const EfiGuid *package_guid,
                                  size_t *buffer_size, EfiHiiHandle *handle);
  EfiStatus (*export_package_lists)(const EfiHiiDatabaseProtocol *self,
                                    EfiHiiHandle handle, size_t *buffer_size,
                                    EfiHiiPackageListHeader *buffer);
  EfiStatus (*register_package_notify)(const EfiHiiDatabaseProtocol *self,
                                       uint8_t package_type,
                                       const EfiGuid *package_guid,
                                       EfiHiiDatabaseNotify package_notify_fn,
                                       EfiHiiDatabaseNotifyType notify_type,
                                       EfiHandle *notify_handle);
  EfiStatus (*unregister_package_notify)(const EfiHiiDatabaseProtocol *self,
                                         EfiHandle notification_handle);
  EfiStatus (*find_keyboard_layouts)(const EfiHiiDatabaseProtocol *self,
                                     uint16_t *key_guid_buffer_length,
                                     EfiGuid *key_guid_buffer);
  EfiStatus (*get_keyboard_layout)(const EfiHiiDatabaseProtocol *self,
                                   EfiGuid *key_guid,
                                   uint16_t *keyboard_layout_length,
                                   EfiHiiKeyboardLayout *keyboard_layout);
  EfiStatus (*set_keyboard_layout)(const EfiHiiDatabaseProtocol *self,
                                   EfiGuid *key_guid);
  EfiStatus (*get_package_list_handle)(const EfiHiiDatabaseProtocol *self,
                                       EfiHiiHandle package_list_handle,
                                       EfiHandle *driver_handle);
};
