/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/minip.h>
#include <lib/minip/netif.h>
#include <sys/types.h>

/* A scriptable network interface for the minip unit tests: everything
 * transmitted on it runs through a programmable fault pipeline (drop,
 * duplicate, pairwise reorder) and is then reflected back into the stack,
 * so both endpoints of a connection live in this kernel -- like loopback,
 * but with faults in the middle and a capture ring for assertions.
 * It lives at 10.99.0.1/24; its own address needs no ARP (pre-seeded),
 * other 10.99.0.x addresses exercise the ARP path.
 */

typedef struct testnetif_faults {
    uint drop_every;     // drop every Nth transmitted frame (0 = never)
    uint dup_every;      // deliver every Nth frame twice (0 = never)
    bool reorder_pairs;  // swap the delivery order of each frame pair
    size_t drop_once_min_len; // drop the first frame at least this long, once
} testnetif_faults_t;

#define TESTNETIF_CAPTURE_FRAMES 8
#define TESTNETIF_CAPTURE_BYTES  64

// lazily creates and registers the interface; always returns it
netif_t *testnetif_get(void);

// the mac address the far end of injected frames uses
const uint8_t *testnetif_peer_mac(void);

// set/clear the fault pipeline (NULL clears) and zero the counters
void testnetif_configure(const testnetif_faults_t *faults);

uint testnetif_tx_count(void);
uint testnetif_dropped_count(void);

// copy out the beginning of the idx-th most recent transmitted frame
// (0 = most recent); returns its full length or 0 if not captured
size_t testnetif_capture_get(uint idx, uint8_t *buf, size_t buflen);

// inject a raw ethernet frame into the stack as if received on this netif
status_t testnetif_inject(const void *frame, size_t len);
