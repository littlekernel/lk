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

#include <endian.h>
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

bool guid_eq(const EfiGuid *a, const EfiGuid *b) {
  if (a == nullptr || b == nullptr) {
    return false;
  }
  return memcmp(a, b, sizeof(*a)) == 0;
}

bool guid_eq(const EfiGuid *a, const EfiGuid &b) {
  return guid_eq(a, &b);
}

const char *protocol_name(const EfiGuid *protocol) {
  if (protocol == nullptr) {
    return nullptr;
  }

  struct ProtocolGuidName {
    const EfiGuid *guid;
    const char *name;
  };
  static constexpr ProtocolGuidName kProtocolGuidNames[] = {
      {&LOADED_IMAGE_PROTOCOL_GUID, "LOADED_IMAGE_PROTOCOL_GUID"},
      {&EFI_DEVICE_PATH_PROTOCOL_GUID, "EFI_DEVICE_PATH_PROTOCOL_GUID"},
      {&LINUX_EFI_LOADED_IMAGE_FIXED_GUID,
       "LINUX_EFI_LOADED_IMAGE_FIXED_GUID"},
      {&EFI_RNG_PROTOCOL_GUID, "EFI_RNG_PROTOCOL_GUID"},
      {&EFI_TCG2_PROTOCOL_GUID, "EFI_TCG2_PROTOCOL_GUID"},
      {&EFI_LOAD_FILE2_PROTOCOL_GUID, "EFI_LOAD_FILE2_PROTOCOL_GUID"},
      {&EFI_BLOCK_IO_PROTOCOL_GUID, "EFI_BLOCK_IO_PROTOCOL_GUID"},
      {&EFI_BLOCK_IO2_PROTOCOL_GUID, "EFI_BLOCK_IO2_PROTOCOL_GUID"},
      {&EFI_TEXT_INPUT_PROTOCOL_GUID, "EFI_TEXT_INPUT_PROTOCOL_GUID"},
      {&EFI_GBL_OS_CONFIGURATION_PROTOCOL_GUID,
       "EFI_GBL_OS_CONFIGURATION_PROTOCOL_GUID"},
      {&EFI_GBL_EFI_IMAGE_LOADING_PROTOCOL_GUID,
       "EFI_GBL_EFI_IMAGE_LOADING_PROTOCOL_GUID"},
      {&EFI_GBL_EFI_BOOT_CONTROL_PROTOCOL_GUID,
       "EFI_GBL_EFI_BOOT_CONTROL_PROTOCOL_GUID"},
      {&EFI_GBL_EFI_AVB_PROTOCOL_GUID, "EFI_GBL_EFI_AVB_PROTOCOL_GUID"},
      {&EFI_GBL_EFI_FASTBOOT_PROTOCOL_GUID,
       "EFI_GBL_EFI_FASTBOOT_PROTOCOL_GUID"},
      {&EFI_GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_GUID,
       "EFI_GBL_EFI_FASTBOOT_TRANSPORT_PROTOCOL_GUID"},
      {&EFI_DT_FIXUP_PROTOCOL_GUID, "EFI_DT_FIXUP_PROTOCOL_GUID"},
      {&EFI_TIMESTAMP_PROTOCOL_GUID, "EFI_TIMESTAMP_PROTOCOL_GUID"},
      {&EFI_BOOT_MEMORY_PROTOCOL_GUID, "EFI_BOOT_MEMORY_PROTOCOL_GUID"},
      {&EFI_ERASE_BLOCK_PROTOCOL_GUID, "EFI_ERASE_BLOCK_PROTOCOL_GUID"},
  };

  for (const auto &entry : kProtocolGuidNames) {
    if (guid_eq(protocol, entry.guid)) {
      return entry.name;
    }
  }

  return nullptr;
}

EfiStatus open_protocol(EfiHandle handle, const EfiGuid *protocol, const void **intf,
                        EfiHandle agent_handle, EfiHandle controller_handle,
                        EfiOpenProtocolAttributes attr);

EfiStatus locate_handle_buffer(EfiLocateHandleSearchType search_type,
                               const EfiGuid *protocol, const void *search_key,
                               size_t *num_handles, EfiHandle **buf);

