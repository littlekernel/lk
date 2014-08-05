/*
 * Copyright (c) 2014 Chris Anderson
 * Copyright (c) 2014 Brian Swetland
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

#include "minip-internal.h"

#include <stdio.h>
#include <debug.h>
#include <endian.h>
#include <stdlib.h>
#include <string.h>
#include <trace.h>
#include <malloc.h>
#include <list.h>
#include <kernel/mutex.h>

struct udp_listener {
    struct list_node list;
    uint16_t port;
    udp_callback_t callback;
    void *arg;
};

static struct list_node udp_list = LIST_INITIAL_VALUE(udp_list);
static struct list_node arp_list = LIST_INITIAL_VALUE(arp_list);

// TODO
// 1. Tear endian code out into something that flips words before/after tx/rx calls

#define LOCAL_TRACE 0
static uint32_t minip_ip      = IPV4_NONE;
static uint32_t minip_netmask = IPV4_NONE;
static uint32_t minip_broadcast = IPV4_BCAST;
static uint32_t minip_gateway = IPV4_NONE;

static uint8_t minip_mac[6] = {0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC};
static uint8_t bcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static char minip_hostname[32] = "";

static mutex_t tx_mutex;

void minip_set_hostname(const char *name) {
    size_t len = strlen(name);
    if (len >= sizeof(minip_hostname)) {
        len = sizeof(minip_hostname) - 1;
    }
    memcpy(minip_hostname, name, len);
    minip_hostname[len] = 0;
}

const char *minip_get_hostname(void) {
   return minip_hostname;
}

int minip_udp_listen(uint16_t port, udp_callback_t cb, void *arg) {
    struct udp_listener *entry;

    list_for_every_entry(&udp_list, entry, struct udp_listener, list) {
        if (entry->port == port) {
            return -1;
        }
    }

    if ((entry = malloc(sizeof(struct udp_listener))) == NULL) {
        return -1;
    }
    entry->port = port;
    entry->callback = cb;
    entry->arg = arg;
    list_add_tail(&udp_list, &entry->list);
    return 0;
}

static void compute_broadcast_address(void)
{
    minip_broadcast = (minip_ip & minip_netmask) | (IPV4_BCAST & ~minip_netmask);
}

void minip_get_macaddr(uint8_t *addr) {
    memcpy(addr, minip_mac, 6);
}

void minip_set_macaddr(const uint8_t *addr) {
    memcpy(minip_mac, addr, 6);
}

uint32_t minip_get_ipaddr(void) {
    return minip_ip;
}

void minip_set_ipaddr(const uint32_t addr) {
    minip_ip = addr;
    compute_broadcast_address();
}

/* This function is called by minip to send packets */
tx_func_t minip_tx_handler;
void *minip_tx_arg;

void minip_init(tx_func_t tx_handler, void *tx_arg,
    uint32_t ip, uint32_t mask, uint32_t gateway)
{
    minip_tx_handler = tx_handler;
    minip_tx_arg = tx_arg;

    minip_ip = ip;
    minip_netmask = mask;
    minip_gateway = gateway;
    compute_broadcast_address();

    mutex_init(&tx_mutex);
    arp_cache_init();
    net_timer_init();
}

uint16_t ipv4_payload_len(struct ipv4_hdr *pkt)
{
    return (pkt->len - ((pkt->ver_ihl >> 4) * 5));
}

static void fill_in_mac_header(struct eth_hdr *pkt, uint8_t *dst, uint16_t type)
{
    memcpy(pkt->dst_mac, dst, sizeof(pkt->dst_mac));
    memcpy(pkt->src_mac, minip_mac, sizeof(minip_mac));
    pkt->type = htons(type);
}

