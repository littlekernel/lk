/*
 * Copyright (c) 2014-2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <assert.h>
#include <dev/virtio/block.h>
#include <endian.h>
#include <inttypes.h>
#include <kernel/event.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lib/bio.h>
#include <lk/compiler.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/list.h>
#include <lk/trace.h>
#include <stdlib.h>

#if WITH_KERNEL_VM
#include <kernel/vm.h>
#endif

#define LOCAL_TRACE 0

namespace {

struct virtio_blk_config {
    uint64_t capacity;
    uint32_t size_max;
    uint32_t seg_max;
    struct virtio_blk_geometry {
        uint16_t cylinders;
        uint8_t heads;
        uint8_t sectors;
    } geometry;
    uint32_t blk_size;
    struct virtio_blk_topology {
        uint8_t physical_block_exp;
        uint8_t alignment_offset;
        uint16_t min_io_size;
        uint32_t opt_io_size;
    } topology;
    uint8_t writeback;
    uint8_t unused[3];
    uint32_t max_discard_sectors;
    uint32_t max_discard_seq;
    uint32_t discard_sector_alignment;
    uint32_t max_write_zeroes_sectors;
    uint32_t max_write_zeroes_seq;
    uint8_t write_zeros_may_unmap;
    uint8_t unused1[3];
    uint32_t max_secure_erase_sectors;
    uint32_t max_secure_erase_seg;
    uint32_t secure_erase_sector_alignment;
    struct virtio_blk_zoned_characteristics {
        uint32_t zone_sectors;
        uint32_t max_open_zones;
        uint32_t max_active_zones;
        uint32_t max_append_sectors;
        uint32_t write_granularity;
        uint8_t model;
        uint8_t unused2[3];
    } zoned;
};
static_assert(sizeof(virtio_blk_config) == 96, "virtio_blk_config size mismatch");

struct virtio_blk_req {
    uint32_t type;
    uint32_t ioprio; // v1.3 says this is 'reserved'
    uint64_t sector;
};
static_assert(sizeof(virtio_blk_req) == 16, "virtio_blk_req size mismatch");

struct virtio_blk_discard_write_zeroes {
    uint64_t sector;
    uint32_t num_sectors;
    struct {
        uint32_t unmap : 1;
        uint32_t reserved : 31;
    } flags;
};
static_assert(sizeof(virtio_blk_discard_write_zeroes) == 16, "virtio_blk_discard_write_zeroes size mismatch");

struct virtio_block_txn {
    /* bio callback, for async */
    void *cookie;
    size_t len;

    /* for async calls */
    void (*callback)(void *, struct bdev *, ssize_t);
    /* virtio request structure, must be DMA-able */
    struct virtio_blk_req req;

    /* response status, must be DMA-able */
    uint8_t status;
};

constexpr uint32_t VIRTIO_BLK_F_BARRIER = (1 << 0); // legacy
constexpr uint32_t VIRTIO_BLK_F_SIZE_MAX = (1 << 1);
constexpr uint32_t VIRTIO_BLK_F_SEG_MAX = (1 << 2);
constexpr uint32_t VIRTIO_BLK_F_GEOMETRY = (1 << 4);
constexpr uint32_t VIRTIO_BLK_F_RO = (1 << 5);
constexpr uint32_t VIRTIO_BLK_F_BLK_SIZE = (1 << 6);
constexpr uint32_t VIRTIO_BLK_F_SCSI = (1 << 7); // legacy
constexpr uint32_t VIRTIO_BLK_F_FLUSH = (1 << 9);
constexpr uint32_t VIRTIO_BLK_F_TOPOLOGY = (1 << 10);
constexpr uint32_t VIRTIO_BLK_F_CONFIG_WCE = (1 << 11);
constexpr uint32_t VIRTIO_BLK_F_MQ = (1 << 12);
constexpr uint32_t VIRTIO_BLK_F_DISCARD = (1 << 13);
constexpr uint32_t VIRTIO_BLK_F_WRITE_ZEROES = (1 << 14);
constexpr uint32_t VIRTIO_BLK_F_LIFETIME = (1 << 15);
constexpr uint32_t VIRTIO_BLK_F_SECURE_ERASE = (1 << 16);
constexpr uint32_t VIRTIO_BLK_F_ZONED = (1 << 17);

