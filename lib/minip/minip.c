/*
 * Copyright (c) 2014 Chris Anderson
 * Copyright (c) 2014 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "minip-internal.h"

#include <assert.h>
#include <lk/err.h>
#include <stdio.h>
#include <lk/debug.h>
#include <endian.h>
#include <errno.h>
#include <iovec.h>
#include <stdlib.h>
#include <string.h>
#include <lk/trace.h>
#include <malloc.h>
#include <arch/atomic.h>
#include <lk/list.h>
#include <lk/init.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>

// TODO
// 1. Tear endian code out into something that flips words before/after tx/rx calls

#define LOCAL_TRACE 0
static ipv4_addr_t minip_gateway = IPV4_NONE;

static char minip_hostname[32] = "";

static volatile bool minip_configured = false;
static event_t minip_configured_event = EVENT_INITIAL_VALUE(minip_configured_event, false, 0);

/* routing table */
static mutex_t route_lock = MUTEX_INITIAL_VALUE(route_lock);
static ipv4_route_t route_table[16];

/* if all the important configuration bits are set, signal that we're configured */
static void check_and_set_configured(void) {
    // search for an ip route other than loopback to signal configured
    mutex_acquire(&route_lock);

    bool found = false;
    for (size_t i = 0; i < countof(route_table); i++) {
        if (route_table[i].flags & IPV4_ROUTE_UP) {
            if ((route_table[i].interface->flags & NETIF_FLAG_LOOPBACK) == 0) {
                found = true;
                break;
            }
        }
    }

    mutex_release(&route_lock);
    if (!found) return;

    // we're configured
    printf("MINIP: setting configured state\n");
    minip_set_configured();
}

void minip_set_hostname(const char *name) {
    strlcpy(minip_hostname, name, sizeof(minip_hostname));
    check_and_set_configured();
}

const char *minip_get_hostname(void) {
    return minip_hostname;
}

uint32_t minip_get_gateway(void) {
    return minip_gateway;
}

void minip_set_gateway(const ipv4_addr_t addr) {
    minip_gateway = addr;

    // look up the route to this and set a default route
    ipv4_route_t *route = ipv4_search_route(addr);
    if (route) {
        ipv4_add_default_route(addr, route->interface);
        ipv4_dec_route_ref(route);
    }

    check_and_set_configured();
}

void minip_set_configured(void) {
    minip_configured = true;
    event_signal(&minip_configured_event, true);
}

bool minip_is_configured(void) {
    return minip_configured;
}

status_t minip_wait_for_configured(lk_time_t timeout) {
    return event_wait_timeout(&minip_configured_event, timeout);
}

void gen_random_mac_address(uint8_t *mac_addr) {
    for (size_t i = 0; i < 6; i++) {
        mac_addr[i] = rand() & 0xff;
    }
    /* unicast and locally administered */
    mac_addr[0] &= ~(1<<0);
    mac_addr[0] |= (1<<1);
}

void minip_start_static(uint32_t ip, uint32_t mask, uint32_t gateway) {
    PANIC_UNIMPLEMENTED;
    minip_set_gateway(gateway);
}

// search for an ipv4 route given the destination
// bumps the ref of the route upon return
ipv4_route_t *ipv4_search_route(ipv4_addr_t dest) {
    ipv4_route_t *r = NULL;
    ipv4_route_t *def_route = NULL;

    mutex_acquire(&route_lock);

    for (size_t i = 0; i < countof(route_table); i++) {
        ipv4_route_t *rtemp = &route_table[i];
        if (rtemp->flags & IPV4_ROUTE_UP) {
            if (rtemp->flags & IPV4_ROUTE_DEFAULT) {
                def_route = rtemp;
            } else {
                // does this route satisfy our destination
                if ((dest & rtemp->mask) == rtemp->dest) {
                    r = rtemp;
                    break;
                }
            }
        }
    }

    if (!r && def_route) {
        r = def_route;
    }
    if (r) {
        // inc ref on the route and pass it back
        r->ref++;
    }

    mutex_release(&route_lock);

    return r;
}

void ipv4_dec_route_ref(ipv4_route_t *r) {
    DEBUG_ASSERT(r);

    mutex_acquire(&route_lock);

    DEBUG_ASSERT(r->ref >= 1);
    r->ref--;

    mutex_release(&route_lock);
}

