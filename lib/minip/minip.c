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
#include <lk/list.h>
#include <lk/init.h>
#include <kernel/event.h>
#include <kernel/thread.h>

// TODO
// 1. Tear endian code out into something that flips words before/after tx/rx calls

#define LOCAL_TRACE 0
static uint32_t minip_ip      = IPV4_NONE;
static uint32_t minip_netmask = IPV4_NONE;
static uint32_t minip_broadcast = IPV4_BCAST;
static uint32_t minip_gateway = IPV4_NONE;

static const uint8_t broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static uint8_t minip_mac[6] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC};

static char minip_hostname[32] = "";

static volatile bool minip_configured = false;
static event_t minip_configured_event = EVENT_INITIAL_VALUE(minip_configured_event, false, 0);

/* This function is called by minip to send packets */
tx_func_t minip_tx_handler;
void *minip_tx_arg;

static void dump_mac_address(const uint8_t *mac);
static void dump_ipv4_addr(uint32_t addr);

/* if all the important configuration bits are set, signal that we're configured */
static void check_and_set_configured(void) {
    if (minip_ip == IPV4_NONE) return;
    if (minip_netmask == IPV4_NONE) return;
    if (minip_broadcast == IPV4_BCAST) return;
    // minip_gateway doesn't have to be set
    if (minip_mac[0] == 0xcc &&
        minip_mac[1] == 0xcc &&
        minip_mac[2] == 0xcc &&
        minip_mac[3] == 0xcc &&
        minip_mac[4] == 0xcc &&
        minip_mac[5] == 0xcc) return;

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

static void compute_broadcast_address(void) {
    minip_broadcast = (minip_ip & minip_netmask) | (IPV4_BCAST & ~minip_netmask);
}

void minip_get_macaddr(uint8_t *addr) {
    mac_addr_copy(addr, minip_mac);
}

uint32_t minip_get_ipaddr(void) {
    return minip_ip;
}

void minip_set_ipaddr(const uint32_t addr) {
    minip_ip = addr;
    compute_broadcast_address();
    check_and_set_configured();
}

uint32_t minip_get_broadcast(void) {
    return minip_broadcast;
}

uint32_t minip_get_netmask(void) {
    return minip_netmask;
}

void minip_set_netmask(const uint32_t netmask) {
    minip_netmask = netmask;
    compute_broadcast_address();
    check_and_set_configured();
}

uint32_t minip_get_gateway(void) {
    return minip_gateway;
}

void minip_set_gateway(const uint32_t addr) {
    minip_gateway = addr;
    // TODO: check that it is reacheable on local network
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
    minip_set_ipaddr(ip);
    minip_set_netmask(mask);
    minip_set_gateway(gateway);
}

void minip_set_eth(tx_func_t tx_handler, void *tx_arg, const uint8_t *macaddr) {
    LTRACEF("handler %p, arg %p, macaddr %p\n", tx_handler, tx_arg, macaddr);

    DEBUG_ASSERT(minip_tx_handler == NULL);
    DEBUG_ASSERT(minip_tx_arg == NULL);
    // TODO: assert mac address is not already set

    // set up the low level driver handler
    minip_tx_handler = tx_handler;
    minip_tx_arg = tx_arg;

    mac_addr_copy(minip_mac, macaddr);
}

static uint16_t ipv4_payload_len(struct ipv4_hdr *pkt) {
    return (pkt->len - ((pkt->ver_ihl >> 4) * 5));
}

void minip_build_mac_hdr(struct eth_hdr *pkt, const uint8_t *dst, uint16_t type) {
    mac_addr_copy(pkt->dst_mac, dst);
    mac_addr_copy(pkt->src_mac, minip_mac);
    pkt->type = htons(type);
}

void minip_build_ipv4_hdr(struct ipv4_hdr *ipv4, uint32_t dst, uint8_t proto, uint16_t len) {
    ipv4->ver_ihl       = 0x45;
    ipv4->dscp_ecn      = 0;
    ipv4->len           = htons(20 + len); // 5 * 4 from ihl, plus payload length
    ipv4->id            = 0;
    ipv4->flags_frags   = 0x40; // no offset, no fragments
    ipv4->ttl           = 64;
    ipv4->proto         = proto;
    ipv4->dst_addr      = dst;
    ipv4->src_addr      = minip_ip;

    /* This may be unnecessary if the controller supports checksum offloading */
    ipv4->chksum = 0;
    ipv4->chksum = rfc1701_chksum((uint8_t *) ipv4, sizeof(struct ipv4_hdr));
}

static int send_arp_request(uint32_t addr) {
    pktbuf_t *p;
    struct eth_hdr *eth;
    struct arp_pkt *arp;

    if ((p = pktbuf_alloc()) == NULL) {
        return -1;
    }

    eth = pktbuf_prepend(p, sizeof(struct eth_hdr));
    arp = pktbuf_append(p, sizeof(struct arp_pkt));
    minip_build_mac_hdr(eth, bcast_mac, ETH_TYPE_ARP);

    arp->htype = htons(0x0001);
    arp->ptype = htons(0x0800);
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = htons(ARP_OPER_REQUEST);
    arp->spa = minip_ip;
    arp->tpa = addr;
    mac_addr_copy(arp->sha, minip_mac);
    mac_addr_copy(arp->tha, bcast_mac);

    minip_tx_handler(minip_tx_arg, p);
    return 0;
}

static void handle_arp_timeout_cb(void *arg) {
    *(bool *)arg = true;
}

const uint8_t *get_dest_mac(uint32_t host) {
    uint8_t *dst_mac = NULL;
    bool arp_timeout = false;
    net_timer_t arp_timeout_timer;

    if (host == IPV4_BCAST) {
        return bcast_mac;
    }

    dst_mac = arp_cache_lookup(host);
    if (dst_mac == NULL) {
        send_arp_request(host);
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

status_t minip_ipv4_send(pktbuf_t *p, uint32_t dest_addr, uint8_t proto) {
    status_t ret = 0;
    size_t data_len = p->dlen;
    const uint8_t *dst_mac;

    struct ipv4_hdr *ip = pktbuf_prepend(p, sizeof(struct ipv4_hdr));
    struct eth_hdr *eth = pktbuf_prepend(p, sizeof(struct eth_hdr));

    // are we sending a broadcast packet?
    if (dest_addr == IPV4_BCAST || dest_addr == minip_broadcast) {
        dst_mac = bcast_mac;
        goto ready;
    }

    // is this a local subnet packet or do we need to send to the router?
    uint32_t target_addr = dest_addr;
    if ((dest_addr & minip_netmask) != (minip_ip & minip_netmask)) {
        // need to use the gateway
        if (minip_gateway == IPV4_NONE) {
            return ERR_NOT_FOUND; // TODO: better error code
        }

        target_addr = minip_gateway;
    }

    dst_mac = arp_get_dest_mac(target_addr);
    if (!dst_mac) {
        pktbuf_free(p, true);
        ret = -EHOSTUNREACH;
        goto err;
    }

ready:
    if (LOCAL_TRACE) {
        printf("sending ipv4\n");
    }

    minip_build_mac_hdr(eth, dst_mac, ETH_TYPE_IPV4);
    minip_build_ipv4_hdr(ip, dest_addr, proto, data_len);

    minip_tx_handler(minip_tx_arg, p);

err:
    return ret;
}

/* Swap the dst/src ip addresses and send an ICMP ECHO REPLY with the same payload.
 * According to spec the data portion doesn't matter, but ping itself validates that
 * the payload is identical
 */
static void send_ping_reply(uint32_t ipaddr, struct icmp_pkt *req, size_t reqdatalen) {
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

    minip_build_mac_hdr(eth, arp_cache_lookup(ipaddr), ETH_TYPE_IPV4);
    minip_build_ipv4_hdr(ip, ipaddr, IP_PROTO_ICMP, len);

    icmp->type = ICMP_ECHO_REPLY;
    icmp->code = 0;
    memcpy(icmp->hdr_data, req->hdr_data, sizeof(icmp->hdr_data));
    icmp->chksum = 0;
    icmp->chksum = rfc1701_chksum((uint8_t *) icmp, len);

    minip_tx_handler(minip_tx_arg, p);
}

static void dump_ipv4_addr(uint32_t addr) {
    const uint8_t *a = (void *)&addr;

    printf("%hhu.%hhu.%hhu.%hhu", a[0], a[1], a[2], a[3]);
}

static void dump_ipv4_packet(const struct ipv4_hdr *ip) {
    printf("IP ");
    dump_ipv4_addr(ip->src_addr);
    printf(" -> ");
    dump_ipv4_addr(ip->dst_addr);
    printf(" hlen 0x%x, prot 0x%x, cksum 0x%x, len 0x%x, ident 0x%x, frag offset 0x%x\n",
           (ip->ver_ihl & 0xf) * 4, ip->proto, ntohs(ip->chksum), ntohs(ip->len), ntohs(ip->id), ntohs(ip->flags_frags) & 0x1fff);
}

__NO_INLINE static void handle_ipv4_packet(pktbuf_t *p, const uint8_t *src_mac) {
    struct ipv4_hdr *ip;

    ip = (struct ipv4_hdr *)p->data;
    if (p->dlen < sizeof(struct ipv4_hdr))
        return;

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
    if (rfc1701_chksum((void *)ip, header_len) != 0) {
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
        if (minip_ip != IPV4_NONE && ip->dst_addr != minip_ip && ip->dst_addr != minip_broadcast) {
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
                send_ping_reply(ip->src_addr, icmp, p->dlen);
            }
        }
        break;

        case IP_PROTO_UDP:
            udp_input(p, ip->src_addr);
            break;

        case IP_PROTO_TCP:
            tcp_input(p, ip->src_addr, ip->dst_addr);
            break;
    }
}

__NO_INLINE static int handle_arp_pkt(pktbuf_t *p) {
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

            if (memcmp(&arp->tpa, &minip_ip, sizeof(minip_ip)) == 0) {
                if ((rp = pktbuf_alloc()) == NULL) {
                    break;
                }

                reth = pktbuf_prepend(rp, sizeof(struct eth_hdr));
                rarp = pktbuf_append(rp, sizeof(struct arp_pkt));

                // Eth header
                minip_build_mac_hdr(reth, eth->src_mac, ETH_TYPE_ARP);

                // ARP packet
                rarp->oper = htons(ARP_OPER_REPLY);
                rarp->htype = htons(0x0001);
                rarp->ptype = htons(0x0800);
                rarp->hlen = 6;
                rarp->plen = 4;
                mac_addr_copy(rarp->sha, minip_mac);
                rarp->spa = minip_ip;
                mac_addr_copy(rarp->tha, arp->sha);
                rarp->tpa = arp->spa;

                minip_tx_handler(minip_tx_arg, rp);
            }
        }
        break;

        case ARP_OPER_REPLY: {
            uint32_t addr;
            memcpy(&addr, &arp->spa, sizeof(addr)); // unaligned word
            arp_cache_update(addr, arp->sha);
        }
        break;
    }

    return 0;
}

