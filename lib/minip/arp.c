/*
 * Copyright (c) 2014 Chris Anderson
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
#include <lk/trace.h>

#define LOCAL_TRACE 1
typedef struct {
    struct list_node node;
    uint32_t addr;
    uint8_t mac[6];
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

void arp_cache_update(uint32_t addr, const uint8_t mac[6]) {
    arp_entry_t *arp;
    ipv4_t ip;
    bool found = false;

    ip.u = addr;

    // Ignore 0.0.0.0 or x.x.x.255
    if (ip.u == 0 || ip.b[3] == 0xFF) {
        return;
    }

    /* If the entry is in the cache update the address and move
     * it to head */
    mutex_acquire(&arp_mutex);
    list_for_every_entry(&arp_list, arp, arp_entry_t, node) {
        if (arp->addr == addr) {
            arp->addr = addr;
            mru_update(&arp->node);
            found = true;
            break;
        }
    }

    if (!found) {
        LTRACEF("Adding %u.%u.%u.%u -> %02x:%02x:%02x:%02x:%02x:%02x to cache\n",
                ip.b[0], ip.b[1], ip.b[2], ip.b[3],
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        arp = malloc(sizeof(arp_entry_t));
        if (arp == NULL) {
            goto err;
        }

        arp->addr = addr;
        mac_addr_copy(arp->mac, mac);
        list_add_head(&arp_list, &arp->node);
    }

err:
    mutex_release(&arp_mutex);
    return;
}

/* Looks up and returns a MAC address based on the provided ip addr */
uint8_t *arp_cache_lookup(uint32_t addr) {
    arp_entry_t *arp = NULL;
    uint8_t *ret = NULL;

    /* If the entry is in the cache update the address and move
     * it to head */
    mutex_acquire(&arp_mutex);
    list_for_every_entry(&arp_list, arp, arp_entry_t, node) {
        if (arp->addr == addr) {
            mru_update(&arp->node);
            ret = arp->mac;
            break;
        }
    }
    mutex_release(&arp_mutex);

    return ret;
}

void arp_cache_dump(void) {
    int i = 0;
    arp_entry_t *arp;

    if (!list_is_empty(&arp_list)) {
        list_for_every_entry(&arp_list, arp, arp_entry_t, node) {
            ipv4_t ip;
            ip.u = arp->addr;
            printf("%2d: %u.%u.%u.%u -> %02x:%02x:%02x:%02x:%02x:%02x\n",
                   i++, ip.b[0], ip.b[1], ip.b[2], ip.b[3],
                   arp->mac[0], arp->mac[1], arp->mac[2], arp->mac[3], arp->mac[4], arp->mac[5]);
        }
    } else {
        printf("The arp table is empty\n");
    }
}

int arp_send_request(netif_t *netif, ipv4_addr_t addr) {
    pktbuf_t *p;
    struct eth_hdr *eth;
    struct arp_pkt *arp;

    if ((p = pktbuf_alloc()) == NULL) {
        return -1;
    }

    eth = pktbuf_prepend(p, sizeof(struct eth_hdr));
    arp = pktbuf_append(p, sizeof(struct arp_pkt));
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
        netif->tx_func(netif->tx_func_arg, p);
    }
    return 0;
}

static void handle_arp_timeout_cb(void *arg) {
    *(bool *)arg = true;
}

const uint8_t *arp_get_dest_mac(uint32_t host) {
    const uint8_t *dst_mac = NULL;
    bool arp_timeout = false;
    net_timer_t arp_timeout_timer;

    if (host == IPV4_BCAST) {
        return bcast_mac;
    }

    dst_mac = arp_cache_lookup(host);
    if (dst_mac == NULL) {
        // TODO: lookup netif based on route
        netif_t *netif = netif_main;
        arp_send_request(netif, host);
        memset(&arp_timeout_timer, 0, sizeof(arp_timeout_timer));
        net_timer_set(&arp_timeout_timer, handle_arp_timeout_cb, &arp_timeout, 100);
        while (!arp_timeout) {
            dst_mac = arp_cache_lookup(host);
            if (dst_mac) {
                net_timer_cancel(&arp_timeout_timer);
                break;
            }
        }
    }

    return dst_mac;
}

int handle_arp_pkt(netif_t *netif, pktbuf_t *p) {
    struct eth_hdr *eth;
    struct arp_pkt *arp;

    eth = (void *) (p->data - sizeof(struct eth_hdr));

    if ((arp = pktbuf_consume(p, sizeof(struct arp_pkt))) == NULL) {
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

                reth = pktbuf_prepend(rp, sizeof(struct eth_hdr));
                rarp = pktbuf_append(rp, sizeof(struct arp_pkt));

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

                netif->tx_func(netif->tx_func_arg, rp);
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


