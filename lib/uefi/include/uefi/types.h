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

#ifndef __EFI_TYPES_H__
#define __EFI_TYPES_H__

#include <stddef.h>
#include <stdint.h>

#include "gbl_protocol_utils.h"

struct EfiEventImpl;

typedef void* EfiHandle;
typedef EfiEventImpl* EfiEvent;
typedef uint64_t EfiPhysicalAddr;
typedef uint64_t EfiVirtualAddr;

typedef struct EfiTableHeader {
  uint64_t signature;
  uint32_t revision;
  uint32_t header_size;
  uint32_t crc32;
  uint32_t reserved;
} EfiTableHeader;

typedef struct EfiGuid {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  uint8_t data4[8];
} EfiGuid;

typedef struct EfiMemoryAttributesTableHeader {
  uint32_t version;
  uint32_t number_of_entries;
  uint32_t descriptor_size;
  uint32_t reserved;
} EfiMemoryAttributesTableHeader;

EFI_ENUM(
    EfiMemoryAttribute, uint64_t,
    EFI_MEMORY_ATTRIBUTE_EMA_UC = 0x0000000000000001, /* uncached */
    EFI_MEMORY_ATTRIBUTE_EMA_WC = 0x0000000000000002, /* write-coalescing */
    EFI_MEMORY_ATTRIBUTE_EMA_WT = 0x0000000000000004, /* write-through */
    EFI_MEMORY_ATTRIBUTE_EMA_WB = 0x0000000000000008, /* write-back */
    EFI_MEMORY_ATTRIBUTE_EMA_WP = 0x0000000000001000, /* write-protect */
    EFI_MEMORY_ATTRIBUTE_EMA_RP = 0x0000000000002000, /* read-protect */
    EFI_MEMORY_ATTRIBUTE_EMA_XP = 0x0000000000004000, /* execute-protect */
    EFI_MEMORY_ATTRIBUTE_EMA_RUNTIME = 0x8000000000000000);

EFI_ENUM(EfiMemoryType, uint32_t, EFI_MEMORY_TYPE_RESERVED_MEMORY_TYPE,
         EFI_MEMORY_TYPE_LOADER_CODE, EFI_MEMORY_TYPE_LOADER_DATA,
         EFI_MEMORY_TYPE_BOOT_SERVICES_CODE, EFI_MEMORY_TYPE_BOOT_SERVICES_DATA,
         EFI_MEMORY_TYPE_RUNTIME_SERVICES_CODE,
         EFI_MEMORY_TYPE_RUNTIME_SERVICES_DATA,
         EFI_MEMORY_TYPE_CONVENTIONAL_MEMORY, EFI_MEMORY_TYPE_UNUSABLE_MEMORY,
         EFI_MEMORY_TYPE_ACPIRECLAIM_MEMORY, EFI_MEMORY_TYPE_ACPIMEMORY_NVS,
         EFI_MEMORY_TYPE_MEMORY_MAPPED_IO,
         EFI_MEMORY_TYPE_MEMORY_MAPPED_IOPORT_SPACE, EFI_MEMORY_TYPE_PAL_CODE,
         EFI_MEMORY_TYPE_PERSISTENT_MEMORY, EFI_MEMORY_TYPE_MAX_MEMORY_TYPE);

typedef struct EfiMemoryDescriptor {
  EfiMemoryType memory_type;
  uint32_t padding;
  EfiPhysicalAddr physical_start;
  EfiVirtualAddr virtual_start;
  uint64_t number_of_pages;
  EfiMemoryAttribute attributes;
} EfiMemoryDescriptor;

typedef struct EfiTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t pad1;
  uint32_t nanosecond;
  int16_t timezone;
  uint8_t daylight;
  uint8_t pad2;
} EfiTime;

typedef struct EfiTimeCapabilities {
  uint32_t resolution;
  uint32_t accuracy;
  bool sets_to_zero;
} EfiTimeCapabilities;

typedef struct EfiCapsuleHeader {
  EfiGuid CapsuleGuid;
  uint32_t HeaderSize;
  uint32_t Flags;
  uint32_t CapsuleImageSize;
} EfiCapsuleHeader;

typedef void (*EfiEventNotify)(EfiEvent event, void* ctx);

