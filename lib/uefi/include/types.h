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

#ifndef __EFI_TYPES_H__
#define __EFI_TYPES_H__
#include <stddef.h>
#include <stdint.h>

using EfiTableHeader = struct EfiTableHeader {
  uint64_t signature;
  uint32_t revision;
  uint32_t header_size;
  uint32_t crc32;
  uint32_t reserved;
};

using EfiGuid = struct EfiGuid {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  uint8_t data4[8];
};

using EfiHandle = void *;
using EfiEvent = void *;
using EfiPhysicalAddr = uint64_t;
using EfiVirtualAddr = uint64_t;

using EfiEventNotify = void (*)(EfiEvent, void *);

using EfiEventType = enum EFI_EVENT_TYPE : uint32_t {
  TIMER = 0x80000000,
  RUNTIME = 0x40000000,
  NOTIFY_WAIT = 0x00000100,
  NOTIFY_SIGNAL = 0x00000200,
  SIGNAL_EXIT_BOOT_SERVICES = 0x00000201,
  SIGNAL_VIRTUAL_ADDRESS_CHANGE = 0x60000202,
};

using EfiTpl = enum EFI_TPL : size_t {
  APPLICATION = 4,
  CALLBACK = 8,
  NOTIFY = 16,
  HIGH_LEVEL = 31,
};

using EfiTimerDelay = enum EFI_TIMER_DELAY {
  TIMER_CANCEL,
  TIMER_PERIODIC,
  TIMER_RELATIVE
};

enum EFI_MEMORY_TYPE {
  RESERVED_MEMORY_TYPE,
  LOADER_CODE,
  LOADER_DATA,
  BOOT_SERVICES_CODE,
  BOOT_SERVICES_DATA,
  RUNTIME_SERVICES_CODE,
  RUNTIME_SERVICES_DATA,
  CONVENTIONAL_MEMORY,
  UNUSABLE_MEMORY,
  ACPIRECLAIM_MEMORY,
  ACPIMEMORY_NVS,
  MEMORY_MAPPED_IO,
  MEMORY_MAPPED_IOPORT_SPACE,
  PAL_CODE,
  PERSISTENT_MEMORY,
  MAX_MEMORY_TYPE
};

typedef EFI_MEMORY_TYPE EfiMemoryType;

#define EFI_ERROR_MASK ((uintptr_t)INTPTR_MAX + 1)
#define EFI_ERR(x) (EFI_ERROR_MASK | (x))

using EfiStatus = enum EFI_STATUS : uintptr_t {
  SUCCESS = 0u,
  LOAD_ERROR = EFI_ERR(1),
  INVALID_PARAMETER = EFI_ERR(2),
  UNSUPPORTED = EFI_ERR(3),
  BAD_BUFFER_SIZE = EFI_ERR(4),
  BUFFER_TOO_SMALL = EFI_ERR(5),
  NOT_READY = EFI_ERR(6),
  DEVICE_ERROR = EFI_ERR(7),
  WRITE_PROTECTED = EFI_ERR(8),
  OUT_OF_RESOURCES = EFI_ERR(9),
  VOLUME_CORRUPTED = EFI_ERR(10),
  VOLUME_FULL = EFI_ERR(11),
  NO_MEDIA = EFI_ERR(12),
  MEDIA_CHANGED = EFI_ERR(13),
  NOT_FOUND = EFI_ERR(14),
  ACCESS_DENIED = EFI_ERR(15),
  NO_RESPONSE = EFI_ERR(16),
  NO_MAPPING = EFI_ERR(17),
  TIMEOUT = EFI_ERR(18),
  NOT_STARTED = EFI_ERR(19),
  ALREADY_STARTED = EFI_ERR(20),
  ABORTED = EFI_ERR(21),
  ICMP_ERROR = EFI_ERR(22),
  TFTP_ERROR = EFI_ERR(23),
  PROTOCOL_ERROR = EFI_ERR(24),
  INCOMPATIBLE_VERSION = EFI_ERR(25),
  SECURITY_VIOLATION = EFI_ERR(26),
  CRC_ERROR = EFI_ERR(27),
  END_OF_MEDIA = EFI_ERR(28),
  END_OF_FILE = EFI_ERR(31),
  INVALID_LANGUAGE = EFI_ERR(32),
  COMPROMISED_DATA = EFI_ERR(33),
  IP_ADDRESS_CONFLICT = EFI_ERR(34),
  HTTP_ERROR = EFI_ERR(35),
  CONNECTION_FIN = EFI_ERR(104),
  CONNECTION_RESET = EFI_ERR(105),
  CONNECTION_REFUSED = EFI_ERR(106),
};

///
/// EFI Time Abstraction:
///  Year:       1900 - 9999
///  Month:      1 - 12
///  Day:        1 - 31
///  Hour:       0 - 23
///  Minute:     0 - 59
///  Second:     0 - 59
///  Nanosecond: 0 - 999,999,999
///  TimeZone:   -1440 to 1440 or 2047
///
using EfiTime = struct {
  uint16_t Year;
  uint8_t Month;
  uint8_t Day;
  uint8_t Hour;
  uint8_t Minute;
  uint8_t Second;
  uint8_t Pad1;
  uint32_t Nanosecond;
  int16_t TimeZone;
  uint8_t Daylight;
  uint8_t Pad2;
};

///
/// This provides the capabilities of the
/// real time clock device as exposed through the EFI interfaces.
///
using EFI_TIME_CAPABILITIES = struct {
  ///
  /// Provides the reporting resolution of the real-time clock device in
  /// counts per second. For a normal PC-AT CMOS RTC device, this
  /// value would be 1 Hz, or 1, to indicate that the device only reports
  /// the time to the resolution of 1 second.
  ///
  uint32_t Resolution;
  ///
  /// Provides the timekeeping accuracy of the real-time clock in an
  /// error rate of 1E-6 parts per million. For a clock with an accuracy
  /// of 50 parts per million, the value in this field would be
  /// 50,000,000.
  ///
  uint32_t Accuracy;
  ///
  /// A TRUE indicates that a time set operation clears the device's
  /// time below the Resolution reporting level. A FALSE
  /// indicates that the state below the Resolution level of the
  /// device is not cleared when the time is set. Normal PC-AT CMOS
  /// RTC devices set this value to FALSE.
  ///
  bool SetsToZero;
};

#endif // __EFI_TYPES_H__
