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
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <uefi/protocols/hii_protocol.h>

namespace {
  const int EFI_HII_PACKAGE_TYPE_SHIFT = 24;
  const uint32_t EFI_HII_PACKAGE_TYPE_MASK = 0xff;
  const int EFI_HII_PACKAGE_LEN_SHIFT = 0;
  const uint32_t EFI_HII_PACKAGE_LEN_MASK = 0xffffff;

  struct efi_string_info {
    uint16_t *string;
  };

  struct efi_string_table {
    struct list_node node;
    uint16_t language_name;
    char *language;
    uint32_t nstrings;
    struct efi_string_info *strings;
  };

  struct efi_hii_packagelist {
    struct list_node node;
    EfiHandle driver_handle;
    uint32_t max_string_id;
    struct list_node string_tables;     /* list of efi_string_table */
  };

  list_node efi_package_lists = LIST_INITIAL_VALUE(efi_package_lists);

  static uint32_t efi_hii_package_type(const EfiHiiPackageHeader *header) {
    uint32_t fields;

    fields = header->fields;

    return (fields >> EFI_HII_PACKAGE_TYPE_SHIFT) & EFI_HII_PACKAGE_TYPE_MASK;
  }

  static uint32_t efi_hii_package_len(const EfiHiiPackageHeader *header) {
    uint32_t fields;

    fields = header->fields;

    return (fields >> EFI_HII_PACKAGE_LEN_SHIFT) & EFI_HII_PACKAGE_LEN_MASK;
  }

  struct efi_hii_packagelist *new_packagelist(void) {
    struct efi_hii_packagelist *hii;

    hii = reinterpret_cast<struct efi_hii_packagelist *>(malloc(sizeof(struct efi_hii_packagelist)));
    if (!hii)
      return nullptr;
    memset(hii, 0, sizeof(struct efi_hii_packagelist));
    hii->node = LIST_INITIAL_CLEARED_VALUE;
    list_add_tail(&efi_package_lists, &hii->node);
    hii->string_tables = LIST_INITIAL_VALUE(hii->string_tables);

    return hii;
  }

  static void free_strings_table(struct efi_string_table *stbl) {
    for (uint32_t i = 0; i < stbl->nstrings; i++)
      free(stbl->strings[i].string);
    free(stbl->strings);
    free(stbl->language);
    free(stbl);
  }

  static void remove_strings_package(struct efi_hii_packagelist *hii) {
    while (true) {
      struct efi_string_table *stbl;

      stbl = reinterpret_cast<struct efi_string_table *>(list_remove_head(&(hii->string_tables)));
      if (!stbl) {
        break;
      }

      free_strings_table(stbl);
    }
  }

  static void free_packagelist(struct efi_hii_packagelist *hii) {
        remove_strings_package(hii);

        list_delete(&hii->node);
        free(hii);
  }

  EfiStatus add_packages(struct efi_hii_packagelist *hii,
                         const struct EfiHiiPackageListHeader *package_list) {
    struct EfiHiiPackageHeader *package;
    void *end;
    EfiStatus ret = EFI_STATUS_SUCCESS;

    end = ((char *)package_list) + package_list->package_length;

    package = reinterpret_cast<struct EfiHiiPackageHeader *>(((char *)package_list) + sizeof(*package_list));
    while ((void *)package < end) {
      switch (efi_hii_package_type(package)) {
      case EFI_HII_PACKAGE_END:
        goto out;
      default:
        break;
      }

      package = reinterpret_cast<struct EfiHiiPackageHeader *>((char *)package + efi_hii_package_len(package));
    }
  out:
    return ret;
  }

  static bool efi_hii_packagelist_exists(EfiHiiHandle package_list) {
    struct efi_hii_packagelist *hii = nullptr;
    bool found = false;
    list_for_every_entry(&efi_package_lists, hii, struct efi_hii_packagelist, node) {
      if (hii == package_list) {
        found = true;
        break;
      }
    }

    return found;
  }

