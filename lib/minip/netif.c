/*
 * Copyright (c) 2022 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */


#include <lib/minip/netif.h>

#include <lk/err.h>
#include <lk/list.h>
#include <lk/trace.h>
#include <lib/minip.h>
#include <assert.h>
#include <stdlib.h>
#include <kernel/mutex.h>

#include "minip-internal.h"

#define LOCAL_TRACE 1

static struct list_node netif_list = LIST_INITIAL_VALUE(netif_list);
static mutex_t lock = MUTEX_INITIAL_VALUE(lock);
static netif_t loopback;

// Used by the ip layer to directly get a handle to the loopback and first
// registered interface (main).
// TODO: remove once proper routing is in place
netif_t *netif_loopback = NULL;
netif_t *netif_main = NULL;

static int loopback_tx_func(void *arg, pktbuf_t *p) {
    LTRACEF("arg %p, pkt %p\n", arg, p);
    return 0;
}

void netif_init(void) {
    LTRACE;

    // loopback device
    netif_create(&loopback, "loopback");
    netif_set_eth(&loopback, loopback_tx_func, NULL, bcast_mac);
    netif_set_ipv4_addr(&loopback, IPV4(128, 0, 0, 1), 8);
    netif_register(&loopback);

    // publish for the main stack to find
    netif_loopback = &loopback;
}

netif_t *netif_create(netif_t *n, const char *name) {
    LTRACEF("n %p\n", n);

    if (!n) {
        n = malloc(sizeof(netif_t));
    }
    if (!n) {
        return n;
    }

    memset(n, 0, sizeof(*n));

    n->magic = NETIF_MAGIC;
    strlcpy(n->name, name, sizeof(n->name));

    return n;
}

status_t netif_register(netif_t *n) {
    LTRACEF("n %p\n", n);

    DEBUG_ASSERT(n->magic == NETIF_MAGIC);

    mutex_acquire(&lock);

    // check that it is at least configured
    DEBUG_ASSERT(n->flags & NETIF_FLAG_ETH_CONFIGURED);
    DEBUG_ASSERT((n->flags & NETIF_FLAG_REGISTERED) == 0);

    list_add_head(&netif_list, &n->node);
    n->flags |= NETIF_FLAG_REGISTERED;

    // TODO: replace with mechanism to set up ip routing (probably in DHCP)
    if (netif_main == NULL && n != &loopback) {
        // register this as the 'main' interface
        netif_main = n;
    }

    mutex_release(&lock);

    return NO_ERROR;
}

status_t netif_set_eth(netif_t *n, tx_func_t tx_handler, void *tx_arg, const uint8_t *macaddr) {
    DEBUG_ASSERT(n->magic == NETIF_MAGIC);
    DEBUG_ASSERT(tx_handler);

    mutex_acquire(&lock);

    // must not have been already configured
    DEBUG_ASSERT((n->flags & NETIF_FLAG_ETH_CONFIGURED) == 0);

    // set the state and mark configured
    mac_addr_copy(n->mac_address, macaddr);
    n->tx_func = tx_handler;
    n->tx_func_arg = tx_arg;
    n->flags |= NETIF_FLAG_ETH_CONFIGURED;

    mutex_release(&lock);

    return NO_ERROR;
}

status_t netif_set_ipv4_addr(netif_t *n, ipv4_addr_t addr, uint8_t subnet_width) {
    DEBUG_ASSERT(n->magic == NETIF_MAGIC);

    mutex_acquire(&lock);

    n->ipv4_addr = addr;
    n->ipv4_subnet_width = subnet_width;
    n->flags |= NETIF_FLAG_IPV4_CONFIGURED;

    mutex_release(&lock);

    return NO_ERROR;
}

void netif_dump(void) {
    mutex_acquire(&lock);

    netif_t *n;
    list_for_every_entry(&netif_list, n, netif_t, node) {
        printf("net interface @%p: name '%s' mac ", n, n->name);
        print_mac_address(n->mac_address);
        printf(" addr ");
        printip(n->ipv4_addr);
        printf("/%u", n->ipv4_subnet_width);
        printf(" netmask ");
        printip(netif_get_netmask_ipv4(n));
        printf(" bcast ");
        printip(netif_get_broadcast_ipv4(n));
        printf("\n");
    }

    mutex_release(&lock);
}