uint64_t guid_data4(const EfiGuid *guid) {
  uint64_t data4 = 0;
  memcpy(&data4, guid->data4, sizeof(data4));
  return LE64(data4);
}

const char *fmt_guid(const EfiGuid *protocol, char *buf, size_t buf_size) {
  const char *name = protocol_name(protocol);
  if (name != nullptr) {
    return name;
  }
  if (protocol == nullptr) {
    return "nullptr";
  }
  snprintf(buf, buf_size, "(0x%x 0x%x 0x%x 0x%llx)", protocol->data1,
           protocol->data2, protocol->data3,
           static_cast<unsigned long long>(guid_data4(protocol)));
  return buf;
}

EfiStatus unload(EfiHandle handle) { return EFI_STATUS_SUCCESS; }

EfiHandle singleton_protocol_handle(const EfiGuid *protocol) {
  static constexpr const EfiGuid *kSingletonGuids[] = {
      &EFI_GBL_OS_CONFIGURATION_PROTOCOL_GUID,
      &EFI_DT_FIXUP_PROTOCOL_GUID,
      &EFI_TIMESTAMP_PROTOCOL_GUID,
      &EFI_BOOT_MEMORY_PROTOCOL_GUID,
      &EFI_GBL_EFI_AVB_PROTOCOL_GUID,
      &EFI_GBL_EFI_BOOT_CONTROL_PROTOCOL_GUID,
      &EFI_RNG_PROTOCOL_GUID,
  };

  for (const EfiGuid *singleton_guid : kSingletonGuids) {
    if (guid_eq(protocol, singleton_guid)) {
      return singleton_guid;
    }
  }
  return nullptr;
}

