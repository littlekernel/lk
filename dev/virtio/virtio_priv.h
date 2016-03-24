/*
 * Copyright (c) 2014 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <compiler.h>
#include <stdint.h>

struct virtio_mmio_config {
    /* 0x00 */  uint32_t magic;
    uint32_t version;
    uint32_t device_id;
    uint32_t vendor_id;
    /* 0x10 */  uint32_t host_features;
    uint32_t host_features_sel;
    uint32_t __reserved0[2];
    /* 0x20 */  uint32_t guest_features;
    uint32_t guest_features_sel;
    uint32_t guest_page_size;
    uint32_t __reserved1[1];
    /* 0x30 */  uint32_t queue_sel;
    uint32_t queue_num_max;
    uint32_t queue_num;
    uint32_t queue_align;
    /* 0x40 */  uint32_t queue_pfn;
    uint32_t __reserved2[3];
    /* 0x50 */  uint32_t queue_notify;
    uint32_t __reserved3[3];
    /* 0x60 */  uint32_t interrupt_status;
    uint32_t interrupt_ack;
    uint32_t __reserved4[2];
    /* 0x70 */  uint32_t status;
    uint8_t __reserved5[0x8c];
    /* 0x100 */ uint32_t config[0];
};

STATIC_ASSERT(sizeof(struct virtio_mmio_config) == 0x100);

#define VIRTIO_MMIO_MAGIC 0x74726976 // 'virt'

#define VIRTIO_STATUS_ACKNOWLEDGE (1<<0)
#define VIRTIO_STATUS_DRIVER      (1<<1)
#define VIRTIO_STATUS_DRIVER_OK   (1<<2)
#define VIRTIO_STATUS_FEATURES_OK (1<<3)
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET (1<<6)
#define VIRTIO_STATUS_FAILED      (1<<7)
