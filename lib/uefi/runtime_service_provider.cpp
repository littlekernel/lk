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

#include "runtime_service_provider.h"

#include <platform.h>
#include <stdio.h>
#include <string.h>
#include <uefi/types.h>

#include "variable_mem.h"

namespace {

constexpr auto &&kSecureBoot = "SecureBoot";

EFI_STATUS GetVariable(char16_t *VariableName, EfiGuid *VendorGuid,
                       uint32_t *Attributes, size_t *DataSize, void *Data) {

  if (!VariableName || !VendorGuid || !DataSize) {
    return INVALID_PARAMETER;
  }

  char buffer[512];
  size_t i = 0;
  while (VariableName[i] && i < sizeof(buffer)) {
    size_t j = 0;
    for (j = 0; j < sizeof(buffer) - 1 && VariableName[i + j]; j++) {
      buffer[j] = VariableName[i + j];
    }
    i += j;
  }
  buffer[i] = 0;
  if (strncmp(buffer, kSecureBoot, sizeof(kSecureBoot)) == 0 || strcmp(buffer, "SetupMode") == 0) {
    if (DataSize) {
      *DataSize = 1;
    }
    if (Data) {
      memset(Data, 0, 1);
    }
    return SUCCESS;
  }

  char *data_in_mem;
  size_t data_in_mem_size;
  if (efi_get_variable(VariableName, VendorGuid, Attributes, &data_in_mem, &data_in_mem_size) == SUCCESS) {
    if (*DataSize == 0 && !Data) {
      *DataSize = data_in_mem_size;
      return BUFFER_TOO_SMALL;
    }
    if (data_in_mem_size > *DataSize) {
      return BUFFER_TOO_SMALL;
    }
    *DataSize = data_in_mem_size;
    memcpy(Data, data_in_mem, data_in_mem_size);
    return SUCCESS;
  }

  printf("%s(%s) is unsupported\n", __FUNCTION__, buffer);
  return UNSUPPORTED;
}

EFI_STATUS SetVirtualAddressMap(size_t MemoryMapSize, size_t DescriptorSize,
                                uint32_t DescriptorVersion,
                                EfiMemoryDescriptor *VirtualMap) {
  printf("%s is unsupported\n", __FUNCTION__);
  return UNSUPPORTED;
}

EFI_STATUS SetVariable(char16_t *VariableName, EfiGuid *VendorGuid,
                       uint32_t Attributes, size_t DataSize, void *Data) {
  if (!VariableName || VariableName[0] == 0) {
    return INVALID_PARAMETER;
  }

  /* Only allow setting non-volatile variables */
  if ((Attributes & EFI_VARIABLE_NON_VOLATILE) == 0) {
    efi_set_variable(VariableName,
                     VendorGuid,
                     Attributes,
                     reinterpret_cast<const char *>(Data),
                     DataSize);
    return SUCCESS;
  }

  printf("%s: Only non-volatile is supported. Attributes = 0x%x\n",
         __FUNCTION__,
         Attributes);
  return UNSUPPORTED;
}

void ResetSystem(EFI_RESET_TYPE ResetType, EFI_STATUS ResetStatus,
                        size_t DataSize, void *ResetData) {
  platform_halt(HALT_ACTION_REBOOT, HALT_REASON_SW_RESET);
}

}  // namespace

void setup_runtime_service_table(EfiRuntimeService *service) {
  service->GetVariable = GetVariable;
  service->SetVariable = SetVariable;
  service->SetVirtualAddressMap = SetVirtualAddressMap;
  service->ResetSystem = ResetSystem;
}
