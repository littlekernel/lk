/*
 * Copyright (C) 2023 The Android Open Source Project
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

#ifndef __BOOT_SERVICE_TABLE_H__
#define __BOOT_SERVICE_TABLE_H__

#include "protocols/device_path_protocol.h"
#include "types.h"

typedef enum EFI_LOCATE_HANDLE_SEARCH_TYPE {
  ALL_HANDLES = 0,
  BY_REGITER_NOTIFY,
  BY_PROTOCOL,
} EFI_LOCATE_HANDLE_SEARCH_TYPE;

typedef EFI_LOCATE_HANDLE_SEARCH_TYPE EfiLocateHandleSearchType;

typedef enum { EFI_NATIVE_INTERFACE } EFI_INTERFACE_TYPE;

typedef EFI_INTERFACE_TYPE EfiInterfaceType;

typedef enum EFI_ALLOCATOR_TYPE {
  ALLOCATE_ANY_PAGES,
  ALLOCATE_MAX_ADDRESS,
  ALLOCATE_ADDRESS,
  MAX_ALLOCATE_TYPE
} EfiAllocatorType;

typedef enum EFI_OPEN_PROTOCOL_ATTRIBUTE : uint32_t {
  BY_HANDLE_PROTOCOL = 0x00000001,
  GET_PROTOCOL = 0x00000002,
  TEST_PROTOCOL = 0x00000004,
  BY_CHILD_CONTROLLER = 0x00000008,
  BY_DRIVER = 0x00000010,
  EXCLUSIVE = 0x00000020,
} EfiOpenProtocolAttributes;

typedef struct {
  uint32_t memory_type;
  uint32_t padding;
  EfiPhysicalAddr physical_start;
  EfiVirtualAddr virtual_start;
  uint64_t number_of_pages;
  uint64_t attributes;
} EfiMemoryDescriptor;

typedef struct {
  EfiHandle agent_handle;
  EfiHandle controller_handle;
  uint32_t attributes;
  uint32_t open_count;
} EfiOpenProtocolInformationEntry;

typedef struct {
  EfiTableHeader hdr;
  EfiTpl (*raise_tpl)(EfiTpl new_tpl);
  void (*restore_tpl)(EfiTpl old_tpl);
  EfiStatus (*allocate_pages)(EfiAllocatorType type, EfiMemoryType memory_type, size_t pages,
                              EfiPhysicalAddr* memory);
  EfiStatus (*free_pages)(EfiPhysicalAddr memory, size_t pages);
  EfiStatus (*get_memory_map)(size_t* memory_map_size, EfiMemoryDescriptor* memory_map,
                              size_t* map_key, size_t* desc_size, uint32_t* desc_version);
  EfiStatus (*allocate_pool)(EfiMemoryType pool_type, size_t size, void** buf);
  EfiStatus (*free_pool)(void* buf);
  EfiStatus (*create_event)(uint32_t type, EfiTpl notify_tpl, EfiEventNotify notify_fn,
                            void* notify_ctx, EfiEvent* event);
  EfiStatus (*set_timer)(EfiEvent event, EfiTimerDelay type, uint64_t trigger_time);
  EfiStatus (*wait_for_event)(size_t num_events, EfiEvent* event, size_t* index);
  EfiStatus (*signal_event)(EfiEvent event);
  EfiStatus (*close_event)(EfiEvent event);
  EfiStatus (*check_event)(EfiEvent event);
  EfiStatus (*install_protocol_interface)(EfiHandle* handle, const EfiGuid* protocol,
                                          EfiInterfaceType intf_type, void* intf);
  EfiStatus (*reinstall_protocol_interface)(EfiHandle hadle, const EfiGuid* protocol,
                                            void* old_intf, void* new_intf);
  EfiStatus (*uninstall_protocol_interface)(EfiHandle handle, const EfiGuid* protocol, void* intf);
  EfiStatus (*handle_protocol)(EfiHandle handle, const EfiGuid* protocol, void** intf);
  void* reserved;
  EfiStatus (*register_protocol_notify)(const EfiGuid* protocol, EfiEvent event,
                                        void** registration);
  EfiStatus (*locate_handle)(EfiLocateHandleSearchType search_type, const EfiGuid* protocol,
                             void* search_key, size_t* buf_size, EfiHandle* buf);
  EfiStatus (*locate_device_path)(const EfiGuid* protocol, EfiDevicePathProtocol** path,
                                  EfiHandle* device);
  EfiStatus (*install_configuration_table)(const EfiGuid* guid, void* table);
  EfiStatus (*load_image)(bool boot_policy, EfiHandle parent_image_handle,
                          EfiDevicePathProtocol* path, void* src, size_t src_size,
                          EfiHandle* image_handle);
  EfiStatus (*start_image)(EfiHandle image_handle, size_t* exit_data_size, char16_t** exit_data);
  EfiStatus (*exit)(EfiHandle image_handle, EfiStatus exit_status, size_t exit_data_size,
                    char16_t* exit_data);
  EfiStatus (*unload_image)(EfiHandle image_handle);
  EfiStatus (*exit_boot_services)(EfiHandle image_handle, size_t map_key);
  EfiStatus (*get_next_monotonic_count)(uint64_t* count);
  EfiStatus (*stall)(size_t microseconds);
  EfiStatus (*set_watchdog_timer)(size_t timeout, uint64_t watchdog_code, size_t data_size,
                                  char16_t* watchdog_data);
  EfiStatus (*connect_controller)(EfiHandle controller_handle, EfiHandle* driver_image_handle,
                                  EfiDevicePathProtocol* remaining_path, bool recursive);
  EfiStatus (*disconnect_controller)(EfiHandle controller_handle, EfiHandle driver_image_handle,
                                     EfiHandle child_handle);
  EfiStatus (*open_protocol)(EfiHandle handle, const EfiGuid* protocol, void** intf,
                             EfiHandle agent_handle, EfiHandle controller_handle,
                             EfiOpenProtocolAttributes attr);
  EfiStatus (*close_protocol)(EfiHandle handle, const EfiGuid* protocol, EfiHandle agent_handle,
                              EfiHandle controller_handle);
  EfiStatus (*open_protocol_information)(EfiHandle handle, const EfiGuid* protocol,
                                         EfiOpenProtocolInformationEntry** entry_buf,
                                         size_t* entry_count);
  EfiStatus (*protocols_per_handle)(EfiHandle handle, EfiGuid*** protocol_buf,
                                    size_t* protocol_buf_count);
  EfiStatus (*locate_handle_buffer)(EfiLocateHandleSearchType search_type, const EfiGuid* protocol,
                                    void* search_key, size_t* num_handles, EfiHandle** buf);
  EfiStatus (*locate_protocol)(const EfiGuid* protocol, void* registration, void** intf);
  EfiStatus (*install_multiple_protocol_interfaces)(EfiHandle* handle, ...);
  EfiStatus (*uninstall_multiple_protocol_interfaces)(EfiHandle handle, ...);
  EfiStatus (*calculate_crc32)(void* data, size_t len, uint32_t* crc32);
  void (*copy_mem)(void* dest, const void* src, size_t len);
  void (*set_mem)(void* buf, size_t len, uint8_t val);
  EfiStatus (*create_event_ex)(EfiEventType type, EfiTpl notify_tpl, EfiEventNotify notify_fn,
                               const void* notify_ctx, const EfiGuid* event_group, EfiEvent* event);
} EfiBootService;

#endif  // __BOOT_SERVICE_TABLE_H__