static void fill_in_ipv4_header(struct ipv4_hdr *ipv4, uint32_t dst, uint8_t proto, uint16_t len)
{
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

int send_arp_request(uint32_t addr)
{
    pktbuf_t *p;
    struct eth_hdr *eth;
    struct arp_pkt *arp;

    if ((p = pktbuf_alloc()) == NULL) {
        return -1;
    }

    eth = pktbuf_prepend(p, sizeof(struct eth_hdr));
    arp = pktbuf_append(p, sizeof(struct arp_pkt));
    fill_in_mac_header(eth, bcast_mac, ETH_TYPE_ARP);

    arp->htype = htons(0x0001);
    arp->ptype = htons(0x0800);
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = htons(ARP_OPER_REQUEST);
    memcpy(&arp->spa, &minip_ip, sizeof(arp->spa));
    memcpy(&arp->tpa, &addr, sizeof(arp->tpa));
    memcpy(arp->sha, minip_mac, sizeof(arp->sha));
    memcpy(arp->tha, bcast_mac, sizeof(arp->tha));

    minip_tx_handler(p);
    return 0;
}

status_t minip_ipv4_send(pktbuf_t *p, uint32_t dest_addr, uint8_t proto)
{
    status_t ret = 0;
    size_t data_len = p->dlen;
    uint8_t *dst_mac;

    struct ipv4_hdr *ip = pktbuf_prepend(p, sizeof(struct ipv4_hdr));
    struct eth_hdr *eth = pktbuf_prepend(p, sizeof(struct eth_hdr));

    mutex_acquire(&tx_mutex);

    if (dest_addr == IPV4_BCAST || dest_addr == minip_broadcast) {
        dst_mac = bcast_mac;
        goto ready;
    }

    /* If we're missing an address in the cache send out a request periodically for a bit */
    dst_mac = arp_cache_lookup(dest_addr);
    if (!dst_mac) {
        send_arp_request(dest_addr);
        // TODO: Add a timeout here rather than an arbitrary iteration limit
        for (int i = 50000; i > 0; i--) {
            if ((dst_mac = arp_cache_lookup(dest_addr)) != NULL) {
                break;
            }
        }

        if (dst_mac == NULL) {
            ret = -1;
            goto err;
        }
    }

ready:
    fill_in_mac_header(eth, dst_mac, ETH_TYPE_IPV4);
    fill_in_ipv4_header(ip, dest_addr, proto, data_len);

    minip_tx_handler(p);

err:
    mutex_release(&tx_mutex);
    return ret;
}

int minip_udp_send(const void *buf, size_t len, uint32_t addr,
    uint16_t dstport, uint16_t srcport)
{
    pktbuf_t *p;
    struct eth_hdr *eth;
    struct ipv4_hdr *ip;
    struct udp_hdr *udp;
    uint8_t *dst_mac;
    int ret = 0;

    if ((p = pktbuf_alloc()) == NULL) {
        return -1;
    }

    udp = pktbuf_prepend(p, sizeof(struct udp_hdr));
    ip = pktbuf_prepend(p, sizeof(struct ipv4_hdr));
    eth = pktbuf_prepend(p, sizeof(struct eth_hdr));
    memset(p->data, 0, p->dlen);
    pktbuf_append_data(p, buf, len);

    mutex_acquire(&tx_mutex);

    if (addr == IPV4_BCAST) {
        dst_mac = bcast_mac;
        goto ready;
    }

    /* If we're missing an address in the cache send out a request periodically for a bit */
    dst_mac = arp_cache_lookup(addr);
    if (!dst_mac) {
        send_arp_request(addr);
        // TODO: Add a timeout here rather than an arbitrary iteration limit
        for (int i = 50000; i > 0; i--) {
            if ((dst_mac = arp_cache_lookup(addr)) != NULL) {
                break;
            }
        }

        if (dst_mac == NULL) {
            ret = -1;
            goto err;
        }
    }

ready:
    udp->src_port   = htons(srcport);
    udp->dst_port  = htons(dstport);
    udp->len        = htons(sizeof(struct udp_hdr) + len);
    udp->chksum     = 0;
    memcpy(udp->data, buf, len);

    fill_in_mac_header(eth, dst_mac, ETH_TYPE_IPV4);
    fill_in_ipv4_header(ip, addr, IP_PROTO_UDP, len + sizeof(struct udp_hdr));

#if (MINIP_USE_UDP_CHECKSUM != 0)
    udp->chksum = rfc768_chksum(ip, udp);
#endif

    minip_tx_handler(p);

err:
    mutex_release(&tx_mutex);
    return ret;
}

/* Swap the dst/src ip addresses and send an ICMP ECHO REPLY with the same payload.
 * According to spec the data portion doesn't matter, but ping itself validates that
 * the payload is identical
 */
void send_ping_reply(uint32_t ipaddr, struct icmp_pkt *req, size_t reqdatalen)
{
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

    fill_in_mac_header(eth, arp_cache_lookup(ipaddr), ETH_TYPE_IPV4);
    fill_in_ipv4_header(ip, ipaddr, IP_PROTO_ICMP, len);

    icmp->type = ICMP_ECHO_REPLY;
    icmp->code = 0;
    memcpy(icmp->hdr_data, req->hdr_data, sizeof(icmp->hdr_data));
    icmp->chksum = 0;
    icmp->chksum = rfc1701_chksum((uint8_t *) icmp, len);

    minip_tx_handler(p);
}

static void dump_ipv4_addr(uint32_t addr)
{
    const uint8_t *a = (void *)&addr;

    printf("%hhu.%hhu.%hhu.%hhu", a[0], a[1], a[2], a[3]);
}

static void dump_ipv4_packet(const struct ipv4_hdr *ip)
{
    printf("IP ");
    dump_ipv4_addr(ip->src_addr);
    printf(" -> ");
    dump_ipv4_addr(ip->dst_addr);
    printf(" hlen 0x%x, prot 0x%x, cksum 0x%x, len 0x%x, ident 0x%x, frag offset 0x%x\n",
        (ip->ver_ihl & 0xf) * 4, ip->proto, ntohs(ip->chksum), ntohs(ip->len), ntohs(ip->id), ntohs(ip->flags_frags) & 0x1fff);
}

__NO_INLINE static void handle_ipv4_packet(pktbuf_t *p, const uint8_t *src_mac)
{
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
        //LTRACEF("REJECT: not version 4\n");
        return;
    }

    /* do we have enough buffer to hold the full header + options? */
    size_t header_len = (ip->ver_ihl & 0xf) * 4;
    if (p->dlen < header_len) {
        //LTRACEF("REJECT: not enough buffer to hold header\n");
        return;
    }

    /* compute checksum */
    if (rfc1701_chksum((void *)ip, header_len) != 0) {
        /* bad checksum */
        //LTRACEF("REJECT: bad checksum\n");
        return;
    }

    /* is the pkt_buf large enough to hold the length the header says the packet is? */
    if (htons(ip->len) > p->dlen) {
        //LTRACEF("REJECT: packet exceeds size of buffer (header %d, dlen %d)\n", htons(ip->len), p->dlen);
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
            //LTRACEF("REJECT: for another host\n");
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

        case IP_PROTO_UDP: {
            struct udp_hdr *udp;
            struct udp_listener *e;
            uint16_t port;

            if ((udp = pktbuf_consume(p, sizeof(struct udp_hdr))) == NULL) {
                break;
            }
            port = ntohs(udp->dst_port);

            list_for_every_entry(&udp_list, e, struct udp_listener, list) {
                if (e->port == port) {
                    e->callback(p->data, p->dlen, ip->src_addr, ntohs(udp->src_port), e->arg);
                    return;
                }
            }
        }
        break;

        case IP_PROTO_TCP:
            tcp_input(p, ip->src_addr, ip->dst_addr);
        break;
    }
}

__NO_INLINE static int handle_arp_pkt(pktbuf_t *p)
{
    struct eth_hdr *eth;
    struct arp_pkt *arp;

    eth = (void*) (p->data - sizeof(struct eth_hdr));

    if ((arp = pktbuf_consume(p, sizeof(struct arp_pkt))) == NULL) {
        return -1;
    }

    switch(ntohs(arp->oper)) {
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
                fill_in_mac_header(reth, eth->src_mac, ETH_TYPE_ARP);

                // ARP packet
                rarp->oper = htons(ARP_OPER_REPLY);
                rarp->htype = htons(0x0001);
                rarp->ptype = htons(0x0800);
                rarp->hlen = 6;
                rarp->plen = 4;
                memcpy(rarp->tha, arp->sha, sizeof(arp->tha));
                memcpy(rarp->sha, minip_mac, sizeof(arp->sha));
                memcpy(&rarp->tpa, &arp->spa, sizeof(rarp->tpa));
                memcpy(&rarp->spa, &minip_ip, sizeof(rarp->spa));

                minip_tx_handler(rp);
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

void minip_rx_driver_callback(pktbuf_t *p)
{
    struct eth_hdr *eth;

    if ((eth = (void*) pktbuf_consume(p, sizeof(struct eth_hdr))) == NULL) {
        return;
    }

    switch(htons(eth->type)) {
        case ETH_TYPE_IPV4:
            handle_ipv4_packet(p, eth->src_mac);
            break;

        case ETH_TYPE_ARP:
            handle_arp_pkt(p);
            break;
    }
}

minip_fd_t *minip_open(uint32_t addr, uint16_t port)
{
    minip_fd_t *fd = malloc(sizeof(minip_fd_t));
    if (!fd) {
        return fd;
    }

    send_arp_request(addr);
    fd->addr = addr;
    fd->port = port;

    return fd;
}

void minip_close(minip_fd_t *fd)
{
    free(fd);
}

void send(minip_fd_t *fd, void *buf, size_t len, int flags)
{
    minip_udp_send(buf, len, fd->addr, fd->port, fd->port);
}

// vim: set ts=4 sw=4 expandtab:
