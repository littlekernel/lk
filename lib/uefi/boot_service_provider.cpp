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
 */
#include "boot_service_provider.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {

EfiStatus unload(EfiHandle handle) { return SUCCESS; }

bool guid_eq(const EfiGuid *a, const EfiGuid *b) {
  return memcmp(a, b, sizeof(*a)) == 0;
}

bool guid_eq(const EfiGuid *a, const EfiGuid &b) {
  return memcmp(a, &b, sizeof(*a)) == 0;
}

EfiStatus handle_protocol(EfiHandle handle, const EfiGuid *protocol,
                          void **intf) {
  if (guid_eq(protocol, LOADED_IMAGE_PROTOCOL_GUID)) {
    printf("handle_protocol(%p, LOADED_IMAGE_PROTOCOL_GUID, %p);\n", handle,
           intf);
    auto loaded_image = static_cast<EFI_LOADED_IMAGE_PROTOCOL *>(
        malloc(sizeof(EFI_LOADED_IMAGE_PROTOCOL)));
    loaded_image = {};
    loaded_image->Revision = EFI_LOADED_IMAGE_PROTOCOL_REVISION;
    loaded_image->ParentHandle = nullptr;
    loaded_image->SystemTable = nullptr;
    loaded_image->LoadOptionsSize = 0;
    loaded_image->LoadOptions = nullptr;
    loaded_image->Unload = unload;

    *intf = loaded_image;
    return SUCCESS;
  } else if (guid_eq(protocol, LINUX_EFI_LOADED_IMAGE_FIXED_GUID)) {
    printf("handle_protocol(%p, LINUX_EFI_LOADED_IMAGE_FIXED_GUID, %p);\n",
           handle, intf);
    return SUCCESS;
  } else {
    printf("handle_protocol(%p, %p, %p);\n", handle, protocol, intf);
  }
  return UNSUPPORTED;
}

EfiStatus allocate_pool(EfiMemoryType pool_type, size_t size, void **buf) {
  if (buf == nullptr) {
    return INVALID_PARAMETER;
  }
  if (size == 0) {
    *buf = nullptr;
    return SUCCESS;
  }
  *buf = malloc(size);
  if (*buf != nullptr) {
    return SUCCESS;
  }
  return OUT_OF_RESOURCES;
}

EfiStatus free_pool(void *mem) {
  free(mem);
  return SUCCESS;
}

EfiStatus get_memory_map(size_t *memory_map_size,
                         EfiMemoryDescriptor *memory_map, size_t *map_key,
                         size_t *desc_size, uint32_t *desc_version) {
  if (memory_map_size) {
    *memory_map_size = 0;
  }
  if (memory_map) {
    memset(memory_map, 0, sizeof(*memory_map));
  }
  if (map_key) {
    *map_key = 0;
  }
  if (desc_size) {
    *desc_size = sizeof(EfiMemoryDescriptor);
  }
  if (desc_version) {
    *desc_version = 0;
  }

  printf("%s(%zu) is unsupported\n", __FUNCTION__, *memory_map_size);
  return UNSUPPORTED;
}

EfiStatus register_protocol_notify(const EfiGuid *protocol, EfiEvent event,
                                   void **registration) {
  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus locate_handle(EfiLocateHandleSearchType search_type,
                        const EfiGuid *protocol, void *search_key,
                        size_t *buf_size, EfiHandle *buf) {

  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus locate_protocol(const EfiGuid *protocol, void *registration,
                          void **intf) {

  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus allocate_pages(EfiAllocatorType type, EfiMemoryType memory_type,
                         size_t pages, EfiPhysicalAddr *memory) {
  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus free_pages(EfiPhysicalAddr memory, size_t pages) {

  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

} // namespace

void setup_boot_service_table(EfiBootService *service) {
  service->handle_protocol = handle_protocol;
  service->allocate_pool = allocate_pool;
  service->free_pool = free_pool;
  service->get_memory_map = get_memory_map;
  service->register_protocol_notify = register_protocol_notify;
  service->locate_handle = locate_handle;
  service->locate_protocol = locate_protocol;
  service->allocate_pages = allocate_pages;
  service->free_pages = free_pages;
}