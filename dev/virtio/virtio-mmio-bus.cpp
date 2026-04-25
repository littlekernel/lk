/*
 * Copyright (c) 2024 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <dev/virtio/virtio-mmio-bus.h>

#include <assert.h>
#include <stdint.h>
#include <endian.h>
#include <lk/trace.h>
#include <lk/err.h>
#include <lk/pow2.h>
#include <lk/reg.h>
#include <arch/ops.h>
#include <dev/virtio.h>
#include <dev/virtio/virtio-device.h>
#include <platform/interrupts.h>
#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

#include "virtio_priv.h"

#if WITH_DEV_VIRTIO_BLOCK
#include <dev/virtio/block.h>
#endif
#if WITH_DEV_VIRTIO_NET
#include <dev/virtio/net.h>
#endif
#if WITH_DEV_VIRTIO_GPU
#include <dev/virtio/gpu.h>
#endif
#if WITH_DEV_VIRTIO_9P
#include <dev/virtio/9p.h>
#endif

#define LOCAL_TRACE 0

// V1/V2 config
struct virtio_mmio_config {
    /* 0x00 */
    uint32_t magic;
    uint32_t version;
    uint32_t device_id;
    uint32_t vendor_id;
    /* 0x10 */
    uint32_t host_features;
    uint32_t host_features_sel;
    uint32_t _reserved0[2];
    /* 0x20 */
    uint32_t guest_features;
    uint32_t guest_features_sel;
    uint32_t guest_page_size;
    uint32_t _reserved1[1];
    /* 0x30 */
    uint32_t queue_sel;
    uint32_t queue_num_max;
    uint32_t queue_num;
    uint32_t queue_align;      // v1 (legacy) only
    /* 0x40 */
    uint32_t queue_pfn;        // v1 (legacy) only
    uint32_t queue_ready;      // v2 (modern) only
    uint32_t _reserved2[2];
    /* 0x50 */
    uint32_t queue_notify;
    uint32_t _reserved3[3];
    /* 0x60 */
    uint32_t interrupt_status;
    uint32_t interrupt_ack;
    uint32_t _reserved4[2];
    /* 0x70 */
    uint32_t status;
    uint32_t _reserved5[3];
    /* 0x80 */
    uint32_t queue_desc_low;   // v2 (modern) only
    uint32_t queue_desc_high;  // v2 (modern) only
    uint32_t _reserved6[2];
    /* 0x90 */
    uint32_t queue_avail_low;  // v2 (modern) only
    uint32_t queue_avail_high; // v2 (modern) only
    uint32_t _reserved7[2];
    /* 0xa0 */
    uint32_t queue_used_low;   // v2 (modern) only
    uint32_t queue_used_high;  // v2 (modern) only
    uint32_t _reserved8[21];
    /* 0xfc */
    uint32_t config_generation; // v2 (modern) only
    /* 0x100 */
    uint32_t config[0];
};

STATIC_ASSERT(sizeof(struct virtio_mmio_config) == 0x100);

#define VIRTIO_MMIO_MAGIC 0x74726976 // 'virt'

namespace {

inline uint32_t virtio_mmio_read32(const volatile uint32_t *reg) {
    return LE32(mmio_read32((volatile uint32_t *)reg));
}

inline void virtio_mmio_write32(volatile uint32_t *reg, uint32_t val) {
    mmio_write32(reg, LE32(val));
}

} // namespace

void virtio_mmio_bus::virtio_reset_device() {
    virtio_mmio_write32(&mmio_config_->status, 0);
}

void virtio_mmio_bus::virtio_status_acknowledge_driver() {
    uint32_t status = virtio_mmio_read32(&mmio_config_->status);
    status |= VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER;
    virtio_mmio_write32(&mmio_config_->status, status);

    if (mmio_version_ == 2) {
        // Modern virtio-mmio requires VERSION_1 negotiation in feature word 1.
        constexpr uint32_t version1_bit_word1 = static_cast<uint32_t>(VIRTIO_F_VERSION_1 >> 32);
        uint32_t host_features_word1 = virtio_read_host_feature_word(1);
        uint32_t guest_features_word1 = 0;
        if (host_features_word1 & version1_bit_word1) {
            guest_features_word1 |= version1_bit_word1;
        }
        virtio_set_guest_features(1, guest_features_word1);
    }
}

void virtio_mmio_bus::virtio_status_driver_ok() {
    uint32_t status = virtio_mmio_read32(&mmio_config_->status);

    if (mmio_version_ == 2 && !(status & VIRTIO_STATUS_FEATURES_OK)) {
        status |= VIRTIO_STATUS_FEATURES_OK;
        virtio_mmio_write32(&mmio_config_->status, status);

        status = virtio_mmio_read32(&mmio_config_->status);
        if (!(status & VIRTIO_STATUS_FEATURES_OK)) {
            status |= VIRTIO_STATUS_FAILED;
            virtio_mmio_write32(&mmio_config_->status, status);
            printf("virtio-mmio v2: device rejected feature negotiation\n");
            return;
        }
    }

    status |= VIRTIO_STATUS_DRIVER_OK;
    virtio_mmio_write32(&mmio_config_->status, status);
}

