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
#include <dev/virtio/block.h>

#include <debug.h>
#include <assert.h>
#include <trace.h>
#include <compiler.h>
#include <list.h>
#include <err.h>
#include <kernel/thread.h>
#include <kernel/event.h>
#include <kernel/vm.h>

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

#define VIRTIO_BLK_T_IN         0
#define VIRTIO_BLK_T_OUT        1
#define VIRTIO_BLK_T_FLUSH      4

#define VIRTIO_BLK_S_OK         0
#define VIRTIO_BLK_S_IOERR      1
#define VIRTIO_BLK_S_UNSUPP     2

static enum handler_return virtio_block_irq_driver_callback(struct virtio_device *dev, uint ring, const struct vring_used_elem *e);

static event_t *curr_event;

status_t virtio_block_init(struct virtio_device *dev, uint32_t host_features)
{
    LTRACEF("dev %p, host_features 0x%x\n", dev, host_features);

    volatile struct virtio_blk_config *config = (struct virtio_blk_config *)dev->config_ptr;

    LTRACEF("capacity 0x%llx\n", config->capacity);
    LTRACEF("size_max 0x%x\n", config->size_max);
    LTRACEF("seg_max  0x%x\n", config->seg_max);
    LTRACEF("blk_size 0x%x\n", config->blk_size);

    /* allocate a virtio ring */
    virtio_alloc_ring(dev, 0, 128);

    /* set our irq handler */
    dev->irq_driver_callback = &virtio_block_irq_driver_callback;

    return NO_ERROR;
}

static enum handler_return virtio_block_irq_driver_callback(struct virtio_device *dev, uint ring, const struct vring_used_elem *e)
{
    LTRACEF("dev %p, ring %u, e %p, id %u, len %u\n", dev, ring, e, e->id, e->len);

    /* parse our descriptor chain, add back to the free queue */
    uint16_t i = e->id;
    for (;;) {
        int next;
        struct vring_desc *desc = virtio_desc_index_to_desc(dev, ring, i);

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
    event_signal(curr_event, false);

    return INT_RESCHEDULE;
}

ssize_t virtio_block_read(struct virtio_device *dev, void *buf, off_t offset, size_t len)
{
    uint16_t i;
    struct vring_desc *desc;
    paddr_t pa;

    LTRACEF("dev %p, buf %p, offset 0x%llx, len %zu\n", dev, buf, offset, len);

    /* set up the request */
    struct virtio_blk_req blk_req;
    blk_req.type = VIRTIO_BLK_T_IN;
    blk_req.ioprio = 0;
    blk_req.sector = offset / 512;

    /* put together a transfer */
    desc = virtio_alloc_desc_chain(dev, 0, 3, &i);
    LTRACEF("after alloc chain desc %p, i %u\n", desc, i);

    // XXX not cache safe.
    // At the moment only tested on arm qemu, which doesn't emulate cache.

    /* set up the descriptor pointing to the head */
#if WITH_KERNEL_VM
    // XXX handle bufs that cross page boundaries
    arch_mmu_query((vaddr_t)&blk_req, &pa, NULL);
    desc->addr = (uint64_t)pa;
#else
    desc->addr = (uint64_t)(uintptr_t)&blk_req;
#endif
    desc->len = sizeof(blk_req);
    desc->flags |= VRING_DESC_F_NEXT;
    virtio_dump_desc(desc);

    /* set up the descriptor pointing to the buffer */
    desc = virtio_desc_index_to_desc(dev, 0, desc->next);
#if WITH_KERNEL_VM
    // XXX handle bufs that cross page boundaries
    arch_mmu_query((vaddr_t)buf, &pa, NULL);
    desc->addr = (uint64_t)pa;
#else
    desc->addr = (uint64_t)(uintptr_t)buf;
#endif
    desc->len = len;
    desc->flags |= VRING_DESC_F_NEXT | VRING_DESC_F_WRITE;
    virtio_dump_desc(desc);

    /* set up the descriptor pointing to the response */
    uint8_t blk_response;
    desc = virtio_desc_index_to_desc(dev, 0, desc->next);
#if WITH_KERNEL_VM
    // XXX handle bufs that cross page boundaries
    arch_mmu_query((vaddr_t)&blk_response, &pa, NULL);
    desc->addr = (uint64_t)pa;
#else
    desc->addr = (uint64_t)(uintptr_t)&blk_response;
#endif
    desc->len = 1;
    desc->flags = VRING_DESC_F_WRITE;
    virtio_dump_desc(desc);

    /* set up an event to block on */
    event_t event;
    event_init(&event, false, 0);
    curr_event = &event;

    /* submit the transfer */
    virtio_submit_chain(dev, 0, i);

    /* kick it off */
    virtio_kick(dev, 0);

    /* wait for the transfer to complete */
    event_wait(&event);

    LTRACEF("status 0x%hhx\n", blk_response);

    return len;
}