status_t ipv4_add_route(ipv4_addr_t dest, ipv4_addr_t mask, netif_t *n) {
    LTRACE_ENTRY;
    DEBUG_ASSERT(n);

    mutex_acquire(&route_lock);

    // find an unused slot
    status_t err;
    for (size_t i = 0; i < countof(route_table); i++) {
        ipv4_route_t *r = &route_table[i];

        if (r->ref == 0 && r->flags == 0) {
            // unused route, use this slot
            r->dest = dest;
            r->mask = mask;
            r->interface = n;
            r->ref = 0;
            r->flags |= IPV4_ROUTE_UP;
            err = NO_ERROR;
            goto done;
        }
    }

    err = ERR_NO_MEMORY;

done:
    mutex_release(&route_lock);

    return err;
}

status_t ipv4_add_default_route(ipv4_addr_t dest, netif_t *n) {
    LTRACE_ENTRY;
    DEBUG_ASSERT(n);

    mutex_acquire(&route_lock);

    // find an unused slot
    status_t err;
    for (size_t i = 0; i < countof(route_table); i++) {
        ipv4_route_t *r = &route_table[i];

        if (r->ref == 0 && r->flags == 0) {
            // unused route, use this slot
            r->dest = dest;
            r->mask = 0;
            r->interface = n;
            r->ref = 0;
            r->flags |= IPV4_ROUTE_UP;
            r->flags |= IPV4_ROUTE_DEFAULT;
            err = NO_ERROR;
            goto done;
        }
    }

    err = ERR_NO_MEMORY;

done:
    mutex_release(&route_lock);

    return err;
}

void dump_ipv4_route_table(void) {
    mutex_acquire(&route_lock);

    for (size_t i = 0; i < countof(route_table); i++) {
        ipv4_route_t *r = &route_table[i];
        if (r->ref > 0 || r->flags != 0) {
            printf("route dest ");
            print_ipv4_address(r->dest);
            printf(" mask ");
            print_ipv4_address(r->mask);
            printf(" flags %c%c ref %d interface %s\n",
                    r->flags & IPV4_ROUTE_UP ? 'U' : ' ',
                    r->flags & IPV4_ROUTE_DEFAULT ? 'D' : ' ',
                    r->ref,
                    r->interface->name);
        }
    }

    mutex_release(&route_lock);
}

// ipv4 send/receive logic
static uint16_t ipv4_payload_len(struct ipv4_hdr *pkt) {
    return (pkt->len - ((pkt->ver_ihl >> 4) * 5));
}

void minip_build_mac_hdr(netif_t *netif, struct eth_hdr *pkt, const uint8_t *dst, uint16_t type) {
    mac_addr_copy(pkt->dst_mac, dst);
    mac_addr_copy(pkt->src_mac, netif->mac_address);
    pkt->type = htons(type);
}

static void minip_build_ipv4_hdr(netif_t *netif, struct ipv4_hdr *ipv4, ipv4_addr_t dst, uint8_t proto, uint16_t len) {
    ipv4->ver_ihl       = 0x45;
    ipv4->dscp_ecn      = 0;
    ipv4->len           = htons(20 + len); // 5 * 4 from ihl, plus payload length
    ipv4->id            = 0;
    ipv4->flags_frags   = 0x40; // no offset, no fragments
    ipv4->ttl           = 64;
    ipv4->proto         = proto;
    ipv4->dst_addr      = dst;
    ipv4->src_addr      = netif->ipv4_addr;

    /* This may be unnecessary if the controller supports checksum offloading */
    ipv4->chksum = 0;
    ipv4->chksum = ~ones_sum16(0, (uint8_t *) ipv4, sizeof(struct ipv4_hdr));
}

status_t minip_ipv4_send_raw(pktbuf_t *p, ipv4_addr_t dest_addr, uint8_t proto, const uint8_t *dest_mac, netif_t *netif) {
    DEBUG_ASSERT(p);
    DEBUG_ASSERT(netif);

    size_t data_len = p->dlen;

    struct ipv4_hdr *ip = pktbuf_prepend(p, sizeof(struct ipv4_hdr));
    struct eth_hdr *eth = pktbuf_prepend(p, sizeof(struct eth_hdr));

    if (LOCAL_TRACE) {
        printf("sending ipv4\n");
    }

    minip_build_mac_hdr(netif, eth, dest_mac, ETH_TYPE_IPV4);
    minip_build_ipv4_hdr(netif, ip, dest_addr, proto, data_len);

    return netif->tx_func(netif->tx_func_arg, p);
}

status_t minip_ipv4_send(pktbuf_t *p, ipv4_addr_t dest_addr, uint8_t proto) {
    status_t ret = 0;

    // TODO: cache route at socket creation
    ipv4_route_t *route = ipv4_search_route(dest_addr);
    if (!route) {
        ret = -EHOSTUNREACH;
        goto err;
    }
    DEBUG_ASSERT(route->interface);
    netif_t *netif = route->interface;

    // are we sending a broadcast packet?
    const uint8_t *dest_mac;
    if (dest_addr == IPV4_BCAST || dest_addr == netif_get_broadcast_ipv4(netif)) {
        dest_mac = bcast_mac;
        goto ready;
    }

    // is this a local subnet packet or do we need to send to the router?
    ipv4_addr_t target_addr = dest_addr;
    ipv4_addr_t netmask = netif_get_netmask_ipv4(netif);
    if ((dest_addr & netmask) != (netif->ipv4_addr & netmask)) {
        // need to use the gateway
        if (minip_gateway == IPV4_NONE) {
            ret = ERR_NOT_FOUND; // TODO: better error code
            goto err;
        }

        target_addr = minip_gateway;
    }

    dest_mac = arp_get_dest_mac(target_addr);
    if (!dest_mac) {
        pktbuf_free(p, true);
        ret = -EHOSTUNREACH;
        goto err;
    }

ready:
    ret = minip_ipv4_send_raw(p, dest_addr, proto, dest_mac, netif);

err:
    if (route) {
        ipv4_dec_route_ref(route);
    }
    return ret;
}

/* Swap the dst/src ip addresses and send an ICMP ECHO REPLY with the same payload.
 * According to spec the data portion doesn't matter, but ping itself validates that
 * the payload is identical
 */
static void send_ping_reply(netif_t *netif, uint32_t ipaddr, struct icmp_pkt *req, size_t reqdatalen) {
    pktbuf_t *p;
    size_t len;
    struct eth_hdr *eth;
    struct ipv4_hdr *ip;
    struct icmp_pkt *icmp;

    if ((p = pktbuf_alloc()) == NULL) {
        return;
    }

    icmp = pktbuf_prepend(p, sizeof(struct icmp_pkt));
    ip = pktbuf_prepend(p, sizeof(struct ipv4_hdr));
    eth = pktbuf_prepend(p, sizeof(struct eth_hdr));
    pktbuf_append_data(p, req->data, reqdatalen);

    len = sizeof(struct icmp_pkt) + reqdatalen;

    minip_build_mac_hdr(netif, eth, arp_cache_lookup(ipaddr), ETH_TYPE_IPV4);
    minip_build_ipv4_hdr(netif, ip, ipaddr, IP_PROTO_ICMP, len);

    icmp->type = ICMP_ECHO_REPLY;
    icmp->code = 0;
    memcpy(icmp->hdr_data, req->hdr_data, sizeof(icmp->hdr_data));
    icmp->chksum = 0;
    icmp->chksum = ~ones_sum16(0, (uint8_t *) icmp, len);

    netif->tx_func(netif->tx_func_arg, p);
}

__NO_INLINE static void dump_ipv4_packet(const struct ipv4_hdr *ip) {
    printf("IP ");
    print_ipv4_address(ip->src_addr);
    printf(" -> ");
    print_ipv4_address(ip->dst_addr);
    printf(" hlen 0x%x, prot 0x%x, cksum 0x%x, len 0x%x, ident 0x%x, frag offset 0x%x\n",
           (ip->ver_ihl & 0xf) * 4, ip->proto, ntohs(ip->chksum), ntohs(ip->len), ntohs(ip->id), ntohs(ip->flags_frags) & 0x1fff);
}

__NO_INLINE static void handle_ipv4_packet(netif_t *netif, pktbuf_t *p, const uint8_t *src_mac) {
    struct ipv4_hdr *ip;

    ip = (struct ipv4_hdr *)p->data;
    if (p->dlen < sizeof(struct ipv4_hdr)) {
        LTRACEF("REJECT: packet too short to hold header\n");
        return;
    }

    /* print packets for us */
    if (LOCAL_TRACE) {
        dump_ipv4_packet(ip);
    }

    /* reject bad packets */
    if (((ip->ver_ihl >> 4) & 0xf) != 4) {
        /* not version 4 */
        LTRACEF("REJECT: not version 4\n");
        return;
    }

    /* do we have enough buffer to hold the full header + options? */
    size_t header_len = (ip->ver_ihl & 0xf) * 4;
    if (p->dlen < header_len) {
        LTRACEF("REJECT: not enough buffer to hold header\n");
        return;
    }

    /* compute checksum */
    if (ones_sum16(0, (void *)ip, header_len) == 0) {
        /* bad checksum */
        LTRACEF("REJECT: bad checksum\n");
        return;
    }

    /* is the pkt_buf large enough to hold the length the header says the packet is? */
    if (htons(ip->len) > p->dlen) {
        LTRACEF("REJECT: packet exceeds size of buffer (header %d, dlen %d)\n", htons(ip->len), p->dlen);
        return;
    }

    /* trim any excess bytes at the end of the packet */
    if (p->dlen > htons(ip->len)) {
        pktbuf_consume_tail(p, p->dlen - htons(ip->len));
    }

    /* remove the header from the front of the packet_buf  */
    if (pktbuf_consume(p, header_len) == NULL) {
        return;
    }

    /* the packet is good, we can use it to populate our arp cache */
    arp_cache_update(ip->src_addr, src_mac);

    /* see if it's for us */
    if (ip->dst_addr != IPV4_BCAST) {
        if (netif->ipv4_addr != IPV4_NONE && ip->dst_addr != netif->ipv4_addr && ip->dst_addr != netif_get_broadcast_ipv4(netif)) {
            LTRACEF("REJECT: for another host\n");
            return;
        }
    }

    /* We only handle UDP and ECHO REQUEST */
    switch (ip->proto) {
        case IP_PROTO_ICMP: {
            struct icmp_pkt *icmp;
            if ((icmp = pktbuf_consume(p, sizeof(struct icmp_pkt))) == NULL) {
                break;
            }
            if (icmp->type == ICMP_ECHO_REQUEST) {
                send_ping_reply(netif, ip->src_addr, icmp, p->dlen);
            }
        }
        break;

        case IP_PROTO_UDP:
            udp_input(netif, p, ip->src_addr);
            break;

        case IP_PROTO_TCP:
            tcp_input(netif, p, ip->src_addr, ip->dst_addr);
            break;
    }
}

static void dump_eth_packet(const struct eth_hdr *eth) {
    printf("ETH src ");
    print_mac_address(eth->src_mac);
    printf(" dst ");
    print_mac_address(eth->dst_mac);
    printf(" type 0x%hx\n", htons(eth->type));
}

void minip_rx_driver_callback(netif_t *netif, pktbuf_t *p) {
    DEBUG_ASSERT(netif);
    DEBUG_ASSERT(p);

    struct eth_hdr *eth;
    if ((eth = (void *) pktbuf_consume(p, sizeof(struct eth_hdr))) == NULL) {
        return;
    }

    if (LOCAL_TRACE) {
        dump_eth_packet(eth);
    }

    if (memcmp(eth->dst_mac, netif->mac_address, 6) != 0 &&
            memcmp(eth->dst_mac, bcast_mac, 6) != 0) {
        /* not for us */
        return;
    }

    switch (htons(eth->type)) {
        case ETH_TYPE_IPV4:
            handle_ipv4_packet(netif, p, eth->src_mac);
            break;

        case ETH_TYPE_ARP:
            handle_arp_pkt(netif, p);
            break;
        default:
            LTRACEF("unhandled pkt type %#hx\n", htons(eth->type));
            break;
    }
}

// utility routines
uint32_t minip_parse_ipaddr(const char *ipaddr_str, size_t len) {
    uint8_t ip[4] = { 0, 0, 0, 0 };
    uint8_t pos = 0, i = 0;

    while (pos < len) {
        char c = ipaddr_str[pos];
        if (c == '.') {
            i++;
        } else if (c == '\0') {
            break;
        } else {
            ip[i] *= 10;
            ip[i] += c - '0';
        }
        pos++;
    }

    return IPV4_PACK(ip);
}

void print_mac_address(const uint8_t *mac) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void print_ipv4_address(ipv4_addr_t x) {
    union {
        uint32_t u;
        uint8_t b[4];
    } ip;
    ip.u = x;
    printf("%d.%d.%d.%d", ip.b[0], ip.b[1], ip.b[2], ip.b[3]);
}

void print_ipv4_address_named(const char *s, ipv4_addr_t x) {
    printf("%s ", s);
    print_ipv4_address(x);
}

// run static initialization
static void minip_init(uint level) {
    arp_cache_init();
    net_timer_init();
    netif_init();
}

LK_INIT_HOOK(minip, minip_init, LK_INIT_LEVEL_THREADING);
