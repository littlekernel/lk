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
#include "memory_protocols.h"
#include "blockio_protocols.h"

#include "arch/defines.h"
#include "boot_service.h"

#include "defer.h"
#include "lib/bio.h"
#include "libfdt.h"
#include "protocols/block_io_protocol.h"
#include "protocols/dt_fixup_protocol.h"
#include "protocols/gbl_efi_os_configuration_protocol.h"
#include "protocols/loaded_image_protocol.h"

#include "switch_stack.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

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
    const auto loaded_image = static_cast<EFI_LOADED_IMAGE_PROTOCOL *>(
        uefi_malloc(sizeof(EFI_LOADED_IMAGE_PROTOCOL)));
    *loaded_image = {};
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
  if (type == ALLOCATE_MAX_ADDRESS && *memory < 0xFFFFFFFF) {
    printf("allocate_pages(%d, %d, %zu, 0x%llx) unsupported\n", type,
           memory_type, pages, *memory);
    return UNSUPPORTED;
  }
  *memory = reinterpret_cast<EfiPhysicalAddr>(alloc_page(pages * PAGE_SIZE));
  if (*memory == 0) {
    return OUT_OF_RESOURCES;
  }
  return SUCCESS;
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

void copy_mem(void *dest, const void *src, size_t len) {
  memcpy(dest, src, len);
}
void set_mem(void *buf, size_t len, uint8_t val) { memset(buf, val, len); }

EfiTpl raise_tpl(EfiTpl new_tpl) {
  printf("%s is called %zu\n", __FUNCTION__, new_tpl);
  return APPLICATION;
}

EFI_STATUS efi_dt_fixup(struct EfiDtFixupProtocol *self, void *fdt,
                        size_t *buffer_size, uint32_t flags) {
  auto offset = fdt_subnode_offset(fdt, 0, "chosen");
  if (offset < 0) {
    printf("Failed to find chosen node %d\n", offset);
    return SUCCESS;
  }
  int length = 0;
  auto prop = fdt_get_property(fdt, offset, "bootargs", &length);

  if (prop == nullptr) {
    printf("Failed to find chosen/bootargs prop\n");
    return SUCCESS;
  }
  char *new_prop_data = reinterpret_cast<char *>(malloc(length));
  DEFER {
    free(new_prop_data);
    new_prop_data = nullptr;
  };
  auto prop_length = strnlen(prop->data, length);
  static constexpr auto &&to_add =
      "console=ttyAMA0 earlycon=pl011,mmio32,0x9000000 ";
  memset(new_prop_data, 0, length);
  memcpy(new_prop_data, to_add, sizeof(to_add) - 1);
  memcpy(new_prop_data + sizeof(to_add) - 1, prop->data, prop_length);
  auto ret = fdt_setprop(fdt, offset, "bootargs", new_prop_data, length);

  printf("chosen/bootargs: %d %d \"%s\"\n", ret, length, new_prop_data);

  return SUCCESS;
}

// Generates fixups for the kernel command line built by GBL.
EfiStatus fixup_kernel_commandline(struct GblEfiOsConfigurationProtocol *self,
                                   const char *command_line, char *fixup,
                                   size_t *fixup_buffer_size) {
  printf("%s(%p, \"%s\")\n", __FUNCTION__, self, command_line);
  *fixup_buffer_size = 0;
  return SUCCESS;
}

// Generates fixups for the bootconfig built by GBL.
EfiStatus fixup_bootconfig(struct GblEfiOsConfigurationProtocol *self,
                           const char *bootconfig, size_t size, char *fixup,
                           size_t *fixup_buffer_size) {
  printf("%s(%p, %s, %lu, %lu)\n", __FUNCTION__, self, bootconfig, size,
         *fixup_buffer_size);
  constexpr auto &&to_add = "\nandroidboot.fstab_suffix=cf.f2fs."
                            "hctr2\nandroidboot.boot_devices=4010000000.pcie";
  const auto final_len = sizeof(to_add);
  if (final_len > *fixup_buffer_size) {
    *fixup_buffer_size = final_len;
    return OUT_OF_RESOURCES;
  }
  *fixup_buffer_size = final_len;
  memcpy(fixup, to_add, final_len);

  return SUCCESS;
}

// Selects which device trees and overlays to use from those loaded by GBL.
EfiStatus select_device_trees(struct GblEfiOsConfigurationProtocol *self,
                              GblEfiVerifiedDeviceTree *device_trees,
                              size_t num_device_trees) {
  printf("%s(%p, %p %lu)\n", __FUNCTION__, self, device_trees,
         num_device_trees);
  return UNSUPPORTED;
}

EfiStatus open_protocol(EfiHandle handle, const EfiGuid *protocol, void **intf,
                        EfiHandle agent_handle, EfiHandle controller_handle,
                        EfiOpenProtocolAttributes attr) {
  if (guid_eq(protocol, LOADED_IMAGE_PROTOCOL_GUID)) {
    auto interface = reinterpret_cast<EfiLoadedImageProtocol *>(
        uefi_malloc(sizeof(EfiLoadedImageProtocol)));
    memset(interface, 0, sizeof(*interface));
    interface->parent_handle = handle;
    interface->image_base = handle;
    *intf = interface;
    printf("%s(LOADED_IMAGE_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
           "controller_handle=%p, attr=0x%x)\n",
           __FUNCTION__, handle, agent_handle, controller_handle, attr);
    return SUCCESS;
  } else if (guid_eq(protocol, EFI_DEVICE_PATH_PROTOCOL_GUID)) {
    printf(
        "%s(EFI_DEVICE_PATH_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
        "controller_handle=%p, attr=0x%x)\n",
        __FUNCTION__, handle, agent_handle, controller_handle, attr);
    return UNSUPPORTED;
  } else if (guid_eq(protocol, EFI_BLOCK_IO_PROTOCOL_GUID)) {
    printf("%s(EFI_BLOCK_IO_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
           "controller_handle=%p, attr=0x%x)\n",
           __FUNCTION__, handle, agent_handle, controller_handle, attr);
    return open_block_device(handle, intf);
  } else if (guid_eq(protocol, EFI_BLOCK_IO2_PROTOCOL_GUID)) {
    printf("%s(EFI_BLOCK_IO2_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
           "controller_handle=%p, attr=0x%x)\n",
           __FUNCTION__, handle, agent_handle, controller_handle, attr);
    return UNSUPPORTED;
  } else if (guid_eq(protocol, EFI_DT_FIXUP_PROTOCOL_GUID)) {
    printf("%s(EFI_DT_FIXUP_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
           "controller_handle=%p, attr=0x%x)\n",
           __FUNCTION__, handle, agent_handle, controller_handle, attr);
    if (intf != nullptr) {
      EfiDtFixupProtocol *fixup = nullptr;
      allocate_pool(BOOT_SERVICES_DATA, sizeof(EfiDtFixupProtocol),
                    reinterpret_cast<void **>(&fixup));
      if (fixup == nullptr) {
        return OUT_OF_RESOURCES;
      }
      fixup->revision = EFI_DT_FIXUP_PROTOCOL_REVISION;
      fixup->fixup = efi_dt_fixup;
      *intf = reinterpret_cast<EfiHandle *>(fixup);
    }
    return SUCCESS;
  } else if (guid_eq(protocol, EFI_GBL_OS_CONFIGURATION_PROTOCOL_GUID)) {
    printf("%s(EFI_GBL_OS_CONFIGURATION_PROTOCOL_GUID, handle=%p, "
           "agent_handle=%p, "
           "controller_handle=%p, attr=0x%x)\n",
           __FUNCTION__, handle, agent_handle, controller_handle, attr);
    GblEfiOsConfigurationProtocol *config = nullptr;
    allocate_pool(BOOT_SERVICES_DATA, sizeof(*config),
                  reinterpret_cast<void **>(&config));
    if (config == nullptr) {
      return OUT_OF_RESOURCES;
    }
    config->revision = GBL_EFI_OS_CONFIGURATION_PROTOCOL_REVISION;
    config->fixup_bootconfig = fixup_bootconfig;
    config->fixup_kernel_commandline = fixup_kernel_commandline;
    config->select_device_trees = select_device_trees;
    *intf = reinterpret_cast<EfiHandle *>(config);
    return SUCCESS;
  }
  printf("%s is unsupported 0x%x 0x%x 0x%x 0x%llx\n", __FUNCTION__,
         protocol->data1, protocol->data2, protocol->data3,
         *(uint64_t *)&protocol->data4);
  return UNSUPPORTED;
}

