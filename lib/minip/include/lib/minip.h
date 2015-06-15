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

#include <endian.h>
#include <list.h>
#include <stdint.h>
#include <sys/types.h>

#include <lib/pktbuf.h>

#define IPV4(a,b,c,d) (((a)&0xFF)|(((b)&0xFF)<<8)|(((c)&0xFF)<<16)|(((d)&0xFF)<<24))
#define IPV4_SPLIT(a) (a & 0xFF), ((a >> 8) & 0xFF), ((a >> 16) & 0xFF), ((a >> 24) & 0xFF)
#define IPV4_PACK(a) (a[3] << 24 | a[2] << 16 | a[1] << 8 | a[0])
#define IPV4_BCAST (0xFFFFFFFF)
#define IPV4_NONE (0)

typedef int (*tx_func_t)(pktbuf_t *p);
typedef void (*udp_callback_t)(void *data, size_t len,
    uint32_t srcaddr, uint16_t srcport, void *arg);

/* initialize minip with static configuration */
void minip_init(tx_func_t tx_func, void *tx_arg,
    uint32_t ip, uint32_t netmask, uint32_t gateway);

/* initialize minip with DHCP configuration */
void minip_init_dhcp(tx_func_t tx_func, void *tx_arg);

/* packet rx hook to hand to ethernet driver */
void minip_rx_driver_callback(pktbuf_t *p);

/* global configuration state */
void minip_get_macaddr(uint8_t *addr);
void minip_set_macaddr(const uint8_t *addr);

uint32_t minip_get_ipaddr(void);
void minip_set_ipaddr(const uint32_t addr);

void minip_set_hostname(const char *name);
const char *minip_get_hostname(void);

uint32_t minip_parse_ipaddr(const char *addr, size_t len);

/* udp */
typedef struct udp_socket udp_socket_t;

int udp_listen(uint16_t port, udp_callback_t cb, void *arg);
status_t udp_open(uint32_t host, uint16_t sport, uint16_t dport, udp_socket_t **handle);
status_t udp_send(void *buf, size_t len, udp_socket_t *handle);
status_t udp_close(udp_socket_t *handle);


/* tcp */
typedef struct tcp_socket tcp_socket_t;

status_t tcp_open_listen(tcp_socket_t **handle, uint16_t port);
status_t tcp_accept_timeout(tcp_socket_t *listen_socket, tcp_socket_t **accept_socket, lk_time_t timeout);
status_t tcp_close(tcp_socket_t *socket);
ssize_t tcp_read(tcp_socket_t *socket, void *buf, size_t len);
ssize_t tcp_write(tcp_socket_t *socket, const void *buf, size_t len);

static inline status_t tcp_accept(tcp_socket_t *listen_socket, tcp_socket_t **accept_socket)
{
    return tcp_accept_timeout(listen_socket, accept_socket, INFINITE_TIME);
}

// vim: set ts=4 sw=4 expandtab:
