/*
 * Copyright (c) 2014 Chris Anderson
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "minip-internal.h"

#include <lk/list.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <lk/err.h>
#include <lk/trace.h>

#define LOCAL_TRACE 0

/* Resolution policy: with a request outstanding, retry every
 * ARP_RETRY_PERIOD ms up to ARP_MAX_ATTEMPTS times, holding at most
 * ARP_MAX_PENDING frames waiting for the answer. On exhaustion the
 * pending frames are dropped; retransmission (TCP) or the caller (UDP)
 * deals with the loss.
 */
#define ARP_MAX_PENDING  3
#define ARP_MAX_ATTEMPTS 3
#define ARP_RETRY_PERIOD 500

typedef struct arp_entry {
    struct list_node node;
    uint32_t addr;
    uint8_t mac[6];
    bool resolved;

    /* unresolved state */
    netif_t *netif;             // interface resolution is happening on
    struct list_node pending;   // fully built frames waiting for the mac
    uint pending_count;
    net_timer_t retry_timer;
    int attempts;
} arp_entry_t;

static struct list_node arp_list = LIST_INITIAL_VALUE(arp_list);
static mutex_t arp_mutex = MUTEX_INITIAL_VALUE(arp_mutex);

void arp_cache_init(void) {}

static void mru_update(struct list_node *entry) {
    if (arp_list.next == entry)
        return;

    list_delete(entry);
    list_add_head(&arp_list, entry);
}

static arp_entry_t *arp_find_locked(uint32_t addr) {
    DEBUG_ASSERT(is_mutex_held(&arp_mutex));

    arp_entry_t *arp;
    list_for_every_entry(&arp_list, arp, arp_entry_t, node) {
        if (arp->addr == addr) {
            return arp;
        }
    }
    return NULL;
}

static arp_entry_t *arp_create_locked(uint32_t addr) {
    DEBUG_ASSERT(is_mutex_held(&arp_mutex));

    arp_entry_t *arp = (arp_entry_t *)calloc(1, sizeof(arp_entry_t));
    if (!arp) {
        return NULL;
    }

    arp->addr = addr;
    list_initialize(&arp->pending);
    list_add_head(&arp_list, &arp->node);
    return arp;
}

/* fill in the destination mac of a parked frame */
static void arp_patch_frame(pktbuf_t *p, const uint8_t *mac) {
    struct eth_hdr *eth = (struct eth_hdr *)p->data;
    mac_addr_copy(eth->dst_mac, mac);
}

void arp_cache_update(uint32_t addr, const uint8_t mac[6]) {
    arp_entry_t *arp;
    ipv4_t ip;

    ip.u = addr;

    // Ignore 0.0.0.0 or x.x.x.255
    if (ip.u == 0 || ip.b[3] == 0xFF) {
        return;
    }

    /* frames that were waiting for this answer, sent outside the lock */
    struct list_node ready = LIST_INITIAL_VALUE(ready);
    netif_t *netif = NULL;

    mutex_acquire(&arp_mutex);

    arp = arp_find_locked(addr);
    if (!arp) {
        LTRACEF("Adding %u.%u.%u.%u -> %02x:%02x:%02x:%02x:%02x:%02x to cache\n",
                ip.b[0], ip.b[1], ip.b[2], ip.b[3],
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        arp = arp_create_locked(addr);
        if (!arp) {
            goto out;
        }
    }

    mac_addr_copy(arp->mac, mac);
    mru_update(&arp->node);

    if (!arp->resolved) {
        arp->resolved = true;
        net_timer_cancel(&arp->retry_timer);

        /* release any frames parked on the entry */
        netif = arp->netif;
        pktbuf_t *p;
        while ((p = list_remove_head_type(&arp->pending, pktbuf_t, list)) != NULL) {
            arp_patch_frame(p, arp->mac);
            list_add_tail(&ready, &p->list);
        }
        arp->pending_count = 0;
    }

out:
    mutex_release(&arp_mutex);

    pktbuf_t *p;
    while ((p = list_remove_head_type(&ready, pktbuf_t, list)) != NULL) {
        DEBUG_ASSERT(netif);
        netif_tx(netif, p);
    }
}

/* Looks up and returns a MAC address based on the provided ip addr */
uint8_t *arp_cache_lookup(uint32_t addr) {
    arp_entry_t *arp;
    uint8_t *ret = NULL;

    mutex_acquire(&arp_mutex);
    arp = arp_find_locked(addr);
    if (arp && arp->resolved) {
        mru_update(&arp->node);
        ret = arp->mac;
    }
    mutex_release(&arp_mutex);

    return ret;
}

void arp_cache_dump(void) {
    int i = 0;
    arp_entry_t *arp;

    mutex_acquire(&arp_mutex);
    if (!list_is_empty(&arp_list)) {
        list_for_every_entry(&arp_list, arp, arp_entry_t, node) {
            ipv4_t ip;
            ip.u = arp->addr;
            if (arp->resolved) {
                printf("%2d: %u.%u.%u.%u -> %02x:%02x:%02x:%02x:%02x:%02x\n",
                       i++, ip.b[0], ip.b[1], ip.b[2], ip.b[3],
                       arp->mac[0], arp->mac[1], arp->mac[2], arp->mac[3], arp->mac[4], arp->mac[5]);
            } else {
                printf("%2d: %u.%u.%u.%u -> (incomplete, %u pending, attempt %d)\n",
                       i++, ip.b[0], ip.b[1], ip.b[2], ip.b[3],
                       arp->pending_count, arp->attempts);
            }
        }
    } else {
        printf("The arp table is empty\n");
    }
    mutex_release(&arp_mutex);
}

int arp_send_request(netif_t *netif, ipv4_addr_t addr) {
    pktbuf_t *p;
    struct eth_hdr *eth;
    struct arp_pkt *arp;

    if ((p = pktbuf_alloc()) == NULL) {
        return -1;
    }

    eth = (struct eth_hdr *)pktbuf_prepend(p, sizeof(struct eth_hdr));
    arp = (struct arp_pkt *)pktbuf_append(p, sizeof(struct arp_pkt));
    minip_build_mac_hdr(netif, eth, bcast_mac, ETH_TYPE_ARP);

    arp->htype = htons(0x0001);
    arp->ptype = htons(0x0800);
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = htons(ARP_OPER_REQUEST);
    arp->spa = netif->ipv4_addr;
    arp->tpa = addr;
    mac_addr_copy(arp->sha, netif->mac_address);
    mac_addr_copy(arp->tha, bcast_mac);

    if (netif->tx_func) {
        netif_tx(netif, p);
    } else {
        pktbuf_free(p, true);
    }
    return 0;
}

static void arp_retry_timer_cb(void *arg) {
    arp_entry_t *arp = (arp_entry_t *)arg;

    /* frames to drop on resolution failure, freed outside the lock */
    struct list_node dropped = LIST_INITIAL_VALUE(dropped);
    netif_t *netif = NULL;
    ipv4_addr_t addr = 0;

    mutex_acquire(&arp_mutex);

    /* entries are never freed, so the pointer is stable */
    if (!arp->resolved) {
        if (++arp->attempts >= ARP_MAX_ATTEMPTS) {
            /* give up; drop the parked frames */
            LTRACEF("giving up resolving 0x%x, dropping %u frames\n", arp->addr, arp->pending_count);
            pktbuf_t *pend;
            while ((pend = list_remove_head_type(&arp->pending, pktbuf_t, list)) != NULL) {
                list_add_tail(&dropped, &pend->list);
            }
            arp->pending_count = 0;
        } else {
            netif = arp->netif;
            addr = arp->addr;
            net_timer_set(&arp->retry_timer, &arp_retry_timer_cb, arp, ARP_RETRY_PERIOD);
        }
    }

    mutex_release(&arp_mutex);

    pktbuf_t *p;
    while ((p = list_remove_head_type(&dropped, pktbuf_t, list)) != NULL) {
        pktbuf_free(p, true);
    }

    if (netif) {
        arp_send_request(netif, addr);
    }
}

/* Send a fully built ethernet frame whose destination mac is not yet known:
 * if the address is already resolved, patch the frame and transmit it now;
 * otherwise park it on the (possibly new) cache entry and kick resolution.
 * Takes ownership of the frame in all cases.
 */
status_t arp_send_or_queue(netif_t *netif, ipv4_addr_t addr, pktbuf_t *p) {
    DEBUG_ASSERT(netif);
    DEBUG_ASSERT(p);

    bool send_request = false;
    status_t ret = NO_ERROR;

    mutex_acquire(&arp_mutex);

    arp_entry_t *arp = arp_find_locked(addr);
    if (arp && arp->resolved) {
        mru_update(&arp->node);
        arp_patch_frame(p, arp->mac);
        mutex_release(&arp_mutex);

        return netif_tx(netif, p);
    }

    if (!arp) {
        arp = arp_create_locked(addr);
        if (!arp) {
            ret = ERR_NO_MEMORY;
            goto drop;
        }
    }

    if (arp->pending_count >= ARP_MAX_PENDING) {
        ret = ERR_NO_RESOURCES;
        goto drop;
    }

    list_add_tail(&arp->pending, &p->list);
    arp->pending_count++;
    arp->netif = netif;

    /* (re)start resolution if there isn't already a retry timer running */
    if (net_timer_set(&arp->retry_timer, &arp_retry_timer_cb, arp, ARP_RETRY_PERIOD)) {
        arp->attempts = 0;
        send_request = true;
    }

    mutex_release(&arp_mutex);

    if (send_request) {
        arp_send_request(netif, addr);
    }
    return NO_ERROR;

drop:
    mutex_release(&arp_mutex);
    pktbuf_free(p, true);
    return ret;
}

/* Blocking resolution for callers on their own thread (console tools).
 * Polls the cache while resolution runs in the background.
 */
const uint8_t *arp_get_dest_mac(uint32_t host) {
    if (host == IPV4_BCAST) {
        return bcast_mac;
    }

    const uint8_t *dst_mac = arp_cache_lookup(host);
    if (dst_mac == NULL) {
        ipv4_route_t *route = ipv4_search_route(host);
        if (!route) {
            return NULL;
        }
        netif_t *netif = route->interface;

        arp_send_request(netif, host);

        for (lk_time_t waited = 0; waited < ARP_MAX_ATTEMPTS * ARP_RETRY_PERIOD; waited += 10) {
            dst_mac = arp_cache_lookup(host);
            if (dst_mac) {
                break;
            }
            thread_sleep(10);
        }
        ipv4_dec_route_ref(route);
    }

    return dst_mac;
}

int handle_arp_pkt(netif_t *netif, pktbuf_t *p) {
    struct eth_hdr *eth;
    struct arp_pkt *arp;

    LTRACEF("ARP packet, len %u\n", p->dlen);

    eth = (struct eth_hdr *)(p->data - sizeof(struct eth_hdr));

    if ((arp = (struct arp_pkt *)pktbuf_consume(p, sizeof(struct arp_pkt))) == NULL) {
        return -1;
    }

    switch (ntohs(arp->oper)) {
        case ARP_OPER_REQUEST: {
            pktbuf_t *rp;
            struct eth_hdr *reth;
            struct arp_pkt *rarp;

            if (memcmp(&arp->tpa, &netif->ipv4_addr, sizeof(netif->ipv4_addr)) == 0) {
                if ((rp = pktbuf_alloc()) == NULL) {
                    break;
                }

                LTRACEF("arp request for us\n");

                reth = (struct eth_hdr *)pktbuf_prepend(rp, sizeof(struct eth_hdr));
                rarp = (struct arp_pkt *)pktbuf_append(rp, sizeof(struct arp_pkt));

                // Eth header
                minip_build_mac_hdr(netif, reth, eth->src_mac, ETH_TYPE_ARP);

                // ARP packet
                rarp->oper = htons(ARP_OPER_REPLY);
                rarp->htype = htons(0x0001);
                rarp->ptype = htons(0x0800);
                rarp->hlen = 6;
                rarp->plen = 4;
                mac_addr_copy(rarp->sha, netif->mac_address);
                rarp->spa = netif->ipv4_addr;
                mac_addr_copy(rarp->tha, arp->sha);
                rarp->tpa = arp->spa;

                netif_tx(netif, rp);
            }
        }
        break;

        case ARP_OPER_REPLY: {
            LTRACEF("arp reply for us\n");

            uint32_t addr;
            memcpy(&addr, &arp->spa, sizeof(addr)); // unaligned word
            arp_cache_update(addr, arp->sha);
        }
        break;
    }

    return 0;
}