EfiStatus close_protocol(EfiHandle handle, const EfiGuid *protocol,
                         EfiHandle agent_handle, EfiHandle controller_handle) {
  if (guid_eq(protocol, LOADED_IMAGE_PROTOCOL_GUID)) {
    printf("%s(LOADED_IMAGE_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
           "controller_handle=%p)\n",
           __FUNCTION__, handle, agent_handle, controller_handle);
    return SUCCESS;
  } else if (guid_eq(protocol, EFI_DEVICE_PATH_PROTOCOL_GUID)) {
    printf(
        "%s(EFI_DEVICE_PATH_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
        "controller_handle=%p)\n",
        __FUNCTION__, handle, agent_handle, controller_handle);
    return SUCCESS;
  } else if (guid_eq(protocol, EFI_BLOCK_IO_PROTOCOL_GUID)) {
    printf("%s(EFI_BLOCK_IO_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
           "controller_handle=%p)\n",
           __FUNCTION__, handle, agent_handle, controller_handle);
    return SUCCESS;
  } else if (guid_eq(protocol, EFI_DT_FIXUP_PROTOCOL_GUID)) {
    printf("%s(EFI_DT_FIXUP_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
           "controller_handle=%p)\n",
           __FUNCTION__, handle, agent_handle, controller_handle);
    return SUCCESS;
  }
  printf("%s is called\n", __FUNCTION__);
  return UNSUPPORTED;
}

EfiStatus list_block_devices(size_t *num_handles, EfiHandle **buf) {
  size_t device_count = 0;
  bio_iter_devices([&device_count](bdev_t *dev) {
    device_count++;
    return true;
  });
  auto devices =
      reinterpret_cast<char **>(uefi_malloc(sizeof(char *) * device_count));
  size_t i = 0;
  bio_iter_devices([&i, devices, device_count](bdev_t *dev) {
    devices[i] = dev->name;
    i++;
    return i < device_count;
  });
  *num_handles = i;
  *buf = reinterpret_cast<EfiHandle *>(devices);
  return SUCCESS;
}

EfiStatus locate_handle_buffer(EfiLocateHandleSearchType search_type,
                               const EfiGuid *protocol, void *search_key,
                               size_t *num_handles, EfiHandle **buf) {
  if (guid_eq(protocol, EFI_BLOCK_IO_PROTOCOL_GUID)) {
    if (search_type == BY_PROTOCOL) {
      return list_block_devices(num_handles, buf);
    }
    printf("%s(0x%x, EFI_BLOCK_IO_PROTOCOL_GUID, search_key=%p)\n",
           __FUNCTION__, search_type, search_key);
    return UNSUPPORTED;
  } else if (guid_eq(protocol, EFI_TEXT_INPUT_PROTOCOL_GUID)) {
    printf("%s(0x%x, EFI_TEXT_INPUT_PROTOCOL_GUID, search_key=%p)\n",
           __FUNCTION__, search_type, search_key);
    return NOT_FOUND;
  } else if (guid_eq(protocol, EFI_GBL_OS_CONFIGURATION_PROTOCOL_GUID)) {
    printf(
        "%s(0x%x, EFI_GBL_OS_CONFIGURATION_PROTOCOL_GUID, search_key=%p)\n",
        __FUNCTION__, search_type, search_key);
    if (num_handles != nullptr) {
      *num_handles = 1;
    }
    if (buf != nullptr) {
      *buf = reinterpret_cast<EfiHandle *>(uefi_malloc(sizeof(buf)));
    }
    return SUCCESS;
  } else if (guid_eq(protocol, EFI_DT_FIXUP_PROTOCOL_GUID)) {
    printf("%s(0x%x, EFI_DT_FIXUP_PROTOCOL_GUID, search_key=%p)\n",
           __FUNCTION__, search_type, search_key);
    if (num_handles != nullptr) {
      *num_handles = 1;
    }
    if (buf != nullptr) {
      *buf = reinterpret_cast<EfiHandle *>(uefi_malloc(sizeof(buf)));
    }
    return SUCCESS;
  }
  printf("%s(0x%x, (0x%x 0x%x 0x%x 0x%llx), search_key=%p)\n", __FUNCTION__,
         search_type, protocol->data1, protocol->data2, protocol->data3,
         *(uint64_t *)&protocol->data4, search_key);
  return UNSUPPORTED;
}

EfiStatus wait_for_event(size_t num_events, EfiEvent *event, size_t *index) {
  return UNSUPPORTED;
}

} // namespace

void setup_boot_service_table(EfiBootService *service) {
  service->handle_protocol = handle_protocol;
  service->allocate_pool = allocate_pool;
  service->free_pool = free_pool;
  service->get_memory_map = get_physical_memory_map;
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
  service->copy_mem = copy_mem;
  service->set_mem = set_mem;
  service->open_protocol = open_protocol;
  service->locate_handle_buffer = locate_handle_buffer;
  service->close_protocol = close_protocol;
  service->wait_for_event = wait_for_event;
}
