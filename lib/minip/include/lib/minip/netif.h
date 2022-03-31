/*
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <lib/minip.h>

// Network interface layer for minip

// Per network interface structure, created by the network driver and registered
// with the net stack.
struct netif {
    uint32_t magic;
    struct list_node node;

    uint32_t flags;

    // mac address
    uint8_t mac_address[6];

    // ipv4 address
    ipv4_addr_t ipv4_addr;
    uint8_t ipv4_subnet_width;

    // driver tx routine
    tx_func_t tx_func;
    void *tx_func_arg;

    // name
    char name[32];
};
typedef struct netif netif_t;
#define NETIF_MAGIC 'NETI'

#define NETIF_FLAG_ETH_CONFIGURED  (1U << 0) // mac address and tx func set
#define NETIF_FLAG_REGISTERED      (1U << 1) // added to the main list
#define NETIF_FLAG_IPV4_CONFIGURED (1U << 2) // ipv4 address is set

// Initialize a netif struct.
// Allocates a new one if passed in pointer is null.
netif_t *netif_create(netif_t *n, const char *name);

status_t netif_set_eth(netif_t *n, tx_func_t tx_handler, void *tx_arg, const uint8_t *macaddr);
status_t netif_set_ipv4_addr(netif_t *n, ipv4_addr_t addr, uint8_t subnet_width);
status_t netif_register(netif_t *n);

// construct netmask and broadcast addresses dynamically
static inline ipv4_addr_t netif_get_netmask_ipv4(netif_t *n) {
    return n->ipv4_addr & (0xffffffff >> n->ipv4_subnet_width);
}
static inline ipv4_addr_t netif_get_broadcast_ipv4(netif_t *n) {
    return (n->ipv4_addr & netif_get_netmask_ipv4(n)) | (0xffffffff << (32 - n->ipv4_subnet_width));
}

// TODO: add temporary hack to remember the first real NIC as the 'main' one and replace global
// stuff in minip.c with a lookup for this