EFI_ENUM(EfiEventType, uint32_t, EFI_EVENT_TYPE_TIMER = 0x80000000,
         EFI_EVENT_TYPE_RUNTIME = 0x40000000,
         EFI_EVENT_TYPE_NOTIFY_WAIT = 0x00000100,
         EFI_EVENT_TYPE_NOTIFY_SIGNAL = 0x00000200,
         EFI_EVENT_TYPE_SIGNAL_EXIT_BOOT_SERVICES = 0x00000201,
         EFI_EVENT_TYPE_SIGNAL_VIRTUAL_ADDRESS_CHANGE = 0x60000202);

EFI_ENUM(EfiTpl, size_t, EFI_TPL_APPLICATION = 4, EFI_TPL_CALLBACK = 8,
         EFI_TPL_NOTIFY = 16, EFI_TPL_HIGH_LEVEL = 31);

EFI_ENUM(EfiTimerDelay, uint32_t, EFI_TIMER_DELAY_TIMER_CANCEL,
         EFI_TIMER_DELAY_TIMER_PERIODIC, EFI_TIMER_DELAY_TIMER_RELATIVE

);

EFI_ENUM(EfiResetType, uint32_t, EFI_RESET_TYPE_COLD, EFI_RESET_TYPE_WARM,
         EFI_RESET_TYPE_SHUTDOWN, EFI_RESET_TYPE_PLATFORM_SPECIFIC);

#define EFI_ERROR_MASK ((uintptr_t)INTPTR_MAX + 1)
#define EFI_ERR(x) (EFI_ERROR_MASK | (x))

EFI_ENUM(
    EfiStatus, uintptr_t, EFI_STATUS_SUCCESS,
    EFI_STATUS_LOAD_ERROR = EFI_ERR(1),
    EFI_STATUS_INVALID_PARAMETER = EFI_ERR(2),
    EFI_STATUS_UNSUPPORTED = EFI_ERR(3),
    EFI_STATUS_BAD_BUFFER_SIZE = EFI_ERR(4),
    EFI_STATUS_BUFFER_TOO_SMALL = EFI_ERR(5), EFI_STATUS_NOT_READY = EFI_ERR(6),
    EFI_STATUS_DEVICE_ERROR = EFI_ERR(7),
    EFI_STATUS_WRITE_PROTECTED = EFI_ERR(8),
    EFI_STATUS_OUT_OF_RESOURCES = EFI_ERR(9),
    EFI_STATUS_VOLUME_CORRUPTED = EFI_ERR(10),
    EFI_STATUS_VOLUME_FULL = EFI_ERR(11), EFI_STATUS_NO_MEDIA = EFI_ERR(12),
    EFI_STATUS_MEDIA_CHANGED = EFI_ERR(13), EFI_STATUS_NOT_FOUND = EFI_ERR(14),
    EFI_STATUS_ACCESS_DENIED = EFI_ERR(15),
    EFI_STATUS_NO_RESPONSE = EFI_ERR(16), EFI_STATUS_NO_MAPPING = EFI_ERR(17),
    EFI_STATUS_TIMEOUT = EFI_ERR(18), EFI_STATUS_NOT_STARTED = EFI_ERR(19),
    EFI_STATUS_ALREADY_STARTED = EFI_ERR(20), EFI_STATUS_ABORTED = EFI_ERR(21),
    EFI_STATUS_ICMP_ERROR = EFI_ERR(22), EFI_STATUS_TFTP_ERROR = EFI_ERR(23),
    EFI_STATUS_PROTOCOL_ERROR = EFI_ERR(24),
    EFI_STATUS_INCOMPATIBLE_VERSION = EFI_ERR(25),
    EFI_STATUS_SECURITY_VIOLATION = EFI_ERR(26),
    EFI_STATUS_CRC_ERROR = EFI_ERR(27), EFI_STATUS_END_OF_MEDIA = EFI_ERR(28),
    EFI_STATUS_END_OF_FILE = EFI_ERR(31),
    EFI_STATUS_INVALID_LANGUAGE = EFI_ERR(32),
    EFI_STATUS_COMPROMISED_DATA = EFI_ERR(33),
    EFI_STATUS_IP_ADDRESS_CONFLICT = EFI_ERR(34),
    EFI_STATUS_HTTP_ERROR = EFI_ERR(35),
    EFI_STATUS_CONNECTION_FIN = EFI_ERR(104),
    EFI_STATUS_CONNECTION_RESET = EFI_ERR(105),
    EFI_STATUS_CONNECTION_REFUSED = EFI_ERR(106));

#endif  // __EFI_TYPES_H__
