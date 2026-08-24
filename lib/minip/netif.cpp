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

#define LOCAL_TRACE 0

static struct list_node netif_list = LIST_INITIAL_VALUE(netif_list);
static mutex_t lock = MUTEX_INITIAL_VALUE(lock);
static netif_t loopback;

static int loopback_tx_func(void *arg, pktbuf_t *p) {
    netif_t *n = (netif_t *)arg;

    LTRACEF("netif %p, pkt %p, dlen %u\n", n, p, p->dlen);

    /* Reflect the frame back into the stack. The receive path mutates
     * frames in place (header byte swaps, consumes) and the transmit side
     * may hold on to p (retransmit queues), so the receive side gets its
     * own copy.
     */
    status_t err = minip_rx_driver_callback_copy(n, p->data, p->dlen);
    pktbuf_free(p, true);

    return (err < 0) ? err : 0;
}

void netif_init(void) {
    LTRACE;

    // loopback device
    static const uint8_t loopback_mac[6] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
    netif_create(&loopback, "loopback");
    loopback.flags |= NETIF_FLAG_LOOPBACK;
    netif_set_eth(&loopback, loopback_tx_func, &loopback, loopback_mac);
    netif_set_ipv4_addr(&loopback, IPV4(127, 0, 0, 1), 8);
    netif_register(&loopback);
}

netif_t *netif_create(netif_t *n, const char *name) {
    LTRACEF("n %p\n", n);

    if (!n) {
        n = (netif_t *)malloc(sizeof(netif_t));
    }
    if (!n) {
        return NULL;
    }

    memset(n, 0, sizeof(*n));

    n->magic = NETIF_MAGIC;
    mutex_init(&n->tx_lock);
    strlcpy(n->name, name, sizeof(n->name));

    return n;
}

// generic logic to decide what to do when a netif comes up
// TODO: make this overridable
void netif_registration_callback(netif_t *n) {
    // if the interface isn't already configured, kick off a dhcp thread
    if ((n->flags & NETIF_FLAG_IPV4_CONFIGURED) == 0) {
        minip_start_dhcp(n);
    }
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

    mutex_release(&lock);

    // let overridable external logic deal with this
    netif_registration_callback(n);

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

    // set an ipv4 route for this
    ipv4_add_route(netif_get_network_ipv4(n), netif_get_netmask_ipv4(n), n);

    return NO_ERROR;
}

status_t netif_tx(netif_t *n, pktbuf_t *p) {
    DEBUG_ASSERT(n->magic == NETIF_MAGIC);
    DEBUG_ASSERT(n->tx_func);

    mutex_acquire(&n->tx_lock);
    status_t err = n->tx_func(n->tx_func_arg, p);
    mutex_release(&n->tx_lock);

    return err;
}

void netif_dump(void) {
    mutex_acquire(&lock);

    netif_t *n;
    list_for_every_entry(&netif_list, n, netif_t, node) {
        printf("net interface @%p: name '%s' mac ", n, n->name);
        print_mac_address(n->mac_address);
        printf(" addr ");
        print_ipv4_address(n->ipv4_addr);
        printf("/%u", n->ipv4_subnet_width);
        printf(" netmask ");
        print_ipv4_address(netif_get_netmask_ipv4(n));
        printf(" bcast ");
        print_ipv4_address(netif_get_broadcast_ipv4(n));
        printf("\n");
    }

    mutex_release(&lock);
}
