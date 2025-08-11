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

#include <lk/compiler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <platform.h>
#include <sys/types.h>
#include <uefi/boot_service.h>
#include <uefi/protocols/block_io_protocol.h>
#include <uefi/protocols/dt_fixup_protocol.h>
#include <uefi/protocols/gbl_efi_image_loading_protocol.h>
#include <uefi/protocols/gbl_efi_os_configuration_protocol.h>
#include <uefi/protocols/loaded_image_protocol.h>
#include <uefi/types.h>

#include "blockio2_protocols.h"
#include "blockio_protocols.h"
#include "events.h"
#include "io_stack.h"
#include "memory_protocols.h"
#include "uefi_platform.h"

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

void copy_mem(void *dest, const void *src, size_t len) {
  memcpy(dest, src, len);
}
void set_mem(void *buf, size_t len, uint8_t val) { memset(buf, val, len); }

EfiTpl raise_tpl(EfiTpl new_tpl) {
  printf("%s is called %zu\n", __FUNCTION__, new_tpl);
  return APPLICATION;
}

EfiStatus get_verify_partitions(struct GblEfiImageLoadingProtocol *self,
                                size_t *NumberOfPartitions,
                                GblEfiPartitionName *Partitions) {
  printf("%s is called\n", __FUNCTION__);
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
    printf("%s(EFI_DEVICE_PATH_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
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
    return open_async_block_device(handle, intf);
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
      *intf = reinterpret_cast<void *>(fixup);
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
    config->select_device_trees = select_device_trees;
    *intf = reinterpret_cast<void *>(config);
    return SUCCESS;
  } else if (guid_eq(protocol, EFI_GBL_EFI_IMAGE_LOADING_PROTOCOL_GUID)) {
    printf(
        "%s(EFI_GBL_EFI_IMAGE_LOADING_PROTOCOL_GUID, handle=%p, "
        "agent_handle%p, controller_handle=%p, attr=0x%x)\n",
        __FUNCTION__, handle, agent_handle, controller_handle, attr);
    GblEfiImageLoadingProtocol *image_loading = nullptr;
    allocate_pool(BOOT_SERVICES_DATA, sizeof(*image_loading),
                  reinterpret_cast<void **>(&image_loading));
    if (image_loading == nullptr) {
      return OUT_OF_RESOURCES;
    }
    image_loading->revision = GBL_EFI_IMAGE_LOADING_PROTOCOL_REVISION;
    image_loading->get_buffer = get_buffer;
    image_loading->get_verify_partitions = get_verify_partitions;
    *intf = reinterpret_cast<void *>(image_loading);
    return SUCCESS;
  } else if (guid_eq(protocol, EFI_TIMESTAMP_PROTOCOL_GUID)) {
    printf("%s(EFI_TIMESTAMP_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
           "controller_handle=%p, attr=0x%x)\n",
           __FUNCTION__, handle, agent_handle, controller_handle, attr);
    EfiTimestampProtocol *ts = reinterpret_cast<EfiTimestampProtocol *>(
        uefi_malloc(sizeof(EfiTimestampProtocol)));
    if (ts == nullptr) {
      return OUT_OF_RESOURCES;
    }
    ts->get_timestamp = get_timestamp;
    ts->get_properties = get_timestamp_properties;
    *intf = reinterpret_cast<void *>(ts);
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
    printf("%s(EFI_DEVICE_PATH_PROTOCOL_GUID, handle=%p, agent_handle=%p, "
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
    printf("%s(0x%x, EFI_GBL_OS_CONFIGURATION_PROTOCOL_GUID, search_key=%p)\n",
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
  } else if (guid_eq(protocol, EFI_TIMESTAMP_PROTOCOL_GUID)) {
    printf("%s(0x%x, EFI_TIMESTAMP_PROTOCOL_GUID, search_key=%p)\n",
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

EfiStatus stall(size_t microseconds) {
  uint64_t end_microseconds;

  end_microseconds = current_time_hires() + microseconds;
  while (current_time_hires() < end_microseconds) {
    thread_yield();
  }

  return SUCCESS;
}

EfiStatus free_pages(EfiPhysicalAddr memory, size_t pages) {
  return ::free_pages(reinterpret_cast<void *>(memory), pages);
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
  service->wait_for_event =
      switch_stack_wrapper<size_t, EfiEvent *, size_t *, wait_for_event>();
  service->signal_event = switch_stack_wrapper<EfiEvent, signal_event>();
  service->check_event = switch_stack_wrapper<EfiEvent, check_event>();
  service->create_event = create_event;
  service->close_event = close_event;
  service->stall = stall;
}