void virtio_mmio_bus::virtio_set_guest_features(uint32_t word, uint32_t features) {
    virtio_mmio_write32(&mmio_config_->guest_features_sel, word);
    virtio_mmio_write32(&mmio_config_->guest_features, features);
}

uint32_t virtio_mmio_bus::virtio_read_host_feature_word(uint32_t word) {
    virtio_mmio_write32(&mmio_config_->host_features_sel, word);
    return virtio_mmio_read32(&mmio_config_->host_features);
}

void virtio_mmio_bus::virtio_kick(uint16_t ring_index) {
    LTRACEF("bus %p, ring %u\n", this, ring_index);

    DEBUG_ASSERT(ring_index < virtio_device::MAX_VIRTIO_RINGS);

    // Ensure descriptors and avail index writes are globally visible before notifying.
    mb();
    virtio_mmio_write32(&mmio_config_->queue_notify, ring_index);
}

void virtio_mmio_bus::register_ring(uint32_t page_size, uint32_t queue_sel, uint32_t queue_num, uint32_t queue_align, uint32_t queue_pfn) {
    DEBUG_ASSERT(mmio_config_);

    virtio_mmio_write32(&mmio_config_->queue_sel, queue_sel);

    uint32_t queue_num_max = virtio_mmio_read32(&mmio_config_->queue_num_max);
    if (queue_num_max == 0 || queue_num > queue_num_max) {
        printf("virtio-mmio: invalid queue size %u (max %u) for queue %u\n", queue_num, queue_num_max, queue_sel);
        return;
    }

    virtio_mmio_write32(&mmio_config_->queue_num, queue_num);

    if (mmio_version_ == 1) {
        virtio_mmio_write32(&mmio_config_->guest_page_size, page_size);
        virtio_mmio_write32(&mmio_config_->queue_align, queue_align);
        virtio_mmio_write32(&mmio_config_->queue_pfn, queue_pfn);
        return;
    }

    if (mmio_version_ == 2) {
        uint64_t queue_pa = static_cast<uint64_t>(queue_pfn) * page_size;
        uint64_t desc_pa = queue_pa;
        uint64_t avail_pa = queue_pa + static_cast<uint64_t>(queue_num) * sizeof(vring_desc);
        uint64_t used_pa = (avail_pa + sizeof(uint16_t) * (3 + queue_num) + queue_align - 1) &
                           ~(static_cast<uint64_t>(queue_align) - 1);

        virtio_mmio_write32(&mmio_config_->queue_ready, 0);
        virtio_mmio_write32(&mmio_config_->queue_desc_low, static_cast<uint32_t>(desc_pa));
        virtio_mmio_write32(&mmio_config_->queue_desc_high, static_cast<uint32_t>(desc_pa >> 32));
        virtio_mmio_write32(&mmio_config_->queue_avail_low, static_cast<uint32_t>(avail_pa));
        virtio_mmio_write32(&mmio_config_->queue_avail_high, static_cast<uint32_t>(avail_pa >> 32));
        virtio_mmio_write32(&mmio_config_->queue_used_low, static_cast<uint32_t>(used_pa));
        virtio_mmio_write32(&mmio_config_->queue_used_high, static_cast<uint32_t>(used_pa >> 32));
        virtio_mmio_write32(&mmio_config_->queue_ready, 1);
    }
}

void dump_mmio_config(const volatile virtio_mmio_config *mmio) {
    printf("mmio at %p\n", mmio);
    printf("\tmagic 0x%x\n", virtio_mmio_read32(&mmio->magic));
    printf("\tversion 0x%x\n", virtio_mmio_read32(&mmio->version));
    printf("\tdevice_id 0x%x\n", virtio_mmio_read32(&mmio->device_id));
    printf("\tvendor_id 0x%x\n", virtio_mmio_read32(&mmio->vendor_id));
    printf("\thost_features 0x%x\n", virtio_mmio_read32(&mmio->host_features));
    printf("\tguest_features 0x%x\n", virtio_mmio_read32(&mmio->guest_features));
    printf("\tguest_features_sel 0x%x\n", virtio_mmio_read32(&mmio->guest_features_sel));
    printf("\tguest_page_size %u\n", virtio_mmio_read32(&mmio->guest_page_size));
    printf("\tqnum %u\n", virtio_mmio_read32(&mmio->queue_num));
    printf("\tqnum_max %u\n", virtio_mmio_read32(&mmio->queue_num_max));
    printf("\tqnum_align %u\n", virtio_mmio_read32(&mmio->queue_align));
    printf("\tqnum_pfn %u\n", virtio_mmio_read32(&mmio->queue_pfn));
    printf("\tstatus 0x%x\n", virtio_mmio_read32(&mmio->status));
}

enum handler_return virtio_mmio_bus::virtio_mmio_irq(void *arg) {
    auto *dev = static_cast<virtio_device *>(arg);
    virtio_mmio_bus *bus = reinterpret_cast<virtio_mmio_bus *>(dev->bus());

    LTRACEF("dev %p, bus %p\n", dev, bus);

    uint32_t irq_status = virtio_mmio_read32(&bus->mmio_config_->interrupt_status);
    LTRACEF("status %#x\n", irq_status);

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (irq_status & 0x1) { /* used ring update */
        // XXX is this safe?
        virtio_mmio_write32(&bus->mmio_config_->interrupt_ack, 0x1);

        auto _ret = dev->handle_queue_interrupt();
        if (_ret == INT_RESCHEDULE) {
            ret = _ret;
        }

    }
    if (irq_status & 0x2) { /* config change */
        virtio_mmio_write32(&bus->mmio_config_->interrupt_ack, 0x2);

        auto _ret = dev->handle_config_interrupt();
        if (_ret == INT_RESCHEDULE) {
            ret = _ret;
        }
    }

    LTRACEF("exiting irq\n");

    return ret;
}

int virtio_mmio_detect(void *ptr, uint count, const uint irqs[], size_t stride) {
    LTRACEF("ptr %p, count %u\n", ptr, count);

    DEBUG_ASSERT(ptr);
    DEBUG_ASSERT(irqs);

    /* allocate an array big enough to hold a list of pointers to devices */
    static virtio_device **devices = new virtio_device *[count] {};
    if (!devices)
        return ERR_NO_MEMORY;

    int found = 0;
    for (uint i = 0; i < count; i++) {
        volatile auto *mmio = reinterpret_cast<virtio_mmio_config *>(static_cast<uint8_t *>(ptr) + i * stride);

        uint32_t magic = virtio_mmio_read32(&mmio->magic);
        uint32_t version = virtio_mmio_read32(&mmio->version);
        uint32_t device_id = virtio_mmio_read32(&mmio->device_id);
        uint32_t vendor_id = virtio_mmio_read32(&mmio->vendor_id);

        if (magic != VIRTIO_MMIO_MAGIC) {
            continue;
        }

        if (device_id == 0) {
            continue;
        }

        if (LOCAL_TRACE) {
            dump_mmio_config(mmio);
        }

        if (version != 1 && version != 2) {
            printf("skipping virtio-mmio device with unsupported version %u\n", version);
            continue;
        }

        if (version == 2) {
            printf("virtio-mmio modern interface (version 2): preliminary support enabled\n");
        }

        auto *bus = new virtio_mmio_bus(mmio, version);
        auto *dev = new virtio_device(bus);
        devices[i] = dev;

        dev->set_config_ptr((void *)mmio->config);

        bus->set_irq(irqs[i]);
        bus->mask_interrupt();
        register_int_handler(irqs[i], &virtio_mmio_bus::virtio_mmio_irq, static_cast<void *>(dev));

        LTRACEF("looking at %p: magic 0x%x version 0x%x did 0x%x vid 0x%x\n",
            mmio, magic, version, device_id, vendor_id);

#if WITH_DEV_VIRTIO_BLOCK
        if (device_id == 0x2) { // virtio-block
            LTRACEF("found block device\n");

            status_t err = virtio_block_init(dev);
            if (err >= 0) {
                found++;
            }
        }
#endif // WITH_DEV_VIRTIO_BLOCK
#if WITH_DEV_VIRTIO_NET
        if (device_id == 1) { // network device
            LTRACEF("found net device\n");

            status_t err = virtio_net_init(dev);
            if (err >= 0) {
                found++;
            }
        }
#endif // WITH_DEV_VIRTIO_NET
#if WITH_DEV_VIRTIO_9P
        if (device_id == 9) { // 9p device
            LTRACEF("found 9p device\n");

            status_t err = virtio_9p_init(dev, bus->virtio_read_host_feature_word((0)));
            if (err >= 0) {
                found++;
                virtio_9p_start(dev);
            }
        }
#endif // WITH_DEV_VIRTIO_9P
#if WITH_DEV_VIRTIO_GPU
        if (device_id == 0x10) { // virtio-gpu
            LTRACEF("found gpu device\n");

            status_t err = virtio_gpu_init(dev);
            if (err >= 0) {
                found++;
                virtio_gpu_start(dev);
            }
        }
#endif // WITH_DEV_VIRTIO_GPU
    }

    return found;
}

