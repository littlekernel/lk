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

#ifndef __SYSTEM_TABLE_H__
#define __SYSTEM_TABLE_H__

#include "types.h"

#include "boot_service.h"
#include "protocols/simple_text_output_protocol.h"

typedef struct {
  EfiGuid vendor_guid;
  const void* vendor_table;
} EfiConfigurationTable;

typedef struct EfiSystemTable {
  EfiTableHeader header;
  char16_t* firmware_vendor;
  uint32_t firmware_revision;
  EfiHandle console_in_handle;
  void* con_in;
  EfiHandle console_out_handle;
  EfiSimpleTextOutputProtocol* con_out;
  EfiHandle standard_error_handle;
  EfiSimpleTextOutputProtocol* std_err;
  void* runtime_service;
  EfiBootService* boot_services;
  size_t number_of_table_entries;
  const EfiConfigurationTable* configuration_table;
} EfiSystemTable;

#endif  // __SYSTEM_TABLE_H__
