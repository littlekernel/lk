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

#include "variable_mem.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <lk/list.h>
#include <uefi/types.h>
#include "charset.h"

namespace {
  constexpr size_t kVarNameMax = 128;

  struct EfiVariable {
    struct list_node node;
    char16_t VariableName[kVarNameMax];
    EfiGuid VendorGuid;
    uint32_t Attributes;
    char* Data;
    size_t DataLen;
  };
}

static list_node variables_in_mem = LIST_INITIAL_VALUE(variables_in_mem);

static EfiVariable * search_existing_variable(const char16_t *varname,
                                              const EfiGuid *guid) {
  /* search for existing variable */
  struct EfiVariable *var = nullptr;
  list_for_every_entry(&variables_in_mem, var, struct EfiVariable, node) {
    if (guid != NULL && memcmp(&var->VendorGuid, guid, sizeof(EfiGuid)) != 0) {
      continue;
    }
    if (utf16_strcmp(varname, var->VariableName) != 0) {
      continue;
    }
    return var;
  }
  return nullptr;
}

EfiStatus efi_get_variable(const char16_t *variable_name,
                           const EfiGuid *guid,
                           uint32_t *attribute,
                           char **data,
                           size_t *data_size) {
  struct EfiVariable *var = search_existing_variable(variable_name, guid);

  if (data)
    *data = nullptr;
  if (data_size)
    *data_size = 0;
  if (attribute)
    *attribute = 0;
  if (var) {
    if (data)
      *data = var->Data;
    if (data_size)
      *data_size = var->DataLen;
    if (attribute)
      *attribute = var->Attributes;
    return SUCCESS;
  }
  return NOT_FOUND;
}

void efi_set_variable(const char16_t *variable_name,
                      const EfiGuid *guid,
                      uint32_t attribute,
                      const char *data,
                      size_t data_len) {
  struct EfiVariable *var = search_existing_variable(variable_name, guid);

  /* alloc new variable if it is not existed */
  if (!var) {
    var = reinterpret_cast<struct EfiVariable *>(malloc(sizeof(struct EfiVariable)));
    memset(var, 0, sizeof(struct EfiVariable));
    var->node = LIST_INITIAL_CLEARED_VALUE;
    size_t name_len = utf16_strlen(variable_name);
    if (name_len >= kVarNameMax)
	name_len = kVarNameMax - 1;
    memcpy(var->VariableName, variable_name, name_len * sizeof(char16_t));
    list_add_tail(&variables_in_mem, &var->node);
  }
  if (var->Data) {
    free(var->Data);
  }
  var->Data = nullptr;
  var->DataLen = 0;
  if (data_len > 0) {
    var->Data = reinterpret_cast<char *>(malloc(data_len));
    memcpy(var->Data, data, data_len);
    var->DataLen = data_len;
  }
  var->Attributes = attribute;
  if (guid) {
    memcpy(&var->VendorGuid, guid, sizeof(EfiGuid));
  }
}

void efi_list_variable(void) {
  struct EfiVariable *var = nullptr;
  list_for_every_entry(&variables_in_mem, var, struct EfiVariable, node) {
    char varname[kVarNameMax];

    utf16_to_utf8(varname, var->VariableName, sizeof(varname));
    printf("%s\n", varname);
    /* print GUID */
    printf("    %08x-%04x-%04x",
           var->VendorGuid.data1,
           var->VendorGuid.data2,
           var->VendorGuid.data3);
    for (int i = 0; i < 8 ; i++) {
      if (i == 0 || i == 2) {
        printf("-");
      }
      printf("%02x", var->VendorGuid.data4[i]);
    }
    printf("\n");
    printf("    ");
    /* print attributes */
    for (uint32_t attr = 1; attr <= var->Attributes; attr <<= 1) {
      if (var->Attributes & attr) {
        if (attr == EFI_VARIABLE_NON_VOLATILE) {
          printf("NV");
        } else if (attr == EFI_VARIABLE_BOOTSERVICE_ACCESS) {
          printf("BS");
        } else if (attr == EFI_VARIABLE_RUNTIME_ACCESS) {
          printf("RT");
        } else {
          printf("0x%02x", attr);
        }
        if (attr << 1 <= var->Attributes) {
          printf("|");
        }
      }
    }
    printf(", DataSize = 0x%zx\n", var->DataLen);
    /* dump data */
    for (size_t i = 0; i < var->DataLen ; i+=16) {
      printf("    %08zx:", i);
      for (size_t j = 0; j < 16; j++) {
        if (i + j < var->DataLen) {
          printf(" %02x", var->Data[i + j]);
        } else {
          printf("   ");
        }
      }
      printf("  ");
      for (size_t j = 0; j < 16 && i + j < var->DataLen; j++) {
        if (var->Data[i + j] >= 0x20 && var->Data[i + j] < 0x7e) {
          printf("%c", var->Data[i + j]);
        } else {
          printf(".");
        }
      }
      printf("\n");
    }
  }
}
