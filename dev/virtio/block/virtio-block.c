/*
 * Copyright (c) 2014-2015 Travis Geiselbrecht
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
#include <dev/virtio/block.h>

#include <debug.h>
#include <assert.h>
#include <trace.h>
#include <compiler.h>
#include <list.h>
#include <err.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <kernel/vm.h>
#include <lib/bio.h>

#define LOCAL_TRACE 0

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
} __PACKED;

struct virtio_blk_req {
    uint32_t type;
    uint32_t ioprio;
    uint64_t sector;
} __PACKED;

#define VIRTIO_BLK_F_BARRIER  (1<<0)
#define VIRTIO_BLK_F_SIZE_MAX (1<<1)
#define VIRTIO_BLK_F_SEG_MAX  (1<<2)
#define VIRTIO_BLK_F_GEOMETRY (1<<4)
#define VIRTIO_BLK_F_RO       (1<<5)
#define VIRTIO_BLK_F_BLK_SIZE (1<<6)
#define VIRTIO_BLK_F_SCSI     (1<<7)
#define VIRTIO_BLK_F_FLUSH    (1<<9)
#define VIRTIO_BLK_F_TOPOLOGY (1<<10)
#define VIRTIO_BLK_F_CONFIG_WCE (1<<11)

#define VIRTIO_BLK_T_IN         0
#define VIRTIO_BLK_T_OUT        1
#define VIRTIO_BLK_T_FLUSH      4

#define VIRTIO_BLK_S_OK         0
#define VIRTIO_BLK_S_IOERR      1
#define VIRTIO_BLK_S_UNSUPP     2

static enum handler_return virtio_block_irq_driver_callback(struct virtio_device *dev, uint ring, const struct vring_used_elem *e);
static ssize_t virtio_bdev_read_block(struct bdev *bdev, void *buf, bnum_t block, uint count);
static ssize_t virtio_bdev_write_block(struct bdev *bdev, const void *buf, bnum_t block, uint count);

struct virtio_block_dev {
    struct virtio_device *dev;

    mutex_t lock;
    event_t io_event;

    /* bio block device */
    bdev_t bdev;

    /* one blk_req structure for io, not crossing a page boundary */
    struct virtio_blk_req *blk_req;
    paddr_t blk_req_phys;

    /* one uint8_t response word */
    uint8_t blk_response;
    paddr_t blk_response_phys;
};

status_t virtio_block_init(struct virtio_device *dev, uint32_t host_features)
{
    LTRACEF("dev %p, host_features 0x%x\n", dev, host_features);

    /* allocate a new block device */
    struct virtio_block_dev *bdev = malloc(sizeof(struct virtio_block_dev));
    if (!bdev)
        return ERR_NO_MEMORY;

    mutex_init(&bdev->lock);
    event_init(&bdev->io_event, false, EVENT_FLAG_AUTOUNSIGNAL);

    bdev->dev = dev;
    dev->priv = bdev;

    bdev->blk_req = memalign(sizeof(struct virtio_blk_req), sizeof(struct virtio_blk_req));
#if WITH_KERNEL_VM
    bdev->blk_req_phys = vaddr_to_paddr(bdev->blk_req);
#else
    bdev->blk_freq_phys = (uint64_t)(uintptr_t)bdev->blk_req;
#endif
    LTRACEF("blk_req structure at %p (0x%lx phys)\n", bdev->blk_req, bdev->blk_req_phys);

#if WITH_KERNEL_VM
    bdev->blk_response_phys = vaddr_to_paddr(&bdev->blk_response);
#else
    bdev->blk_response_phys = (uint64_t)(uintptr_t)&bdev->blk_response;
#endif

    /* make sure the device is reset */
    virtio_reset_device(dev);

    volatile struct virtio_blk_config *config = (struct virtio_blk_config *)dev->config_ptr;

    LTRACEF("capacity 0x%llx\n", config->capacity);
    LTRACEF("size_max 0x%x\n", config->size_max);
    LTRACEF("seg_max  0x%x\n", config->seg_max);
    LTRACEF("blk_size 0x%x\n", config->blk_size);

    /* ack and set the driver status bit */
    virtio_status_acknowledge_driver(dev);

    // XXX check features bits and ack/nak them

    /* allocate a virtio ring */
    virtio_alloc_ring(dev, 0, 256);

    /* set our irq handler */
    dev->irq_driver_callback = &virtio_block_irq_driver_callback;

    /* set DRIVER_OK */
    virtio_status_driver_ok(dev);

    /* construct the block device */
    static uint8_t found_index = 0;
    char buf[16];
    snprintf(buf, sizeof(buf), "virtio%u", found_index++);
    bio_initialize_bdev(&bdev->bdev, buf,
                        config->blk_size, config->capacity,
                        0, NULL, BIO_FLAGS_NONE);

    /* override our block device hooks */
    bdev->bdev.read_block = &virtio_bdev_read_block;
    bdev->bdev.write_block = &virtio_bdev_write_block;

    bio_register_device(&bdev->bdev);

    printf("found virtio block device of size %lld\n", config->capacity * config->blk_size);

    return NO_ERROR;
}

