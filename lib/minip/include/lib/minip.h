/*
 * Copyright (c) 2014 Chris Anderson
 * Copyright (c) 2014 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <endian.h>
#include <iovec.h>
#include <lib/pktbuf.h>
#include <lk/compiler.h>
#include <lk/list.h>
#include <stdint.h>
#include <sys/types.h>

__BEGIN_CDECLS

#define IPV4(a,b,c,d) (((a)&0xFF)|(((b)&0xFF)<<8)|(((c)&0xFF)<<16)|(((d)&0xFF)<<24))
#define IPV4_SPLIT(a) (a & 0xFF), ((a >> 8) & 0xFF), ((a >> 16) & 0xFF), ((a >> 24) & 0xFF)
#define IPV4_PACK(a) (a[3] << 24 | a[2] << 16 | a[1] << 8 | a[0])
#define IPV4_BCAST (0xFFFFFFFF)
#define IPV4_NONE (0)

typedef int (*tx_func_t)(void *arg, pktbuf_t *p);
typedef void (*udp_callback_t)(void *data, size_t len,
                               uint32_t srcaddr, uint16_t srcport, void *arg);

/* initialize and start minip with static configuration */
void minip_start_static(uint32_t ip, uint32_t netmask, uint32_t gateway);

/* initialize and start minip with DHCP configuration
 * note: may take a while to have an ip address assigned, check
 * for configuration with minip_is_configured()
 */
void minip_start_dhcp(void);

/* ethernet driver install hook */
void minip_set_eth(tx_func_t tx_handler, void *tx_arg, const uint8_t *macaddr);

/* check or wait for minip to be configured */
bool minip_is_configured(void);
status_t minip_wait_for_configured(lk_time_t timeout);

/* packet rx hook to hand to ethernet driver */
void minip_rx_driver_callback(pktbuf_t *p);

/* global configuration state */
void minip_get_macaddr(uint8_t *addr);
void minip_set_macaddr(const uint8_t *addr);

uint32_t minip_get_ipaddr(void);
void minip_set_ipaddr(const uint32_t addr);
uint32_t minip_get_netmask(void);
void minip_set_netmask(const uint32_t mask);
uint32_t minip_get_broadcast(void); // computed from ipaddr & netmask
uint32_t minip_get_gateway(void);
void minip_set_gateway(const uint32_t addr);
void minip_set_hostname(const char *name);
const char *minip_get_hostname(void);
void minip_set_configured(void); // set by dhcp or static init to signal minip is ready to be used

/* udp */
typedef struct udp_socket udp_socket_t;

int udp_listen(uint16_t port, udp_callback_t cb, void *arg);
status_t udp_open(uint32_t host, uint16_t sport, uint16_t dport, udp_socket_t **handle);
status_t udp_send(void *buf, size_t len, udp_socket_t *handle);
status_t udp_send_iovec(const iovec_t *iov, uint iov_count, udp_socket_t *handle);
status_t udp_close(udp_socket_t *handle);

/* tcp */
typedef struct tcp_socket tcp_socket_t;

status_t tcp_connect(tcp_socket_t **handle, uint32_t addr, uint16_t port);
status_t tcp_open_listen(tcp_socket_t **handle, uint16_t port);
status_t tcp_accept_timeout(tcp_socket_t *listen_socket, tcp_socket_t **accept_socket, lk_time_t timeout);
status_t tcp_close(tcp_socket_t *socket);
ssize_t tcp_read(tcp_socket_t *socket, void *buf, size_t len);
ssize_t tcp_write(tcp_socket_t *socket, const void *buf, size_t len);

static inline status_t tcp_accept(tcp_socket_t *listen_socket, tcp_socket_t **accept_socket) {
    return tcp_accept_timeout(listen_socket, accept_socket, INFINITE_TIME);
}

/* utilities */
void gen_random_mac_address(uint8_t *mac_addr);
uint32_t minip_parse_ipaddr(const char *addr, size_t len);
void printip(uint32_t x);
void printip_named(const char *s, u32 x);

__END_CDECLS