static void dump_eth_packet(const struct eth_hdr *eth) {
    printf("ETH src ");
    dump_mac_address(eth->src_mac);
    printf(" dst ");
    dump_mac_address(eth->dst_mac);
    printf(" type 0x%hx\n", htons(eth->type));
}

void minip_rx_driver_callback(pktbuf_t *p) {
    struct eth_hdr *eth;

    if ((eth = (void *) pktbuf_consume(p, sizeof(struct eth_hdr))) == NULL) {
        return;
    }

    if (LOCAL_TRACE) {
        dump_eth_packet(eth);
    }

    if (memcmp(eth->dst_mac, minip_mac, 6) != 0 &&
            memcmp(eth->dst_mac, broadcast_mac, 6) != 0) {
        /* not for us */
        return;
    }

    switch (htons(eth->type)) {
        case ETH_TYPE_IPV4:
            LTRACEF("ipv4 pkt\n");
            handle_ipv4_packet(p, eth->src_mac);
            break;

        case ETH_TYPE_ARP:
            LTRACEF("arp pkt\n");
            handle_arp_pkt(p);
            break;
    }
}

void dump_mac_address(const uint8_t *mac) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

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

// printf the ip address passed in
void printip(uint32_t x) {
    union {
        u32 u;
        u8 b[4];
    } ip;
    ip.u = x;
    printf("%d.%d.%d.%d", ip.b[0], ip.b[1], ip.b[2], ip.b[3]);
}

void printip_named(const char *s, uint32_t x) {
    printf("%s ", s);
    printip(x);
}

// run static initialization
static void minip_init(uint level) {
    arp_cache_init();
    net_timer_init();
}


LK_INIT_HOOK(minip, minip_init, LK_INIT_LEVEL_THREADING);
