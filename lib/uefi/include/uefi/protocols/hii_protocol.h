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

#ifndef __HII_PROTOCOL_H__
#define __HII_PROTOCOL_H__

#include <stddef.h>
#include <stdint.h>
#include <uefi/types.h>

#define EFI_HII_PACKAGE_TYPE_ALL          0x00
#define EFI_HII_PACKAGE_TYPE_GUID         0x01
#define EFI_HII_PACKAGE_FORMS             0x02
#define EFI_HII_PACKAGE_STRINGS           0x04
#define EFI_HII_PACKAGE_FONTS             0x05
#define EFI_HII_PACKAGE_IMAGES            0x06
#define EFI_HII_PACKAGE_SIMPLE_FONTS      0x07
#define EFI_HII_PACKAGE_DEVICE_PATH       0x08
#define EFI_HII_PACKAGE_KEYBOARD_LAYOUT   0x09
#define EFI_HII_PACKAGE_ANIMATIONS        0x0A
#define EFI_HII_PACKAGE_END               0xDF
#define EFI_HII_PACKAGE_TYPE_SYSTEM_BEGIN 0xE0
#define EFI_HII_PACKAGE_TYPE_SYSTEM_END   0xFF

typedef void *EfiHiiHandle;

struct EfiHiiPackageListHeader {
  EfiGuid package_list_guid;
  uint32_t package_length;
} __attribute__((packed));

struct EfiHiiPackageHeader {
  uint32_t fields;
} __attribute__((packed));

struct EfiHiiKeyboardLayout {
  uint16_t layout_length;
  uint8_t guid[16];
  uint32_t layout_descriptor_string_offset;
  uint8_t descriptor_count;
} __attribute__((packed)) ;

typedef struct EfiHiiDatabaseProtocol {
  EfiStatus (*new_package_list)(struct EfiHiiDatabaseProtocol *self,
                                /* in */ const struct EfiHiiPackageListHeader *package_list,
                                /* in */ const EfiHandle driver_handle,
                                /* out */ EfiHiiHandle *handle);
  EfiStatus (*remove_package_list)(struct EfiHiiDatabaseProtocol *self,
                                   /* in */ EfiHiiHandle handle);
  EfiStatus (*update_package_list)(struct EfiHiiDatabaseProtocol *self,
                                   /* in */ EfiHiiHandle handle,
                                   /* in */ const struct EfiHiiPackageListHeader *package_list);
  EfiStatus (*list_package_lists)(struct EfiHiiDatabaseProtocol *self,
                                  /* in */ uint8_t package_type,
                                  /* in */ const EfiGuid *package_guid,
                                  /* in/out */ size_t *buffer_size,
                                  /* out */ EfiHiiHandle *handle);
  EfiStatus (*export_package_lists)(struct EfiHiiDatabaseProtocol *self,
                                    /* in */ EfiHiiHandle handle,
                                    /* in/out */ size_t *buffer_size,
                                    /* out */ struct EfiHiiPackageListHeader *buffer);
  EfiStatus (*register_package_notify)(struct EfiHiiDatabaseProtocol *self,
                                       /* in */ uint8_t package_type,
                                       /* in */ const EfiGuid *package_guid,
                                       /* in */ const void *package_notify_fn,
                                       /* in */ size_t notify_type,
                                       /* out */ EfiHandle *notify_handle);
  EfiStatus (*unregister_package_notify)(struct EfiHiiDatabaseProtocol *self,
                                         /* in */ EfiHandle notification_handle);
  EfiStatus(*find_keyboard_layouts)(struct EfiHiiDatabaseProtocol *self,
                                    /* in/out */ uint16_t *key_guid_buffer_length,
                                    /* out */ EfiGuid *key_guid_buffer);
  EfiStatus(*get_keyboard_layout)(struct EfiHiiDatabaseProtocol *self,
                                  /* in */ EfiGuid *key_guid,
                                  /* in/out */ uint16_t *keyboard_layout_length,
                                  /* out */ struct EfiHiiKeyboardLayout *keyboard_layout);
  EfiStatus(*set_keyboard_layout)(struct EfiHiiDatabaseProtocol *self,
                                  /* in */ EfiGuid *key_guid);
  EfiStatus(*get_package_list_handle)(struct EfiHiiDatabaseProtocol *self,
                                      /* in */ EfiHiiHandle package_list_handle,
                                      /* out */ EfiHandle *driver_handle);
} EfiHiiDatabaseProtocol;

#endif  // __HII_PROTOCOL_H__