constexpr uint32_t VIRTIO_BLK_T_IN = 0;
constexpr uint32_t VIRTIO_BLK_T_OUT = 1;
constexpr uint32_t VIRTIO_BLK_T_FLUSH = 4;
constexpr uint32_t VIRTIO_BLK_T_GET_ID = 8;
constexpr uint32_t VIRTIO_BLK_T_GET_LIFETIME = 10;
constexpr uint32_t VIRTIO_BLK_T_DISCARD = 11;
constexpr uint32_t VIRTIO_BLK_T_WRITE_ZEROES = 13;
constexpr uint32_t VIRTIO_BLK_T_SECURE_ERASE = 14;
constexpr uint32_t VIRTIO_BLK_T_ZONE_APPEND = 15;
constexpr uint32_t VIRTIO_BLK_T_ZONE_REPORT = 16;
constexpr uint32_t VIRTIO_BLK_T_ZONE_OPEN = 17;
constexpr uint32_t VIRTIO_BLK_T_ZONE_CLOSE = 18;
constexpr uint32_t VIRTIO_BLK_T_ZONE_FINISH = 19;
constexpr uint32_t VIRTIO_BLK_T_ZONE_RESET = 24;
constexpr uint32_t VIRTIO_BLK_T_ZONE_RESET_ALL = 26;

constexpr uint32_t VIRTIO_BLK_S_OK = 0;
constexpr uint32_t VIRTIO_BLK_S_IOERR = 1;
constexpr uint32_t VIRTIO_BLK_S_UNSUPP = 2;

constexpr uint16_t VIRTIO_BLK_RING_LEN = 256;

enum handler_return virtio_block_irq_driver_callback(virtio_device *dev, uint ring, const vring_used_elem *e);
ssize_t virtio_bdev_read_block(bdev *bdev, void *buf, bnum_t block, uint count);
ssize_t virtio_bdev_write_block(bdev *bdev, const void *buf, bnum_t block, uint count);
status_t virtio_bdev_read_async(
    bdev *bdev, void *buf, off_t offset, size_t len,
    void (*callback)(void *, struct bdev *, ssize_t), void *cookie);
status_t virtio_bdev_write_async(
    bdev *bdev, const void *buf, off_t offset, size_t len,
    void (*callback)(void *, struct bdev *, ssize_t), void *cookie);

struct virtio_block_dev {
    virtio_device *dev;

    /* bio block device */
    bdev_t bdev;

    /* our negotiated guest features */
    uint32_t guest_features;
    bool readonly;
    virtio_block_txn *txns;
};



void dump_feature_bits(const char *name, uint32_t feature) {
    printf("virtio-block %s features (%#x):", name, feature);
    if (feature & VIRTIO_BLK_F_BARRIER) {
        printf(" BARRIER");
    }
    if (feature & VIRTIO_BLK_F_SIZE_MAX) {
        printf(" SIZE_MAX");
    }
    if (feature & VIRTIO_BLK_F_SEG_MAX) {
        printf(" SEG_MAX");
    }
    if (feature & VIRTIO_BLK_F_GEOMETRY) {
        printf(" GEOMETRY");
    }
    if (feature & VIRTIO_BLK_F_RO) {
        printf(" RO");
    }
    if (feature & VIRTIO_BLK_F_BLK_SIZE) {
        printf(" BLK_SIZE");
    }
    if (feature & VIRTIO_BLK_F_SCSI) {
        printf(" SCSI");
    }
    if (feature & VIRTIO_BLK_F_FLUSH) {
        printf(" FLUSH");
    }
    if (feature & VIRTIO_BLK_F_TOPOLOGY) {
        printf(" TOPOLOGY");
    }
    if (feature & VIRTIO_BLK_F_CONFIG_WCE) {
        printf(" CONFIG_WCE");
    }
    if (feature & VIRTIO_BLK_F_MQ) {
        printf(" MQ");
    }
    if (feature & VIRTIO_BLK_F_DISCARD) {
        printf(" DISCARD");
    }
    if (feature & VIRTIO_BLK_F_WRITE_ZEROES) {
        printf(" WRITE_ZEROES");
    }
    if (feature & VIRTIO_BLK_F_LIFETIME) {
        printf(" LIFETIME");
    }
    if (feature & VIRTIO_BLK_F_SECURE_ERASE) {
        printf(" SECURE_ERASE");
    }
    if (feature & VIRTIO_BLK_F_ZONED) {
        printf(" ZONED");
    }
    printf("\n");
}

} // namespace

