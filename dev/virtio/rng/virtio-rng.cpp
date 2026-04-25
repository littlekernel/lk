/*
 * Copyright (c) 2026 Kuan-Wei Chiu <visitorckw@gmail.com>
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <dev/virtio/rng.h>
#include <dev/virtio/virtio-device.h>
#include <dev/virtio/virtio_ring.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/trace.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

constexpr uint16_t RNG_QUEUE_INDEX = 0;

struct virtio_rng_state {
    virtio_device *dev;
    bool initialized;
    volatile size_t last_rx_len;
};

static virtio_rng_state rng;

static enum handler_return virtio_rng_irq(virtio_device *dev, uint ring_index, const vring_used_elem *e) {
    if (ring_index == RNG_QUEUE_INDEX && e)
        rng.last_rx_len = e->len;

    return INT_NO_RESCHEDULE;
}

status_t virtio_rng_init(virtio_device *dev) {
    LTRACE_ENTRY;

    rng.dev = dev;
    rng.initialized = false;
    rng.last_rx_len = 0;

    dev->set_irq_callbacks(virtio_rng_irq, nullptr);

    status_t err = dev->virtio_alloc_ring(RNG_QUEUE_INDEX, 32);
    if (err != NO_ERROR) {
        TRACEF("virtio-rng: Failed to allocate virtqueue\n");
        return err;
    }

    dev->bus()->virtio_status_driver_ok();
    rng.initialized = true;

    LTRACEF("virtio-rng: initialized successfully\n");
    return NO_ERROR;
}

ssize_t virtio_rng_read(void *buf, size_t len) {
    if (!rng.initialized || len == 0) return ERR_NOT_CONFIGURED;

    vaddr_t v_start = (vaddr_t)buf;
    vaddr_t v_end = v_start + len - 1;
    if ((v_start / PAGE_SIZE) != (v_end / PAGE_SIZE))
        return ERR_INVALID_ARGS;

    paddr_t paddr = vaddr_to_paddr(buf);
    if (!paddr) return ERR_INVALID_ARGS;

    uint16_t desc_idx = rng.dev->virtio_alloc_desc(RNG_QUEUE_INDEX);
    if (desc_idx == 0xffff) return ERR_NO_MEMORY;

    vring_desc *desc = rng.dev->virtio_desc_index_to_desc(RNG_QUEUE_INDEX, desc_idx);
    desc->addr = paddr;
    desc->len = (uint32_t)len;
    desc->flags = VRING_DESC_F_WRITE;

    rng.last_rx_len = 0;
    rng.dev->virtio_submit_chain(RNG_QUEUE_INDEX, desc_idx);
    rng.dev->bus()->virtio_kick(RNG_QUEUE_INDEX);

    while (rng.last_rx_len == 0) {
        rng.dev->handle_queue_interrupt();
        thread_yield();
    }

    size_t rx_len = rng.last_rx_len;
    rng.last_rx_len = 0;
    
    rng.dev->virtio_free_desc(RNG_QUEUE_INDEX, desc_idx);
    return rx_len; 
}

static void seed_system_prng(uint level) {
    unsigned int seed = 0;
    ssize_t bytes_read;

    if (!rng.initialized)
        return;

    bytes_read = virtio_rng_read(&seed, sizeof(seed));
    
    if (bytes_read == sizeof(seed)) {
        srand(seed);
        dprintf(INFO, "virtio-rng: System PRNG seeded with hardware entropy\n");
    } else {
        dprintf(INFO, "virtio-rng: Failed to seed system PRNG\n");
    }
}

LK_INIT_HOOK(virtio_rng_seed, seed_system_prng, LK_INIT_LEVEL_TARGET + 1);
