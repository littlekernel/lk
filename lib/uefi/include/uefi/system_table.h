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
 * SPDX-License-Identifier: Apache-2.0 OR BSD-2-Clause-Patent
 *
 * You may choose to use or redistribute this file under
 *  (a) the Apache License, Version 2.0, or
 *  (b) the BSD 2-Clause Patent license.
 *
 * Unless you expressly elect the BSD-2-Clause-Patent terms, the Apache-2.0
 * terms apply by default.
 */

#ifndef __SYSTEM_TABLE_H__
#define __SYSTEM_TABLE_H__

#include "boot_service.h"
#include "protocols/simple_text_output_protocol.h"
#include "runtime_service.h"
#include "types.h"

typedef struct {
  EfiGuid vendor_guid;
  void* vendor_table;
} EfiConfigurationTable;

typedef struct EfiSystemTable {
  EfiTableHeader header;
  uint16_t* firmware_vendor;
  uint32_t firmware_revision;
  EfiHandle console_in_handle;
  void* con_in;
  EfiHandle console_out_handle;
  EfiSimpleTextOutputProtocol* con_out;
  EfiHandle standard_error_handle;
  EfiSimpleTextOutputProtocol* std_err;
  EfiRuntimeService* runtime_services;
  EfiBootService* boot_services;
  size_t number_of_table_entries;
  EfiConfigurationTable* configuration_table;
} EfiSystemTable;

#endif  // __SYSTEM_TABLE_H__