EfiStatus handle_protocol(EfiHandle handle, const EfiGuid *protocol,
                          void **intf) {
  char name_buf[64];
  printf("%s(%p, %s, %p);\n", __FUNCTION__, handle,
         fmt_guid(protocol, name_buf, sizeof(name_buf)), intf);

  if (guid_eq(protocol, LOADED_IMAGE_PROTOCOL_GUID)) {
    const auto loaded_image = static_cast<EfiLoadedImageProtocol *>(
        uefi_malloc(sizeof(EfiLoadedImageProtocol)));
    if (!loaded_image) {
      return EFI_STATUS_OUT_OF_RESOURCES;
    }
    *loaded_image = {};
    loaded_image->revision = EFI_LOADED_IMAGE_PROTOCOL_REVISION;
    loaded_image->parent_handle = nullptr;
    loaded_image->system_table = nullptr;
    loaded_image->load_options_size = 0;
    loaded_image->load_options = nullptr;
    loaded_image->unload = unload;
    loaded_image->image_base = handle;

    *intf = loaded_image;
    return EFI_STATUS_SUCCESS;
  } else if (guid_eq(protocol, LINUX_EFI_LOADED_IMAGE_FIXED_GUID)) {
    return EFI_STATUS_UNSUPPORTED;
  }
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus register_protocol_notify(const EfiGuid *protocol, EfiEvent event,
                                   void **registration) {
  char name_buf[64];
  printf("%s(%s, event=%p, registration=%p) is unsupported\n", __FUNCTION__,
         fmt_guid(protocol, name_buf, sizeof(name_buf)), event, registration);
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus locate_handle(EfiLocateHandleSearchType search_type,
                        const EfiGuid *protocol, const void *search_key,
                        size_t *buf_size, EfiHandle *buf) {
  if (buf_size == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }

  if (guid_eq(protocol, EFI_BLOCK_IO_PROTOCOL_GUID)) {
    if (search_type == EFI_LOCATE_HANDLE_SEARCH_TYPE_BY_PROTOCOL) {
      size_t num_handles = *buf_size / sizeof(EfiHandle);
      const EfiStatus status = list_block_devices(&num_handles, buf);
      *buf_size = num_handles * sizeof(EfiHandle);
      return status;
    }
    char name_buf[64];
    printf("%s(0x%x, %s, search_key=%p)\n", __FUNCTION__, search_type,
           fmt_guid(protocol, name_buf, sizeof(name_buf)), search_key);
    return EFI_STATUS_UNSUPPORTED;
  }

  const EfiHandle singleton_handle = singleton_protocol_handle(protocol);
  char name_buf[64];
  printf("%s(0x%x, %s, search_key=%p)\n", __FUNCTION__, search_type,
         fmt_guid(protocol, name_buf, sizeof(name_buf)), search_key);

  if (singleton_handle != nullptr) {
    const size_t required_size = sizeof(EfiHandle);
    if (*buf_size < required_size) {
      *buf_size = required_size;
      return EFI_STATUS_BUFFER_TOO_SMALL;
    }

    *buf_size = required_size;
    if (buf == nullptr) {
      return EFI_STATUS_INVALID_PARAMETER;
    }
    *buf = singleton_handle;
    return EFI_STATUS_SUCCESS;
  }

  return EFI_STATUS_NOT_FOUND;
}

EfiStatus locate_protocol(const EfiGuid *protocol, void *registration,
                          void **intf) {
  if (protocol == nullptr || intf == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }

  const EfiHandle singleton_handle = singleton_protocol_handle(protocol);
  if (singleton_handle != nullptr) {
    return open_protocol(singleton_handle, protocol, const_cast<const void **>(intf),
                         nullptr, nullptr, 0);
  }

  char name_buf[64];
  const char *name = fmt_guid(protocol, name_buf, sizeof(name_buf));

  if (guid_eq(protocol, EFI_RNG_PROTOCOL_GUID)) {
    printf("%s(%s) is unsupported.\n", __FUNCTION__, name);
    return EFI_STATUS_UNSUPPORTED;
  }
  if (guid_eq(protocol, EFI_TCG2_PROTOCOL_GUID)) {
    printf("%s(%s) is unsupported.\n", __FUNCTION__, name);
    return EFI_STATUS_NOT_FOUND;
  }

  printf("%s(%s) is unsupported\n", __FUNCTION__, name);
  return EFI_STATUS_NOT_FOUND;
}

EfiStatus uninstall_multiple_protocol_interfaces(EfiHandle handle, ...) {
  printf("%s is unsupported\n", __FUNCTION__);
  return EFI_STATUS_UNSUPPORTED;
}
EfiStatus calculate_crc32(void *data, size_t len, uint32_t *crc32) {
  printf("%s is unsupported\n", __FUNCTION__);
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus uninstall_protocol_interface(EfiHandle handle,
                                       const EfiGuid *protocol, void *intf) {
  char name_buf[64];
  printf("%s(%s, handle=%p, intf=%p) is unsupported\n", __FUNCTION__,
         fmt_guid(protocol, name_buf, sizeof(name_buf)), handle, intf);
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus load_image(bool boot_policy, EfiHandle parent_image_handle,
                     EfiDevicePathProtocol *path, void *src, size_t src_size,
                     EfiHandle *image_handle) {
  printf("%s is unsupported\n", __FUNCTION__);
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus locate_device_path(const EfiGuid *protocol,
                             EfiDevicePathProtocol **path, EfiHandle *device) {
  if (guid_eq(protocol, EFI_LOAD_FILE2_PROTOCOL_GUID)) {
    return EFI_STATUS_NOT_FOUND;
  }
  char name_buf[64];
  printf("%s(%s) is unsupported\n", __FUNCTION__,
         fmt_guid(protocol, name_buf, sizeof(name_buf)));
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus install_configuration_table(const EfiGuid *guid, void *table) {
  printf("%s is unsupported\n", __FUNCTION__);
  return EFI_STATUS_UNSUPPORTED;
}

void copy_mem(void *dest, const void *src, size_t len) {
  memcpy(dest, src, len);
}
void set_mem(void *buf, size_t len, uint8_t val) { memset(buf, val, len); }

EfiTpl raise_tpl(EfiTpl new_tpl) {
  printf("%s is called %zu\n", __FUNCTION__, new_tpl);
  return EFI_TPL_APPLICATION;
}

void restore_tpl(EfiTpl old_tpl) {
  printf("%s is called %zu\n", __FUNCTION__, old_tpl);
}

EfiStatus open_protocol(EfiHandle handle, const EfiGuid *protocol, const void **intf,
                        EfiHandle agent_handle, EfiHandle controller_handle,
                        EfiOpenProtocolAttributes attr) {
  char name_buf[64];
  printf("%s(%s, handle=%p, agent_handle=%p, controller_handle=%p, "
         "attr=0x%x)\n",
         __FUNCTION__, fmt_guid(protocol, name_buf, sizeof(name_buf)), handle,
         agent_handle, controller_handle, attr);

  if (guid_eq(protocol, LOADED_IMAGE_PROTOCOL_GUID)) {
    auto interface = reinterpret_cast<EfiLoadedImageProtocol *>(
        uefi_malloc(sizeof(EfiLoadedImageProtocol)));
    if (interface == nullptr) {
      return EFI_STATUS_OUT_OF_RESOURCES;
    }
    memset(interface, 0, sizeof(*interface));
    interface->parent_handle = handle;
    interface->image_base = handle;
    *intf = interface;
    return EFI_STATUS_SUCCESS;
  } else if (guid_eq(protocol, EFI_DEVICE_PATH_PROTOCOL_GUID)) {
    return EFI_STATUS_UNSUPPORTED;
  } else if (guid_eq(protocol, EFI_BLOCK_IO_PROTOCOL_GUID)) {
    return open_block_device(handle, intf);
  } else if (guid_eq(protocol, EFI_BLOCK_IO2_PROTOCOL_GUID)) {
    return open_async_block_device(handle, intf);
  } else if (guid_eq(protocol, EFI_DT_FIXUP_PROTOCOL_GUID)) {
    if (intf != nullptr) {
      EfiDtFixupProtocol *fixup = nullptr;
      allocate_pool(EFI_MEMORY_TYPE_BOOT_SERVICES_DATA, sizeof(EfiDtFixupProtocol),
                    reinterpret_cast<void **>(&fixup));
      if (fixup == nullptr) {
        return EFI_STATUS_OUT_OF_RESOURCES;
      }
      fixup->revision = EFI_DT_FIXUP_PROTOCOL_REVISION;
      fixup->fixup = efi_dt_fixup;
      *intf = reinterpret_cast<void *>(fixup);
    }
    return EFI_STATUS_SUCCESS;
  } else if (guid_eq(protocol, EFI_GBL_OS_CONFIGURATION_PROTOCOL_GUID)) {
    GblEfiOsConfigurationProtocol *config = nullptr;
    allocate_pool(EFI_MEMORY_TYPE_BOOT_SERVICES_DATA, sizeof(*config),
                  reinterpret_cast<void **>(&config));
    if (config == nullptr) {
      return EFI_STATUS_OUT_OF_RESOURCES;
    }
    config->revision = GBL_EFI_OS_CONFIGURATION_PROTOCOL_REVISION;
    config->fixup_bootconfig = fixup_bootconfig;
    config->select_device_trees = select_device_trees;
    config->select_fit_configuration = select_fit_configuration;
    *intf = reinterpret_cast<void *>(config);
    return EFI_STATUS_SUCCESS;
  } else if (guid_eq(protocol, EFI_GBL_EFI_IMAGE_LOADING_PROTOCOL_GUID)) {
    return EFI_STATUS_UNSUPPORTED;
  } else if (guid_eq(protocol, EFI_TIMESTAMP_PROTOCOL_GUID)) {
    EfiTimestampProtocol *ts = reinterpret_cast<EfiTimestampProtocol *>(
        uefi_malloc(sizeof(EfiTimestampProtocol)));
    if (ts == nullptr) {
      return EFI_STATUS_OUT_OF_RESOURCES;
    }
    ts->get_timestamp = get_timestamp;
    ts->get_properties = get_timestamp_properties;
    *intf = reinterpret_cast<void *>(ts);
    return EFI_STATUS_SUCCESS;
  } else if (guid_eq(protocol, EFI_ERASE_BLOCK_PROTOCOL_GUID)) {
    return open_efi_erase_block_protocol(handle, intf);
  } else if (guid_eq(protocol, EFI_BOOT_MEMORY_PROTOCOL_GUID)) {
    *intf = open_boot_memory_protocol();
    if (*intf == nullptr) {
      return EFI_STATUS_OUT_OF_RESOURCES;
    }
    return EFI_STATUS_SUCCESS;
  } else if (guid_eq(protocol, EFI_GBL_EFI_AVB_PROTOCOL_GUID)) {
    *intf = open_gbl_efi_avb_protocol();
    if (*intf == nullptr) {
      return EFI_STATUS_OUT_OF_RESOURCES;
    }
    return EFI_STATUS_SUCCESS;
  } else if (guid_eq(protocol, EFI_GBL_EFI_BOOT_CONTROL_PROTOCOL_GUID)) {
    *intf = open_gbl_efi_boot_control_protocol();
    if (*intf == nullptr) {
      return EFI_STATUS_OUT_OF_RESOURCES;
    }
    return EFI_STATUS_SUCCESS;
  } else if (guid_eq(protocol, EFI_RNG_PROTOCOL_GUID)) {
    *intf = open_efi_rng_protocol();
    if (*intf == nullptr) {
      return EFI_STATUS_OUT_OF_RESOURCES;
    }
    return EFI_STATUS_SUCCESS;
  }
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus close_protocol(EfiHandle handle, const EfiGuid *protocol,
                         EfiHandle agent_handle, EfiHandle controller_handle) {
  char name_buf[64];
  printf("%s(%s, handle=%p, agent_handle=%p, controller_handle=%p)\n",
         __FUNCTION__, fmt_guid(protocol, name_buf, sizeof(name_buf)), handle,
         agent_handle, controller_handle);

  if (guid_eq(protocol, LOADED_IMAGE_PROTOCOL_GUID)) {
    return EFI_STATUS_SUCCESS;
  } else if (guid_eq(protocol, EFI_DEVICE_PATH_PROTOCOL_GUID)) {
    return EFI_STATUS_SUCCESS;
  } else if (guid_eq(protocol, EFI_BLOCK_IO_PROTOCOL_GUID)) {
    return EFI_STATUS_SUCCESS;
  } else if (guid_eq(protocol, EFI_DT_FIXUP_PROTOCOL_GUID)) {
    return EFI_STATUS_SUCCESS;
  }
  return EFI_STATUS_UNSUPPORTED;
}

EfiStatus locate_handle_buffer(EfiLocateHandleSearchType search_type,
                               const EfiGuid *protocol, const void *search_key,
                               size_t *num_handles, EfiHandle **buf) {
  if (num_handles == nullptr || buf == nullptr) {
    return EFI_STATUS_INVALID_PARAMETER;
  }
  *num_handles = 0;
  *buf = nullptr;

  size_t buf_size = 0;
  EfiStatus status =
      locate_handle(search_type, protocol, search_key, &buf_size, nullptr);
  if (status != EFI_STATUS_BUFFER_TOO_SMALL) {
    return status;
  }

  status = allocate_pool(EFI_MEMORY_TYPE_BOOT_SERVICES_DATA, buf_size,
                         reinterpret_cast<void **>(buf));
  if (status != EFI_STATUS_SUCCESS) {
    return status;
  }

  status = locate_handle(search_type, protocol, search_key, &buf_size, *buf);
  if (status != EFI_STATUS_SUCCESS) {
    free_pool(*buf);
    *buf = nullptr;
    return status;
  }

  *num_handles = buf_size / sizeof(EfiHandle);
  if (*num_handles == 0) {
    free_pool(*buf);
    *buf = nullptr;
    return EFI_STATUS_NOT_FOUND;
  }

  return EFI_STATUS_SUCCESS;
}

EfiStatus stall(size_t microseconds) {
  uint64_t end_microseconds;

  end_microseconds = current_time_hires() + microseconds;
  while (current_time_hires() < end_microseconds) {
    thread_yield();
  }

  return EFI_STATUS_SUCCESS;
}

EfiStatus free_pages(EfiPhysicalAddr memory, size_t pages) {
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
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
  service->raise_tpl = raise_tpl;
  service->restore_tpl = restore_tpl;
  service->set_watchdog_timer = set_watchdog_timer;
}
