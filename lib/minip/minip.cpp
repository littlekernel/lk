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

bool minip_trace = (LOCAL_TRACE != 0);

static ipv4_addr_t minip_gateway = IPV4_NONE;
static ipv4_addr_t minip_dns_server = IPV4_NONE;

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

ipv4_addr_t minip_get_dns_server(void) {
    return minip_dns_server;
}

void minip_set_dns_server(const ipv4_addr_t addr) {
    minip_dns_server = addr;
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

/* prepend the ethernet and ipv4 headers on a payload */
static void minip_ipv4_build(netif_t *netif, pktbuf_t *p, ipv4_addr_t dest_addr, uint8_t proto,
                             const uint8_t *dest_mac) {
    size_t data_len = p->dlen;

    struct ipv4_hdr *ip = (struct ipv4_hdr *)pktbuf_prepend(p, sizeof(struct ipv4_hdr));
    struct eth_hdr *eth = (struct eth_hdr *)pktbuf_prepend(p, sizeof(struct eth_hdr));

    minip_build_mac_hdr(netif, eth, dest_mac, ETH_TYPE_IPV4);
    minip_build_ipv4_hdr(netif, ip, dest_addr, proto, data_len);
}

status_t minip_ipv4_send_raw(pktbuf_t *p, ipv4_addr_t dest_addr, uint8_t proto, const uint8_t *dest_mac, netif_t *netif) {
    DEBUG_ASSERT(p);
    DEBUG_ASSERT(netif);

    if (LOCAL_TRACE) {
        printf("sending ipv4\n");
    }

    minip_ipv4_build(netif, p, dest_addr, proto, dest_mac);

    return netif_tx(netif, p);
}

status_t minip_ipv4_send(pktbuf_t *p, ipv4_addr_t dest_addr, uint8_t proto) {
    status_t ret = 0;
    netif_t *netif = NULL;
    const uint8_t *dest_mac = NULL;
    ipv4_addr_t target_addr = dest_addr;
    ipv4_addr_t netmask;

    // TODO: cache route at socket creation
    ipv4_route_t *route = ipv4_search_route(dest_addr);
    if (!route) {
        /* like every other outcome, consume the caller's reference */
        pktbuf_free(p, true);
        ret = -EHOSTUNREACH;
        goto err;
    }
    DEBUG_ASSERT(route->interface);
    netif = route->interface;

    // are we sending a broadcast packet?
    if (dest_addr == IPV4_BCAST || dest_addr == netif_get_broadcast_ipv4(netif)) {
        dest_mac = bcast_mac;
        goto ready;
    }

    // loopback needs no arp; address the frame to our own mac
    if (netif_is_loopback(netif)) {
        dest_mac = netif->mac_address;
        goto ready;
    }

    // is this a local subnet packet or do we need to send to the router?
    netmask = netif_get_netmask_ipv4(netif);
    if ((dest_addr & netmask) != (netif->ipv4_addr & netmask)) {
        // need to use the gateway
        if (minip_gateway == IPV4_NONE) {
            pktbuf_free(p, true);
            ret = ERR_NOT_FOUND; // TODO: better error code
            goto err;
        }

        target_addr = minip_gateway;
    }

    // fast path: address already resolved
    dest_mac = arp_cache_lookup(target_addr);
    if (dest_mac) {
        goto ready;
    }

    /* the mac is not known yet: build the frame with a placeholder mac and
     * hand it to the arp layer, which fills it in and transmits when the
     * address resolves (or drops it if resolution fails). never blocks.
     */
    {
        static const uint8_t zero_mac[6] = { 0, 0, 0, 0, 0, 0 };
        minip_ipv4_build(netif, p, dest_addr, proto, zero_mac);
        ret = arp_send_or_queue(netif, target_addr, p);
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

    icmp = (struct icmp_pkt *)pktbuf_prepend(p, sizeof(struct icmp_pkt));
    ip = (struct ipv4_hdr *)pktbuf_prepend(p, sizeof(struct ipv4_hdr));
    eth = (struct eth_hdr *)pktbuf_prepend(p, sizeof(struct eth_hdr));
    pktbuf_append_data(p, req->data, reqdatalen);

    len = sizeof(struct icmp_pkt) + reqdatalen;

    minip_build_mac_hdr(netif, eth, arp_cache_lookup(ipaddr), ETH_TYPE_IPV4);
    minip_build_ipv4_hdr(netif, ip, ipaddr, IP_PROTO_ICMP, len);

    icmp->type = ICMP_ECHO_REPLY;
    icmp->code = 0;
    memcpy(icmp->hdr_data, req->hdr_data, sizeof(icmp->hdr_data));
    icmp->chksum = 0;
    icmp->chksum = ~ones_sum16(0, (uint8_t *) icmp, len);

    netif_tx(netif, p);
}

__NO_INLINE static void dump_ipv4_packet(const struct ipv4_hdr *ip) {
    printf("IP ");
    print_ipv4_address(ip->src_addr);
    printf(" -> ");
    print_ipv4_address(ip->dst_addr);
    printf(" hlen 0x%x, prot 0x%x, cksum 0x%x, len 0x%x, ident 0x%x, frag offset 0x%x\n",
           (ip->ver_ihl & 0xf) * 4, ip->proto, ntohs(ip->chksum), ntohs(ip->len), ntohs(ip->id), ntohs(ip->flags_frags) & 0x1fff);
}

/* returns true if ownership of p was taken by a protocol layer */
__NO_INLINE static bool handle_ipv4_packet(netif_t *netif, pktbuf_t *p, const uint8_t *src_mac) {
    struct ipv4_hdr *ip;

    ip = (struct ipv4_hdr *)p->data;
    if (p->dlen < sizeof(struct ipv4_hdr)) {
        LTRACEF("REJECT: packet too short to hold header\n");
        return false;
    }

    /* print packets for us */
    if (minip_trace) {
        dump_ipv4_packet(ip);
    }

    /* reject bad packets */
    if (((ip->ver_ihl >> 4) & 0xf) != 4) {
        /* not version 4 */
        LTRACEF("REJECT: not version 4\n");
        return false;
    }

    /* do we have enough buffer to hold the full header + options? */
    size_t header_len = (ip->ver_ihl & 0xf) * 4;
    if (p->dlen < header_len) {
        LTRACEF("REJECT: not enough buffer to hold header\n");
        return false;
    }

    /* compute checksum */
    if (ones_sum16(0, (void *)ip, header_len) != 0xffff) {
        /* bad checksum */
        LTRACEF("REJECT: bad checksum\n");
        return false;
    }

    /* is the pkt_buf large enough to hold the length the header says the packet is? */
    if (htons(ip->len) > p->dlen) {
        LTRACEF("REJECT: packet exceeds size of buffer (header %d, dlen %d)\n", htons(ip->len), p->dlen);
        return false;
    }

    /* trim any excess bytes at the end of the packet */
    if (p->dlen > htons(ip->len)) {
        pktbuf_consume_tail(p, p->dlen - htons(ip->len));
    }

    /* remove the header from the front of the packet_buf  */
    if (pktbuf_consume(p, header_len) == NULL) {
        return false;
    }

    /* The packet is good, so use it to populate the arp cache -- except on
     * loopback and for 127/8 sources, which are not ethernet neighbors
     * (some NATs leak host-loopback-sourced frames onto the wire).
     */
    if (!netif_is_loopback(netif) && (ip->src_addr & 0xff) != 127) {
        arp_cache_update(ip->src_addr, src_mac);
    }

    /* see if it's for us */
    if (ip->dst_addr != IPV4_BCAST) {
        if (netif->ipv4_addr != IPV4_NONE && ip->dst_addr != netif->ipv4_addr && ip->dst_addr != netif_get_broadcast_ipv4(netif)) {
            LTRACEF("REJECT: for another host\n");
            return false;
        }
    }

    /* We only handle UDP and ECHO REQUEST */
    switch (ip->proto) {
        case IP_PROTO_ICMP: {
            struct icmp_pkt *icmp;
            if ((icmp = (struct icmp_pkt *)pktbuf_consume(p, sizeof(struct icmp_pkt))) == NULL) {
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
            return tcp_input(netif, p, ip->src_addr, ip->dst_addr);
    }

    return false;
}

static void dump_eth_packet(const struct eth_hdr *eth) {
    printf("ETH src ");
    print_mac_address(eth->src_mac);
    printf(" dst ");
    print_mac_address(eth->dst_mac);
    printf(" type 0x%hx\n", htons(eth->type));
}

/* main demux of a received frame, called on the stack worker thread.
 * Returns true if ownership of p was taken by a protocol layer; otherwise
 * the caller frees it.
 */
bool minip_rx_process(netif_t *netif, pktbuf_t *p) {
    DEBUG_ASSERT(netif);
    DEBUG_ASSERT(p);

    LTRACEF("netif %p, p %p, dlen %u\n", netif, p, p->dlen);

    struct eth_hdr *eth;
    if ((eth = (struct eth_hdr *)pktbuf_consume(p, sizeof(struct eth_hdr))) == NULL) {
        return false;
    }

    if (minip_trace) {
        dump_eth_packet(eth);
    }

    if (memcmp(eth->dst_mac, netif->mac_address, 6) != 0 &&
            memcmp(eth->dst_mac, bcast_mac, 6) != 0) {
        /* not for us */
        return false;
    }

    switch (htons(eth->type)) {
        case ETH_TYPE_IPV4:
            return handle_ipv4_packet(netif, p, eth->src_mac);

        case ETH_TYPE_ARP:
            handle_arp_pkt(netif, p);
            break;
        default:
            LTRACEF("unhandled pkt type %#hx\n", htons(eth->type));
            break;
    }

    return false;
}

// utility routines

/* Parse a dotted quad. Anything that is not exactly four decimal octets is
 * rejected, which is what lets callers use this to tell a literal address
 * from a host name.
 */
status_t minip_parse_ipaddr_checked(const char *ipaddr_str, size_t len, ipv4_addr_t *out) {
    if (!ipaddr_str || !out) {
        return ERR_INVALID_ARGS;
    }

    uint8_t ip[4] = { 0, 0, 0, 0 };
    size_t octet = 0;
    uint digits = 0;
    uint val = 0;

    for (size_t pos = 0; pos < len; pos++) {
        char c = ipaddr_str[pos];

        if (c == '\0') {
            break;
        } else if (c == '.') {
            if (digits == 0 || octet >= 3) {
                return ERR_NOT_VALID;
            }
            ip[octet++] = (uint8_t)val;
            val = 0;
            digits = 0;
        } else if (c >= '0' && c <= '9') {
            if (++digits > 3) {
                return ERR_NOT_VALID;
            }
            val = val * 10 + (uint)(c - '0');
            if (val > 255) {
                return ERR_NOT_VALID;
            }
        } else {
            return ERR_NOT_VALID;
        }
    }

    if (digits == 0 || octet != 3) {
        return ERR_NOT_VALID;
    }
    ip[3] = (uint8_t)val;

    *out = IPV4_PACK(ip);
    return NO_ERROR;
}

uint32_t minip_parse_ipaddr(const char *ipaddr_str, size_t len) {
    ipv4_addr_t addr;

    if (minip_parse_ipaddr_checked(ipaddr_str, len, &addr) != NO_ERROR) {
        return IPV4_NONE;
    }
    return addr;
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
    netstack_init();
    netif_init();
}

LK_INIT_HOOK(minip, minip_init, LK_INIT_LEVEL_THREADING);
