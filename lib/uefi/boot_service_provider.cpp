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
#include "boot_service.h"
#include "kernel/vm.h"
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

EfiStatus handle_protocol(const EfiHandle handle, const EfiGuid *protocol,
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
    loaded_image->ImageBase = handle;

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
  if (memory_map_size == nullptr) {
    return INVALID_PARAMETER;
  }
  if (map_key) {
    *map_key = 0;
  }
  if (desc_size) {
    *desc_size = sizeof(EfiMemoryDescriptor);
  }
  if (desc_version) {
    *desc_version = 1;
  }
  vmm_region_t *region = nullptr;
  auto aspace = vmm_get_kernel_aspace();
  size_t num_entries = 0;
  list_for_every_entry(&aspace->region_list, region, vmm_region_t, node) {
    num_entries++;
  }
  const size_t size_needed = num_entries * sizeof(EfiMemoryDescriptor);
  if (*memory_map_size < size_needed) {
    *memory_map_size = size_needed;
    return BUFFER_TOO_SMALL;
  }
  *memory_map_size = size_needed;
  size_t i = 0;
  memset(memory_map, 0, size_needed);
  list_for_every_entry(&aspace->region_list, region, vmm_region_t, node) {
    memory_map[i].virtual_start = region->base;
    memory_map[i].physical_start = memory_map[i].virtual_start;
    memory_map[i].number_of_pages = region->size / PAGE_SIZE;
    i++;
  }

  return SUCCESS;
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
  if (protocol == nullptr) {
    return INVALID_PARAMETER;
  }
  if (memcmp(protocol, &EFI_RNG_PROTOCOL_GUID, sizeof(*protocol)) == 0) {
    printf("%s(EFI_RNG_PROTOCOL_GUID) is unsupported.\n", __FUNCTION__);
    return UNSUPPORTED;
  }
  if (memcmp(protocol, &EFI_TCG2_PROTOCOL_GUID, sizeof(*protocol)) == 0) {
    printf("%s(EFI_TCG2_PROTOCOL_GUID) is unsupported.\n", __FUNCTION__);
    return NOT_FOUND;
  }

  printf("%s(%x %x %x %llx) is unsupported\n", __FUNCTION__, protocol->data1,
         protocol->data2, protocol->data3,
         *reinterpret_cast<const uint64_t *>(&protocol->data4));
  return UNSUPPORTED;
}

EfiStatus allocate_pages(EfiAllocatorType type, EfiMemoryType memory_type,
                         size_t pages, EfiPhysicalAddr *memory) {
  if (memory == nullptr) {
    return INVALID_PARAMETER;
  }
  if (type == ALLOCATE_MAX_ADDRESS && *memory != 0xffffffffffffffff) {
    printf("allocate_pages(%d, %d, %zu, 0x%llx) unsupported\n", type,
           memory_type, pages, *memory);
    return UNSUPPORTED;
  }
  *memory =
      reinterpret_cast<EfiPhysicalAddr>(memalign(PAGE_SIZE, pages * PAGE_SIZE));
  if (*memory == 0) {
    return OUT_OF_RESOURCES;
  }
  return SUCCESS;
}

EfiStatus free_pages(EfiPhysicalAddr memory, size_t pages) {
  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus uninstall_multiple_protocol_interfaces(EfiHandle handle, ...) {
  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}
EfiStatus calculate_crc32(void *data, size_t len, uint32_t *crc32) {
  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus uninstall_protocol_interface(EfiHandle handle,
                                       const EfiGuid *protocol, void *intf) {
  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus load_image(bool boot_policy, EfiHandle parent_image_handle,
                     EfiDevicePathProtocol *path, void *src, size_t src_size,
                     EfiHandle *image_handle) {
  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus locate_device_path(const EfiGuid *protocol,
                             EfiDevicePathProtocol **path, EfiHandle *device) {
  if (memcmp(protocol, &EFI_LOAD_FILE2_PROTOCOL_GUID,
             sizeof(EFI_LOAD_FILE2_PROTOCOL_GUID)) == 0) {
    return NOT_FOUND;
  }
  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus install_configuration_table(const EfiGuid *guid, void *table) {
  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus exit_boot_services(EfiHandle image_handle, size_t map_key) {
  printf("%s is called\n", __FUNCTION__);
  return SUCCESS;
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
  service->uninstall_multiple_protocol_interfaces =
      uninstall_multiple_protocol_interfaces;
  service->calculate_crc32 = calculate_crc32;
  service->uninstall_protocol_interface = uninstall_protocol_interface;
  service->load_image = load_image;
  service->locate_device_path = locate_device_path;
  service->install_configuration_table = install_configuration_table;
  service->exit_boot_services = exit_boot_services;
}
