/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "testnetif.h"

#include <kernel/mutex.h>
#include <string.h>
#include <stdlib.h>
#include <lk/err.h>

#include "../minip-internal.h"

static netif_t testnetif;
static bool testnetif_registered;
static mutex_t tn_lock = MUTEX_INITIAL_VALUE(tn_lock);

static const uint8_t tn_mac[6] = { 0x02, 0x00, 0x00, 0x99, 0x00, 0x01 };
static const uint8_t tn_peer_mac[6] = { 0x02, 0x00, 0x00, 0x99, 0x00, 0x02 };

static testnetif_faults_t tn_faults;
static uint tn_tx_count;
static uint tn_dropped_count;

static struct {
    uint8_t data[TESTNETIF_CAPTURE_BYTES];
    size_t len;
} tn_capture[TESTNETIF_CAPTURE_FRAMES];
static uint tn_capture_next;

/* one frame held back for pairwise reordering */
static uint8_t tn_held[PKTBUF_SIZE];
static size_t tn_held_len;

const uint8_t *testnetif_peer_mac(void) {
    return tn_peer_mac;
}

static int testnetif_tx(void *arg, pktbuf_t *p) {
    mutex_acquire(&tn_lock);

    tn_tx_count++;

    /* capture the head of the frame */
    size_t clen = (p->dlen < TESTNETIF_CAPTURE_BYTES) ? p->dlen : TESTNETIF_CAPTURE_BYTES;
    memcpy(tn_capture[tn_capture_next % TESTNETIF_CAPTURE_FRAMES].data, p->data, clen);
    tn_capture[tn_capture_next % TESTNETIF_CAPTURE_FRAMES].len = p->dlen;
    tn_capture_next++;

    bool drop = tn_faults.drop_every && (tn_tx_count % tn_faults.drop_every) == 0;

    /* a one shot drop of the first frame over a given size, which is how a
     * test singles out a data segment without parsing the frame */
    if (tn_faults.drop_once_min_len && p->dlen >= tn_faults.drop_once_min_len) {
        tn_faults.drop_once_min_len = 0;
        drop = true;
    }
    const bool dup = tn_faults.dup_every && (tn_tx_count % tn_faults.dup_every) == 0;
    const bool reorder = tn_faults.reorder_pairs;

    if (drop) {
        tn_dropped_count++;
        mutex_release(&tn_lock);
        pktbuf_free(p, true);
        return 0;
    }

    if (reorder && tn_held_len == 0 && p->dlen <= sizeof(tn_held)) {
        /* hold this frame back; it goes out after the next one */
        memcpy(tn_held, p->data, p->dlen);
        tn_held_len = p->dlen;
        mutex_release(&tn_lock);
        pktbuf_free(p, true);
        return 0;
    }

    /* take a copy of any held frame to release after this one */
    uint8_t *release = NULL;
    size_t release_len = 0;
    if (tn_held_len) {
        release = malloc(tn_held_len);
        if (release) {
            memcpy(release, tn_held, tn_held_len);
            release_len = tn_held_len;
        }
        tn_held_len = 0;
    }

    mutex_release(&tn_lock);

    minip_rx_driver_callback_copy(&testnetif, p->data, p->dlen);
    if (dup) {
        minip_rx_driver_callback_copy(&testnetif, p->data, p->dlen);
    }
    if (release) {
        minip_rx_driver_callback_copy(&testnetif, release, release_len);
        free(release);
    }

    pktbuf_free(p, true);
    return 0;
}

netif_t *testnetif_get(void) {
    mutex_acquire(&tn_lock);
    bool need_register = !testnetif_registered;
    if (need_register) {
        testnetif_registered = true;
    }
    mutex_release(&tn_lock);

    if (need_register) {
        netif_create(&testnetif, "testnetif");
        netif_set_eth(&testnetif, &testnetif_tx, NULL, tn_mac);
        /* configure the address before registering so dhcp doesn't start */
        netif_set_ipv4_addr(&testnetif, IPV4(10, 99, 0, 1), 24);
        netif_register(&testnetif);

        /* reaching our own address should not need a resolution round trip */
        arp_cache_update(IPV4(10, 99, 0, 1), tn_mac);
    }

    return &testnetif;
}

void testnetif_configure(const testnetif_faults_t *faults) {
    mutex_acquire(&tn_lock);
    if (faults) {
        tn_faults = *faults;
    } else {
        memset(&tn_faults, 0, sizeof(tn_faults));
    }
    tn_tx_count = 0;
    tn_dropped_count = 0;
    tn_capture_next = 0;
    tn_held_len = 0;
    memset(tn_capture, 0, sizeof(tn_capture));
    mutex_release(&tn_lock);
}

uint testnetif_tx_count(void) {
    mutex_acquire(&tn_lock);
    uint ret = tn_tx_count;
    mutex_release(&tn_lock);
    return ret;
}

uint testnetif_dropped_count(void) {
    mutex_acquire(&tn_lock);
    uint ret = tn_dropped_count;
    mutex_release(&tn_lock);
    return ret;
}

size_t testnetif_capture_get(uint idx, uint8_t *buf, size_t buflen) {
    size_t ret = 0;

    mutex_acquire(&tn_lock);
    if (idx < TESTNETIF_CAPTURE_FRAMES && idx < tn_capture_next) {
        uint slot = (tn_capture_next - 1 - idx) % TESTNETIF_CAPTURE_FRAMES;
        if (tn_capture[slot].len) {
            size_t copy = tn_capture[slot].len;
            if (copy > TESTNETIF_CAPTURE_BYTES) copy = TESTNETIF_CAPTURE_BYTES;
            if (copy > buflen) copy = buflen;
            memcpy(buf, tn_capture[slot].data, copy);
            ret = tn_capture[slot].len;
        }
    }
    mutex_release(&tn_lock);

    return ret;
}

status_t testnetif_inject(const void *frame, size_t len) {
    return minip_rx_driver_callback_copy(testnetif_get(), frame, len);
}