static enum handler_return virtio_block_irq_driver_callback(struct virtio_device *dev, uint ring, const struct vring_used_elem *e)
{
    struct virtio_block_dev *bdev = (struct virtio_block_dev *)dev->priv;

    LTRACEF("dev %p, ring %u, e %p, id %u, len %u\n", dev, ring, e, e->id, e->len);

    /* parse our descriptor chain, add back to the free queue */
    uint16_t i = e->id;
    for (;;) {
        int next;
        struct vring_desc *desc = virtio_desc_index_to_desc(dev, ring, i);

        //virtio_dump_desc(desc);

        if (desc->flags & VRING_DESC_F_NEXT) {
            next = desc->next;
        } else {
            /* end of chain */
            next = -1;
        }

        virtio_free_desc(dev, ring, i);

        if (next < 0)
            break;
        i = next;
    }

    /* signal our event */
    event_signal(&bdev->io_event, false);

    return INT_RESCHEDULE;
}

ssize_t virtio_block_read_write(struct virtio_device *dev, void *buf, off_t offset, size_t len, bool write)
{
    struct virtio_block_dev *bdev = (struct virtio_block_dev *)dev->priv;

    uint16_t i;
    struct vring_desc *desc;
    paddr_t pa;
    vaddr_t va = (vaddr_t)buf;

    LTRACEF("dev %p, buf %p, offset 0x%llx, len %zu\n", dev, buf, offset, len);

    mutex_acquire(&bdev->lock);

    /* set up the request */
    bdev->blk_req->type = write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
    bdev->blk_req->ioprio = 0;
    bdev->blk_req->sector = offset / 512;
    LTRACEF("blk_req type %u ioprio %u sector %llu\n",
            bdev->blk_req->type, bdev->blk_req->ioprio, bdev->blk_req->sector);

    /* put together a transfer */
    desc = virtio_alloc_desc_chain(dev, 0, 3, &i);
    LTRACEF("after alloc chain desc %p, i %u\n", desc, i);

    // XXX not cache safe.
    // At the moment only tested on arm qemu, which doesn't emulate cache.

    /* set up the descriptor pointing to the head */
    desc->addr = bdev->blk_req_phys;
    desc->len = sizeof(struct virtio_blk_req);
    desc->flags |= VRING_DESC_F_NEXT;

    /* set up the descriptor pointing to the buffer */
    desc = virtio_desc_index_to_desc(dev, 0, desc->next);
#if WITH_KERNEL_VM
    /* translate the first buffer */
    pa = vaddr_to_paddr((void *)va);
    desc->addr = (uint64_t)pa;
    /* desc->len is filled in below */
#else
    desc->addr = (uint64_t)(uintptr_t)buf;
    desc->len = len;
#endif
    desc->flags |= write ? 0 : VRING_DESC_F_WRITE; /* mark buffer as write-only if its a block read */
    desc->flags |= VRING_DESC_F_NEXT;

#if WITH_KERNEL_VM
    /* see if we need to add more descriptors due to scatter gather */
    paddr_t next_pa = PAGE_ALIGN(pa + 1);
    desc->len = MIN(next_pa - pa, len);
    LTRACEF("first descriptor va 0x%lx desc->addr 0x%llx desc->len %u\n", va, desc->addr, desc->len);
    len -= desc->len;
    while (len > 0) {
        /* amount of source buffer handled by this iteration of the loop */
        size_t len_tohandle = MIN(len, PAGE_SIZE);

        /* translate the next page in the buffer */
        va = PAGE_ALIGN(va + 1);
        pa = vaddr_to_paddr((void *)va);
        LTRACEF("va now 0x%lx, pa 0x%lx, next_pa 0x%lx, remaining len %zu\n", va, pa, next_pa, len);

        /* is the new translated physical address contiguous to the last one? */
        if (next_pa == pa) {
            LTRACEF("extending last one by %zu bytes\n", len_tohandle);
            desc->len += len_tohandle;
        } else {
            uint16_t next_i = virtio_alloc_desc(dev, 0);
            struct vring_desc *next_desc = virtio_desc_index_to_desc(dev, 0, next_i);
            DEBUG_ASSERT(next_desc);

            LTRACEF("doesn't extend, need new desc, allocated desc %i (%p)\n", next_i, next_desc);

            /* fill this descriptor in and put it after the last one but before the response descriptor */
            next_desc->addr = (uint64_t)pa;
            next_desc->len = len_tohandle;
            next_desc->flags = write ? 0 : VRING_DESC_F_WRITE; /* mark buffer as write-only if its a block read */
            next_desc->flags |= VRING_DESC_F_NEXT;
            next_desc->next = desc->next;
            desc->next = next_i;

            desc = next_desc;
        }
        len -= len_tohandle;
        next_pa += PAGE_SIZE;
    }
#endif

    /* set up the descriptor pointing to the response */
    desc = virtio_desc_index_to_desc(dev, 0, desc->next);
    desc->addr = bdev->blk_response_phys;
    desc->len = 1;
    desc->flags = VRING_DESC_F_WRITE;

    /* submit the transfer */
    virtio_submit_chain(dev, 0, i);

    /* kick it off */
    virtio_kick(dev, 0);

    /* wait for the transfer to complete */
    event_wait(&bdev->io_event);

    LTRACEF("status 0x%hhx\n", bdev->blk_response);

    mutex_release(&bdev->lock);

    return len;
}

static ssize_t virtio_bdev_read_block(struct bdev *bdev, void *buf, bnum_t block, uint count)
{
    struct virtio_block_dev *dev = containerof(bdev, struct virtio_block_dev, bdev);

    LTRACEF("dev %p, buf %p, block 0x%x, count %u\n", bdev, buf, block, count);

    if (virtio_block_read_write(dev->dev, buf, (off_t)block * dev->bdev.block_size,
                                count * dev->bdev.block_size, false) == 0) {
        return count * dev->bdev.block_size;
    } else {
        return ERR_IO;
    }
}

static ssize_t virtio_bdev_write_block(struct bdev *bdev, const void *buf, bnum_t block, uint count)
{
    struct virtio_block_dev *dev = containerof(bdev, struct virtio_block_dev, bdev);

    LTRACEF("dev %p, buf %p, block 0x%x, count %u\n", bdev, buf, block, count);

    if (virtio_block_read_write(dev->dev, (void *)buf, (off_t)block * dev->bdev.block_size,
                                count * dev->bdev.block_size, true) == 0) {
        return count * dev->bdev.block_size;
    } else {
        return ERR_IO;
    }
}

