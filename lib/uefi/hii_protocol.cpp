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
 */

#include <lk/list.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <uefi/protocols/hii_protocol.h>

namespace {

constexpr uint32_t kPackageTypeShift = 24;
constexpr uint32_t kPackageTypeMask = 0xff;
constexpr uint32_t kPackageLengthMask = 0xffffff;

struct EfiHiiPackageList {
  list_node node;
  EfiHandle driver_handle;
  EfiGuid package_list_guid{};
};

list_node package_lists = LIST_INITIAL_VALUE(package_lists);

uint32_t package_type(const EfiHiiPackageHeader *header) {
  const uint32_t fields = header->fields;
  return (fields >> kPackageTypeShift) & kPackageTypeMask;
}

uint32_t package_length(const EfiHiiPackageHeader *header) {
  const uint32_t fields = header->fields;
  return fields & kPackageLengthMask;
}

bool package_list_is_valid(const EfiHiiPackageListHeader *package_list) {
  if (package_list->package_length <
      sizeof(*package_list) + sizeof(EfiHiiPackageHeader)) {
    return false;
  }

  const auto *package = reinterpret_cast<const EfiHiiPackageHeader *>(
      reinterpret_cast<const uint8_t *>(package_list) + sizeof(*package_list));
  size_t remaining = package_list->package_length - sizeof(*package_list);

  while (remaining >= sizeof(*package)) {
    const uint32_t length = package_length(package);
    if (length < sizeof(*package) || length > remaining) {
      return false;
    }

    if (package_type(package) == EFI_HII_PACKAGE_END) {
      return length == sizeof(*package) && length == remaining;
    }

    remaining -= length;
    package = reinterpret_cast<const EfiHiiPackageHeader *>(
        reinterpret_cast<const uint8_t *>(package) + length);
  }

  return false;
}

bool guid_equal(const EfiGuid &left, const EfiGuid &right) {
  return memcmp(&left, &right, sizeof(left)) == 0;
}

EfiHiiPackageList *find_package_list(EfiHiiHandle handle) {
  EfiHiiPackageList *package_list = nullptr;
  list_for_every_entry(&package_lists, package_list, EfiHiiPackageList, node) {
    if (package_list == handle) {
      return package_list;
    }
  }
  return nullptr;
}

bool package_list_guid_exists(const EfiGuid &guid, EfiHandle driver_handle) {
  EfiHiiPackageList *package_list = nullptr;
  list_for_every_entry(&package_lists, package_list, EfiHiiPackageList, node) {
    if (package_list->driver_handle == driver_handle &&
        guid_equal(package_list->package_list_guid, guid)) {
      return true;
    }
  }
  return false;
}

EfiStatus new_package_list(const EfiHiiDatabaseProtocol *self,
                           const EfiHiiPackageListHeader *package_list,
                           EfiHandle driver_handle, EfiHiiHandle *handle) {
  if (self == nullptr || package_list == nullptr || handle == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }
  *handle = nullptr;

  if (!package_list_is_valid(package_list)) {
    return EFI_STATUS_INVALID_PARAMETER;
  }

  EfiGuid package_list_guid;
  memcpy(&package_list_guid, &package_list->package_list_guid,
         sizeof(package_list_guid));
  if (package_list_guid_exists(package_list_guid, driver_handle)) {
    return EFI_STATUS_INVALID_PARAMETER;
  }

  auto *hii =
      reinterpret_cast<EfiHiiPackageList *>(malloc(sizeof(EfiHiiPackageList)));
  if (hii == nullptr) {
    return EFI_STATUS_OUT_OF_RESOURCES;
  }

  *hii = {};
  hii->node = LIST_INITIAL_CLEARED_VALUE;
  hii->driver_handle = driver_handle;
  hii->package_list_guid = package_list_guid;
  list_add_tail(&package_lists, &hii->node);
  *handle = hii;
  return EFI_STATUS_SUCCESS;
}

EfiStatus remove_package_list(const EfiHiiDatabaseProtocol *self,
                              EfiHiiHandle handle) {
  if (self == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }

  EfiHiiPackageList *package_list = find_package_list(handle);
  if (package_list == nullptr) {
    return EFI_STATUS_NOT_FOUND;
  }

  list_delete(&package_list->node);
  free(package_list);
  return EFI_STATUS_SUCCESS;
}

EfiStatus update_package_list(const EfiHiiDatabaseProtocol *self,
                              EfiHiiHandle handle,
                              const EfiHiiPackageListHeader *package_list) {
  printf("UEFI: Hii: update_package_list is not supported\n");
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus list_package_lists(const EfiHiiDatabaseProtocol *self,
                             uint8_t package_type, const EfiGuid *package_guid,
                             size_t *buffer_size, EfiHiiHandle *handle) {
  printf("UEFI: Hii: list_package_lists is not supported\n");
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus export_package_lists(const EfiHiiDatabaseProtocol *self,
                               EfiHiiHandle handle, size_t *buffer_size,
                               EfiHiiPackageListHeader *buffer) {
  printf("UEFI: Hii: export_package_lists is not supported\n");
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus register_package_notify(const EfiHiiDatabaseProtocol *self,
                                  uint8_t package_type,
                                  const EfiGuid *package_guid,
                                  EfiHiiDatabaseNotify package_notify_fn,
                                  EfiHiiDatabaseNotifyType notify_type,
                                  EfiHandle *notify_handle) {
  printf("UEFI: Hii: register_package_notify is not supported\n");
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus unregister_package_notify(const EfiHiiDatabaseProtocol *self,
                                    EfiHandle notification_handle) {
  printf("UEFI: Hii: unregister_package_notify is not supported\n");
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus find_keyboard_layouts(const EfiHiiDatabaseProtocol *self,
                                uint16_t *key_guid_buffer_length,
                                EfiGuid *key_guid_buffer) {
  printf("UEFI: Hii: find_keyboard_layouts is not supported\n");
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus get_keyboard_layout(const EfiHiiDatabaseProtocol *self,
                              EfiGuid *key_guid,
                              uint16_t *keyboard_layout_length,
                              EfiHiiKeyboardLayout *keyboard_layout) {
  printf("UEFI: Hii: get_keyboard_layout is not supported\n");
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus set_keyboard_layout(const EfiHiiDatabaseProtocol *self,
                              EfiGuid *key_guid) {
  printf("UEFI: Hii: set_keyboard_layout is not supported\n");
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus get_package_list_handle(const EfiHiiDatabaseProtocol *self,
                                  EfiHiiHandle handle,
                                  EfiHandle *driver_handle) {
  if (self == nullptr || driver_handle == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }

  EfiHiiPackageList *package_list = find_package_list(handle);
  if (package_list == nullptr) {
    return EFI_STATUS_NOT_FOUND;
  }

  *driver_handle = package_list->driver_handle;
  return EFI_STATUS_SUCCESS;
}

}  // namespace

__WEAK EfiHiiDatabaseProtocol *open_hii_database_protocol() {
  static EfiHiiDatabaseProtocol protocol = {
      .new_package_list = new_package_list,
      .remove_package_list = remove_package_list,
      .update_package_list = update_package_list,
      .list_package_lists = list_package_lists,
      .export_package_lists = export_package_lists,
      .register_package_notify = register_package_notify,
      .unregister_package_notify = unregister_package_notify,
      .find_keyboard_layouts = find_keyboard_layouts,
      .get_keyboard_layout = get_keyboard_layout,
      .set_keyboard_layout = set_keyboard_layout,
      .get_package_list_handle = get_package_list_handle,
  };
  return &protocol;
}