status_t virtio_block_init(virtio_device *dev) {
    LTRACEF("dev %p\n", dev);

    /* allocate a new block device */
    auto *bdev = (virtio_block_dev *)malloc(sizeof(virtio_block_dev));
    if (!bdev) {
        return ERR_NO_MEMORY;
    }

    bdev->dev = dev;
    dev->set_priv(bdev);

    /* make sure the device is reset */
    dev->bus()->virtio_reset_device();


    // Per virtio spec: v2 (modern) config becomes little-endian after VERSION_1
    // negotiation completes; v1 (legacy) remains native-endian.
    const bool modern = dev->config_is_modern();
    dprintf(INFO, "virtio-block: modern config %u, expecting %s-endian config\n",
            modern, modern ? "little" : "native");

    /* ack and set the driver status bit */
    dev->bus()->virtio_status_acknowledge_driver();

    uint32_t host_features = dev->bus()->virtio_read_host_feature_word(0);
    bdev->readonly = (host_features & VIRTIO_BLK_F_RO) != 0;
    bdev->guest_features = host_features;
    /* keep the features we understand or can tolerate */
    bdev->guest_features &= (VIRTIO_BLK_F_SIZE_MAX |
                             VIRTIO_BLK_F_RO |
                             VIRTIO_BLK_F_FLUSH |
                             VIRTIO_BLK_F_BLK_SIZE |
                             VIRTIO_BLK_F_GEOMETRY |
                             VIRTIO_BLK_F_TOPOLOGY |
                             VIRTIO_BLK_F_CONFIG_WCE);
    dev->bus()->virtio_set_guest_features(0, bdev->guest_features);

    // If supported, prefer writeback mode for better throughput.
    if (bdev->guest_features & VIRTIO_BLK_F_CONFIG_WCE) {
        dev->config_write8(offsetof(virtio_blk_config, writeback), 1);
    }

    /* allocate a virtio ring */
    dev->virtio_alloc_ring(0, VIRTIO_BLK_RING_LEN);

    // descriptor index would be used to index into the txns array
    // This is a simple way to keep track of which transaction entry is
    // free, and which transaction entry corresponds to which descriptor.
    // Hence, we allocate txns array with the same size as the ring.
    bdev->txns = static_cast<struct virtio_block_txn *>(memalign(alignof(struct virtio_block_txn),
                                                                 VIRTIO_BLK_RING_LEN * sizeof(struct virtio_block_txn)));

    /* set our irq handler */
    dev->set_irq_callbacks(&virtio_block_irq_driver_callback, nullptr);
    dev->bus()->unmask_interrupt();

    /* set DRIVER_OK */
    dev->bus()->virtio_status_driver_ok();

    // Read config after DRIVER_OK so VERSION_1 negotiation has taken effect.
    const uint64_t capacity = dev->config_read64(offsetof(virtio_blk_config, capacity));
    const uint32_t size_max = dev->config_read32(offsetof(virtio_blk_config, size_max));
    const uint32_t seg_max = dev->config_read32(offsetof(virtio_blk_config, seg_max));
    const uint32_t blk_size = dev->config_read32(offsetof(virtio_blk_config, blk_size));

    LTRACEF("capacity %" PRIx64 "\n", capacity);
    LTRACEF("size_max %#x\n", size_max);
    LTRACEF("seg_max  %#x\n", seg_max);
    LTRACEF("blk_size %#x\n", blk_size);

    /* construct the block device */
    static uint8_t found_index = 0;
    char buf[16];
    snprintf(buf, sizeof(buf), "virtio%u", found_index++);
    bio_initialize_bdev(&bdev->bdev, buf,
                        blk_size, capacity,
                        0, NULL, BIO_FLAGS_NONE);

    /* override our block device hooks */
    bdev->bdev.read_block = &virtio_bdev_read_block;
    bdev->bdev.write_block = &virtio_bdev_write_block;
    bdev->bdev.read_async = &virtio_bdev_read_async;
    bdev->bdev.write_async = &virtio_bdev_write_async;

    bio_register_device(&bdev->bdev);

    printf("virtio-block found device of size %" PRIu64 "\n", capacity * blk_size);

    /* dump feature bits */
    dump_feature_bits("host", host_features);
    dump_feature_bits("guest", bdev->guest_features);
    if (bdev->readonly) {
        printf("\tdevice mode: read-only\n");
    }
    if (host_features & VIRTIO_BLK_F_CONFIG_WCE) {
        printf("\twriteback mode: %s\n",
               dev->config_read8(offsetof(virtio_blk_config, writeback)) ? "enabled" : "disabled");
    }
    printf("\tsize_max %u seg_max %u\n", size_max, seg_max);
    if (host_features & VIRTIO_BLK_F_GEOMETRY) {
        printf("\tgeometry: cyl %u head %u sector %u\n",
               dev->config_read16(offsetof(virtio_blk_config, geometry.cylinders)),
               dev->config_read8(offsetof(virtio_blk_config, geometry.heads)),
               dev->config_read8(offsetof(virtio_blk_config, geometry.sectors)));
    }
    if (host_features & VIRTIO_BLK_F_BLK_SIZE) {
        printf("\tblock_size: %u\n", blk_size);
    }
    if (host_features & VIRTIO_BLK_F_TOPOLOGY) {
        printf("\ttopology: block exp %u alignment_offset %u min_io_size %u opt_io_size %u\n",
               dev->config_read8(offsetof(virtio_blk_config, topology.physical_block_exp)),
               dev->config_read8(offsetof(virtio_blk_config, topology.alignment_offset)),
               dev->config_read16(offsetof(virtio_blk_config, topology.min_io_size)),
               dev->config_read32(offsetof(virtio_blk_config, topology.opt_io_size)));
    }
    if (host_features & VIRTIO_BLK_F_DISCARD) {
        printf("\tdiscard: max sectors %u max sequence %u alignment %u\n",
               dev->config_read32(offsetof(virtio_blk_config, max_discard_sectors)),
               dev->config_read32(offsetof(virtio_blk_config, max_discard_seq)),
               dev->config_read32(offsetof(virtio_blk_config, discard_sector_alignment)));
    }
    if (host_features & VIRTIO_BLK_F_WRITE_ZEROES) {
        printf("\twrite zeroes: max sectors %u max sequence %u may unmap %u\n",
               dev->config_read32(offsetof(virtio_blk_config, max_write_zeroes_sectors)),
               dev->config_read32(offsetof(virtio_blk_config, max_write_zeroes_seq)),
               dev->config_read8(offsetof(virtio_blk_config, write_zeros_may_unmap)));
    }

    return NO_ERROR;
}

namespace {

enum handler_return virtio_block_irq_driver_callback(virtio_device *dev, uint ring, const struct vring_used_elem *e) {
    auto *bdev = (virtio_block_dev *)dev->priv();

    struct virtio_block_txn *txn = &bdev->txns[e->id];
    LTRACEF("dev %p, ring %u, e %p, id %u, len %u, status %d\n", dev, ring, e, e->id, e->len, txn->status);

    /* parse our descriptor chain, add back to the free queue */
    uint16_t i = e->id;
    for (;;) {
        int next;
        vring_desc *desc = dev->virtio_desc_index_to_desc(ring, i);

        // virtio_dump_desc(desc);

        const bool modern = dev->config_is_modern();
        if (vring_desc_read_flags(desc, modern) & VRING_DESC_F_NEXT) {
            next = vring_desc_read_next(desc, modern);
        } else {
            /* end of chain */
            next = -1;
        }

        dev->virtio_free_desc(ring, i);

        if (next < 0) {
            break;
        }
        i = next;
    }

    if (txn->callback) {
        // async
        ssize_t result =
            (txn->status == VIRTIO_BLK_S_OK) ? (ssize_t)txn->len : ERR_IO;
        LTRACEF("calling callback %p with cookie %p, len %ld\n", txn->callback,
                txn->cookie, result);
        txn->callback(txn->cookie, &bdev->bdev, result);
    }

    return INT_RESCHEDULE;
}

status_t virtio_block_do_txn(virtio_device *dev, void *buf,
                             off_t offset, size_t len, bool write,
                             bio_async_callback_t callback, void *cookie,
                             virtio_block_txn **txn_out) {
    auto *bdev = (virtio_block_dev *)dev->priv();

    uint16_t i;
    vring_desc *desc;

    LTRACEF("dev %p, buf %p, offset 0x%llx, len %zu\n", dev, buf, offset, len);

    /* put together a transfer */
    desc = dev->virtio_alloc_desc_chain(0, 3, &i);
    LTRACEF("after alloc chain desc %p, i %u\n", desc, i);
    if (!desc) {
        return ERR_NO_RESOURCES;
    }
    struct virtio_block_txn *txn = &bdev->txns[i];
    /* set up the request */
    txn->req.type = dev->ring_swap32(write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN);
    txn->req.ioprio = dev->ring_swap32(0);
    txn->req.sector = dev->ring_swap64((uint64_t)offset / 512);

    txn->callback = callback;
    txn->cookie = cookie;
    txn->len = len;
    LTRACEF("blk_req type %u ioprio %u sector %llu\n", dev->ring_swap32(txn->req.type),
            dev->ring_swap32(txn->req.ioprio), dev->ring_swap64(txn->req.sector));

    const bool modern = dev->config_is_modern();

    if (txn_out) {
        *txn_out = txn;
    }

    // XXX not cache safe.
    // At the moment only tested on arm qemu, which doesn't emulate cache.

    /* set up the descriptor pointing to the head */
#if WITH_KERNEL_VM
    paddr_t req_phys = vaddr_to_paddr(&txn->req);
#else
    paddr_t req_phys = (uint64_t)(uintptr_t)&txn->req;
#endif
    vring_desc_write_addr(desc, req_phys, modern);
    vring_desc_write_len(desc, sizeof(virtio_blk_req), modern);
    vring_desc_write_flags(desc, VRING_DESC_F_NEXT, modern);

    /* set up the descriptor pointing to the buffer */
    desc = dev->virtio_desc_index_to_desc(0, vring_desc_read_next(desc, modern));
#if WITH_KERNEL_VM
    /* translate the first buffer */
    vaddr_t va = (vaddr_t)buf;
    paddr_t pa = vaddr_to_paddr((void *)va);
    vring_desc_write_addr(desc, (uint64_t)pa, modern);
    /* desc->len is filled in below */
#else
    /* non VM world simply queues a single buffer that transfers the whole thing */
    vring_desc_write_addr(desc, (uint64_t)(uintptr_t)buf, modern);
    vring_desc_write_len(desc, len, modern);
#endif
    vring_desc_write_flags(desc, (write ? 0 : VRING_DESC_F_WRITE) | VRING_DESC_F_NEXT, modern);

#if WITH_KERNEL_VM
    /* see if we need to add more descriptors due to scatter gather */
    paddr_t next_pa = PAGE_ALIGN(pa + 1);
    vring_desc_write_len(desc, MIN(next_pa - pa, len), modern);
    LTRACEF("first descriptor va 0x%lx desc->addr 0x%llx desc->len %u\n", va,
            vring_desc_read_addr(desc, modern), vring_desc_read_len(desc, modern));

    size_t remaining_len = len;
    remaining_len -= vring_desc_read_len(desc, modern);
    while (remaining_len > 0) {
        /* amount of source buffer handled by this iteration of the loop */
        size_t len_tohandle = MIN(remaining_len, PAGE_SIZE);

        /* translate the next page in the buffer */
        va = PAGE_ALIGN(va + 1);
        pa = vaddr_to_paddr((void *)va);
        LTRACEF("va now 0x%lx, pa 0x%lx, next_pa 0x%lx, remaining len %zu\n", va, pa, next_pa, remaining_len);

        /* is the new translated physical address contiguous to the last one? */
        if (next_pa == pa) {
            /* we can simply extend the previous descriptor by another page */
            LTRACEF("extending last one by %zu bytes\n", len_tohandle);
            vring_desc_write_len(desc, vring_desc_read_len(desc, modern) + len_tohandle, modern);
        } else {
            /* new physical page needed, allocate a new descriptor and start again */
            uint16_t next_i = dev->virtio_alloc_desc(0);
            vring_desc *next_desc = dev->virtio_desc_index_to_desc(0, next_i);
            DEBUG_ASSERT(next_desc);

            LTRACEF("doesn't extend, need new desc, allocated desc %i (%p)\n", next_i, next_desc);

            /* fill this descriptor in and put it after the last one but before the response descriptor */
            vring_desc_write_addr(next_desc, (uint64_t)pa, modern);
            vring_desc_write_len(next_desc, len_tohandle, modern);
            vring_desc_write_flags(next_desc,
                                   (write ? 0 : VRING_DESC_F_WRITE) | VRING_DESC_F_NEXT, modern);
            vring_desc_write_next(next_desc, vring_desc_read_next(desc, modern), modern);
            vring_desc_write_next(desc, next_i, modern);

            desc = next_desc;
        }
        remaining_len -= len_tohandle;
        next_pa += PAGE_SIZE;
    }
#endif

    /* set up the descriptor pointing to the response */
#if WITH_KERNEL_VM
    paddr_t status_phys = vaddr_to_paddr(&txn->status);
#else
    paddr_t status_phys = (uint64_t)(uintptr_t)&txn->status;
#endif
    desc = dev->virtio_desc_index_to_desc(0, vring_desc_read_next(desc, modern));
    vring_desc_write_addr(desc, status_phys, modern);
    vring_desc_write_len(desc, 1, modern);
    vring_desc_write_flags(desc, VRING_DESC_F_WRITE, modern);

    /* submit the transfer */
    dev->virtio_submit_chain(0, i);

    /* kick it off */
    dev->bus()->virtio_kick(0);

    return NO_ERROR;
}

// TODO: handle partial block transfers
void sync_completion_cb(void *cookie, struct bdev *dev, ssize_t bytes) {
    DEBUG_ASSERT(cookie);
    event_t *event = (event_t *)cookie;
    event_signal(event, false);
}

ssize_t virtio_block_read_write(virtio_device *dev, void *buf,
                                const off_t offset, const size_t len,
                                const bool write) {
    struct virtio_block_txn *txn;
    event_t event;
    event_init(&event, false, EVENT_FLAG_AUTOUNSIGNAL);

    status_t err = virtio_block_do_txn(dev, buf, offset, len, write,
                                       &sync_completion_cb, &event, &txn);
    if (err < 0) {
        return err;
    }

    /* wait for the transfer to complete */
    event_wait(&event);

    LTRACEF("status 0x%hhx\n", txn->status);

    ssize_t result = (txn->status == VIRTIO_BLK_S_OK) ? (ssize_t)len : ERR_IO;

    return result;
}

ssize_t virtio_bdev_read_block(bdev *bdev, void *buf, bnum_t block, uint count) {
    virtio_block_dev *dev = containerof(bdev, struct virtio_block_dev, bdev);

    LTRACEF("dev %p, buf %p, block 0x%x, count %u\n", bdev, buf, block, count);

    ssize_t result = virtio_block_read_write(dev->dev, buf, (off_t)block * dev->bdev.block_size,
                                             count * dev->bdev.block_size, false);
    return result;
}

status_t virtio_bdev_read_async(bdev *bdev, void *buf,
                                off_t offset, size_t len,
                                bio_async_callback_t callback,
                                void *cookie) {
    struct virtio_block_dev *dev =
        containerof(bdev, struct virtio_block_dev, bdev);

    return virtio_block_do_txn(dev->dev, buf, offset, len, false, callback,
                               cookie, NULL);
}

status_t virtio_bdev_write_async(bdev *bdev, const void *buf,
                                 off_t offset, size_t len,
                                 bio_async_callback_t callback,
                                 void *cookie) {
    struct virtio_block_dev *dev =
        containerof(bdev, struct virtio_block_dev, bdev);

    if (dev->readonly) {
        return ERR_NOT_SUPPORTED;
    }

    return virtio_block_do_txn(dev->dev, (void *)buf, offset, len, true,
                               callback, cookie, NULL);
}

ssize_t virtio_bdev_write_block(bdev *bdev, const void *buf, bnum_t block, uint count) {
    struct virtio_block_dev *dev = containerof(bdev, struct virtio_block_dev, bdev);

    LTRACEF("dev %p, buf %p, block 0x%x, count %u\n", bdev, buf, block, count);

    if (dev->readonly) {
        return ERR_NOT_SUPPORTED;
    }

    ssize_t result = virtio_block_read_write(dev->dev, (void *)buf, (off_t)block * dev->bdev.block_size,
                                             count * dev->bdev.block_size, true);
    return result;
}

} // namespace
