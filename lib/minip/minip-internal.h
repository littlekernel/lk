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
#pragma once

#include <lib/minip.h>

#include <compiler.h>
#include <endian.h>
#include <list.h>
#include <stdint.h>
#include <string.h>

/* Lib configuration */
#define MINIP_USE_UDP_CHECKSUM    0
#define MINIP_MTU_SIZE            1536
#define MINIP_USE_ARP             1

#pragma pack(push, 1)
struct arp_pkt {
    uint16_t htype;
    uint16_t ptype;
    uint8_t  hlen;
    uint8_t  plen;
    uint16_t oper;
    uint8_t  sha[6];
    uint32_t spa;
    uint8_t  tha[6];
    uint32_t tpa;
};

struct ipv4_hdr {
    uint8_t  ver_ihl;
    uint8_t  dscp_ecn;
    uint16_t len;
    uint16_t id;
    uint16_t flags_frags;
    uint8_t  ttl;
    uint8_t  proto;
    uint16_t chksum;
    uint32_t src_addr;
    uint32_t dst_addr;
    uint8_t  data[];
};

struct icmp_pkt {
    uint8_t  type;
    uint8_t  code;
    uint16_t chksum;
    uint8_t  hdr_data[4];
    uint8_t  data[];
};

struct eth_hdr {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t type;
};

#pragma pack(pop)

enum {
    ICMP_ECHO_REPLY   = 0,
    ICMP_ECHO_REQUEST = 8,
};

enum {
    IP_PROTO_ICMP = 0x1,
    IP_PROTO_TCP  = 0x6,
    IP_PROTO_UDP  = 0x11,
};

enum {
    ETH_TYPE_IPV4 = 0x0800,
    ETH_TYPE_ARP  = 0x0806,
};

enum {
    ARP_OPER_REQUEST = 0x0001,
    ARP_OPER_REPLY   = 0x0002,
};

extern tx_func_t minip_tx_handler;
typedef struct udp_hdr udp_hdr_t;
static const uint8_t bcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void arp_cache_init(void);
void arp_cache_update(uint32_t addr, const uint8_t mac[6]);
uint8_t *arp_cache_lookup(uint32_t addr);
void arp_cache_dump(void);
int send_arp_request(uint32_t addr);

uint16_t rfc1701_chksum(const uint8_t *buf, size_t len);
uint16_t rfc768_chksum(struct ipv4_hdr *ipv4, udp_hdr_t *udp);
uint16_t ones_sum16(uint32_t sum, const void *_buf, int len);

/* Helper methods for building headers */
void minip_build_mac_hdr(struct eth_hdr *pkt, const uint8_t *dst, uint16_t type);
void minip_build_ipv4_hdr(struct ipv4_hdr *ipv4, uint32_t dst, uint8_t proto, uint16_t len);

status_t minip_ipv4_send(pktbuf_t *p, uint32_t dest_addr, uint8_t proto);

void tcp_input(pktbuf_t *p, uint32_t src_ip, uint32_t dst_ip);
void udp_input(pktbuf_t *p, uint32_t src_ip);

const uint8_t *get_dest_mac(uint32_t host);

// timers
typedef void (*net_timer_callback_t)(void *);

typedef struct net_timer {
    struct list_node node;

    lk_time_t sched_time;

    net_timer_callback_t cb;
    void *arg;
} net_timer_t;

/* set a net timer. returns true if the timer was not set before and is now */
bool net_timer_set(net_timer_t *, net_timer_callback_t, void *callback_args, lk_time_t delay) __NONNULL((1));

/* cancels a net timer. returns true if it was previously set and is not now */
bool net_timer_cancel(net_timer_t *) __NONNULL();

void net_timer_init(void);

static inline void mac_addr_copy(uint8_t *dest, const uint8_t *src) {
    *(uint32_t *)dest = *(const uint32_t *)src;
    *(uint16_t *)(dest + 4) = *(const uint16_t *)(src + 4);
}

// vim: set ts=4 sw=4 expandtab:

