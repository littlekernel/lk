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

// Reference:
// https://uefi.org/specs/UEFI/2.10/24_Network_Protocols_SNP_PXE_BIS.html#simple-network-protocol

#ifndef __SIMPLE_NETWORK_PROTOCOL_H__
#define __SIMPLE_NETWORK_PROTOCOL_H__

#include <stddef.h>

#include "types.h"

#define EFI_SIMPLE_NETWORK_PROTOCOL_REVISION 0x00010000

#define MAX_MCAST_FILTER_CNT 16

typedef struct {
  uint8_t addr[32];
} EfiMacAddress;

typedef struct {
  uint8_t addr[4];
} EfiIpV4Address;

typedef struct {
  uint8_t addr[16];
} EfiIpV6Address;

typedef union {
  EfiIpV4Address v4;
  EfiIpV6Address v6;
  uint32_t addr[4];
} EfiIpAddr;

typedef struct {
  uint32_t state;
  uint32_t hw_address_size;
  uint32_t media_header_size;
  uint32_t max_packet_size;
  uint32_t nv_ram_size;
  uint32_t nv_ram_access_size;
  uint32_t receive_filter_mask;
  uint32_t receive_filter_setting;
  uint32_t max_m_cast_filter_count;
  uint32_t m_cast_filter_count;
  EfiMacAddress m_cast_filter[MAX_MCAST_FILTER_CNT];
  EfiMacAddress current_address;
  EfiMacAddress broadcast_address;
  EfiMacAddress permanent_address;
  uint8_t if_type;
  bool mac_address_changeable;
  bool multiple_tx_supported;
  bool media_present_supported;
  bool media_present;
} EfiSimpleNetworkMode;

typedef struct {
  uint64_t rx_total_frames;
  uint64_t rx_good_frames;
  uint64_t rx_undersize_frames;
  uint64_t rx_oversize_frames;
  uint64_t rx_dropped_frames;
  uint64_t rx_unicast_frames;
  uint64_t rx_broadcast_frames;
  uint64_t rx_multicast_frames;
  uint64_t rx_crc_error_frames;
  uint64_t rx_total_bytes;
  uint64_t tx_total_frames;
  uint64_t tx_good_frames;
  uint64_t tx_undersize_frames;
  uint64_t tx_oversize_frames;
  uint64_t tx_dropped_frames;
  uint64_t tx_unicast_frames;
  uint64_t tx_broadcast_frames;
  uint64_t tx_multicast_frames;
  uint64_t tx_crc_error_frames;
  uint64_t tx_total_bytes;
  uint64_t collisions;
  uint64_t unsupported_protocol;
  uint64_t rx_duplicated_frames;
  uint64_t rx_decrypt_error_frames;
  uint64_t tx_error_frames;
  uint64_t tx_retry_frames;
} EfiNetworkStatistics;

typedef struct EfiSimpleNetworkProtocol {
  uint64_t revision;
  EfiStatus (*start)(struct EfiSimpleNetworkProtocol* self);
  EfiStatus (*stop)(struct EfiSimpleNetworkProtocol* self);
  EfiStatus (*initialize)(struct EfiSimpleNetworkProtocol* self, size_t extra_rx_buffer_size,
                          size_t extra_tx_buffer_size);
  EfiStatus (*reset)(struct EfiSimpleNetworkProtocol* self, bool extended_verification);
  EfiStatus (*shutdown)(struct EfiSimpleNetworkProtocol* self);
  EfiStatus (*receive_filters)(struct EfiSimpleNetworkProtocol* self, uint32_t enable,
                               uint32_t disable, bool reset_mcast_filter, size_t mcast_filter_count,
                               EfiMacAddress* mcast_filter);
  EfiStatus (*station_address)(struct EfiSimpleNetworkProtocol* self, bool reset,
                               EfiMacAddress* new_addr);
  EfiStatus (*statistics)(struct EfiSimpleNetworkProtocol* self, bool reset, size_t* stats_size,
                          EfiNetworkStatistics* stats_table);
  EfiStatus (*m_cast_ip_to_mac)(struct EfiSimpleNetworkProtocol* self, bool ipv6, EfiIpAddr* ip,
                                EfiMacAddress* mac);
  EfiStatus (*nv_data)(struct EfiSimpleNetworkProtocol* self, bool read_write, size_t offset,
                       size_t buf_size, void* buf);
  EfiStatus (*get_status)(struct EfiSimpleNetworkProtocol* self, uint32_t* interrupt_status,
                          void** tx_buf);
  EfiStatus (*transmit)(struct EfiSimpleNetworkProtocol* self, size_t header_size, size_t buf_size,
                        void* buf, EfiMacAddress* src, EfiMacAddress* dest, uint16_t* protocol);
  EfiStatus (*receive)(struct EfiSimpleNetworkProtocol* self, size_t* header_size, size_t* buf_size,
                       void* buf, EfiMacAddress* src, EfiMacAddress* dest, uint16_t* protocol);

  EfiEvent wait_for_packet;
  EfiSimpleNetworkMode* mode;
} EfiSimpleNetworkProtocol;

#endif  // __SIMPLE_NETWORK_PROTOCOL_H__