  EfiStatus new_package_list(EfiHiiDatabaseProtocol *self,
                             const struct EfiHiiPackageListHeader *package_list,
                             const EfiHandle driver_handle,
                             EfiHiiHandle *handle) {
    struct efi_hii_packagelist *hii;
    EfiStatus ret;

    if (!package_list || !handle)
      return EFI_STATUS_INVALID_PARAMETER;

    hii = new_packagelist();
    if (!hii)
      return EFI_STATUS_OUT_OF_RESOURCES;

    ret = add_packages(hii, package_list);
    if (ret != EFI_STATUS_SUCCESS) {
      free_packagelist(hii);
      return ret;
    }

    hii->driver_handle = driver_handle;
    *handle = hii;

    return EFI_STATUS_SUCCESS;
  }

  EfiStatus remove_package_list(struct EfiHiiDatabaseProtocol *self,
                                EfiHiiHandle handle) {
    struct efi_hii_packagelist *hii = reinterpret_cast<struct efi_hii_packagelist *>(handle);

    if (!handle || !efi_hii_packagelist_exists(handle))
      return EFI_STATUS_NOT_FOUND;

    free_packagelist(hii);

    return EFI_STATUS_SUCCESS;
  }

  EfiStatus update_package_list(struct EfiHiiDatabaseProtocol *self,
                                EfiHiiHandle handle,
                                const struct EfiHiiPackageListHeader *package_list) {
    printf("UEFI: Hii: update_package_list is not supported\n");
    return EFI_STATUS_UNSUPPORTED;
  }

  EfiStatus list_package_lists(struct EfiHiiDatabaseProtocol *self,
                               uint8_t package_type,
                               const EfiGuid *package_guid,
                               size_t *buffer_size,
                               EfiHiiHandle *handle) {
    printf("UEFI: Hii: list_package_lists is not supported\n");
    return EFI_STATUS_UNSUPPORTED;
  }

  EfiStatus export_package_lists(struct EfiHiiDatabaseProtocol *self,
                                 EfiHiiHandle handle,
                                 size_t *buffer_size,
                                 struct EfiHiiPackageListHeader *buffer) {
    printf("UEFI: Hii: export_package_lists is not supported\n");
    return EFI_STATUS_UNSUPPORTED;
  }

  EfiStatus register_package_notify(struct EfiHiiDatabaseProtocol *self,
                                    uint8_t package_type,
                                    const EfiGuid *package_guid,
                                    const void *package_notify_fn,
                                    size_t notify_type,
                                    EfiHandle *notify_handle) {
    printf("UEFI: Hii: register_package_notify is not supported\n");
    return EFI_STATUS_UNSUPPORTED;
  }

  EfiStatus unregister_package_notify(struct EfiHiiDatabaseProtocol *self,
                                      EfiHandle notification_handle) {
    printf("UEFI: Hii: unregister_package_notify is not supported\n");
    return EFI_STATUS_UNSUPPORTED;
  }

  EfiStatus find_keyboard_layouts(struct EfiHiiDatabaseProtocol *self,
                                  uint16_t *key_guid_buffer_length,
                                  EfiGuid *key_guid_buffer) {
    printf("UEFI: Hii: find_keyboard_layouts is not supported\n");
    return EFI_STATUS_UNSUPPORTED;
  }

  EfiStatus get_keyboard_layout(struct EfiHiiDatabaseProtocol *self,
                                EfiGuid *key_guid,
                                uint16_t *keyboard_layout_length,
                                struct EfiHiiKeyboardLayout *keyboard_layout) {
    printf("UEFI: Hii: get_keyboard_layout is not supported\n");
    return EFI_STATUS_UNSUPPORTED;
  }

  EfiStatus set_keyboard_layout(struct EfiHiiDatabaseProtocol *self,
                                EfiGuid *key_guid) {
    printf("UEFI: Hii: set_keyboard_layout is not supported");
    return EFI_STATUS_UNSUPPORTED;
  }

  EfiStatus get_package_list_handle(struct EfiHiiDatabaseProtocol *self,
                                    EfiHiiHandle package_list_handle,
                                    EfiHandle *driver_handle) {
    printf("UEFI: Hii: get_package_list_handle is not supported\n");
    return EFI_STATUS_UNSUPPORTED;
  }

}

__WEAK EfiHiiDatabaseProtocol* open_hii_database_protocol() {
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
    .get_package_list_handle = get_package_list_handle
  };
  return &protocol;
}
