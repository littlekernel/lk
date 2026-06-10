/*
 * Copyright (c) 2014 Chris Anderson
 * Copyright (c) 2014 Brian Swetland
 * Copyright (c) 2015 Nathaniel Quillin
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "minip-internal.h"

#include <lk/err.h>
#include <errno.h>
#include <iovec.h>
#include <lk/list.h>
#include <malloc.h>
#include <stdint.h>
#include <lk/trace.h>

#define LOCAL_TRACE 0

static struct list_node udp_list = LIST_INITIAL_VALUE(udp_list);

struct udp_listener {
    struct list_node list;
    uint16_t port;
    udp_callback_t callback;
    void *arg;
};

typedef struct udp_socket {
    ipv4_addr_t local_ip;
    ipv4_addr_t remote_ip;
    uint16_t sport;
    uint16_t dport;
    ipv4_route_t *route;

    // only set if bound to a particular network interface
    netif_t *netif;
} udp_socket_t;

typedef struct udp_hdr {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t chksum;
} __PACKED udp_hdr_t;

int udp_listen(uint16_t port, udp_callback_t cb, void *arg) {
    struct udp_listener *entry, *temp;

    list_for_every_entry_safe(&udp_list, entry, temp, struct udp_listener, list) {
        if (entry->port == port) {
            if (cb == NULL) {
                list_delete(&entry->list);
                return 0;
            }
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

status_t udp_open(ipv4_addr_t host, uint16_t sport, uint16_t dport, udp_socket_t **handle) {
    LTRACEF("host %u.%u.%u.%u sport %u dport %u handle %p\n",
            IPV4_SPLIT(host), sport, dport, handle);
    udp_socket_t *socket;

    if (handle == NULL) {
        return -EINVAL;
    }

    socket = (udp_socket_t *) calloc(1, sizeof(udp_socket_t));
    if (!socket) {
        return -ENOMEM;
    }

    // look up route to set local address
    ipv4_route_t *route = ipv4_search_route(host);
    if (!route) {
        return ERR_NO_ROUTE;
    }

    netif_t *netif = route->interface;

    socket->local_ip = netif->ipv4_addr;
    socket->remote_ip = host;
    socket->sport = sport;
    socket->dport = dport;
    socket->route = route;

    *handle = socket;

    return NO_ERROR;
}

status_t udp_open_raw(ipv4_addr_t host, uint16_t sport, uint16_t dport, netif_t *netif, udp_socket_t **handle) {
    LTRACEF("host %u.%u.%u.%u sport %u dport %u netif '%s' handle %p\n",
            IPV4_SPLIT(host), sport, dport, netif->name, handle);
    udp_socket_t *socket;

    if (handle == NULL) {
        return -EINVAL;
    }
    if (netif == NULL) {
        return -EINVAL;
    }

    socket = (udp_socket_t *) calloc(1, sizeof(udp_socket_t));
    if (!socket) {
        return -ENOMEM;
    }

    socket->local_ip = netif->ipv4_addr;
    socket->remote_ip = host;
    socket->sport = sport;
    socket->dport = dport;
    socket->netif = netif;

    *handle = socket;

    return NO_ERROR;
}

status_t udp_close(udp_socket_t *handle) {
    if (handle == NULL) {
        return -EINVAL;
    }

    if (handle->route) {
        ipv4_dec_route_ref(handle->route);
    }

    free(handle);
    return NO_ERROR;
}

status_t udp_send_iovec(const iovec_t *iov, uint iov_count, udp_socket_t *handle) {
    pktbuf_t *p;
    udp_hdr_t *udp;
    void *buf;
    ssize_t len;

    if (handle == NULL || iov == NULL || iov_count == 0) {
        return -EINVAL;
    }

    if ((p = pktbuf_alloc()) == NULL) {
        return -ENOMEM;
    }

    len = iovec_size(iov, iov_count);

    buf = pktbuf_append(p, len);

    iovec_to_membuf(buf, len, iov, iov_count, 0);

    udp = pktbuf_prepend(p, sizeof(udp_hdr_t));
    udp->src_port   = htons(handle->sport);
    udp->dst_port   = htons(handle->dport);
    udp->len        = htons(sizeof(udp_hdr_t) + len);
    udp->chksum     = 0;

#if MINIP_USE_UDP_CHECKSUM
    {
        ipv4_pseudo_header_t pheader;
        pheader.source_addr = handle->local_ip;
        pheader.dest_addr = handle->remote_ip;
        pheader.zero = 0;
        pheader.protocol = IP_PROTO_UDP;
        pheader.tcp_length = htons(p->dlen);

        udp->chksum = cksum_pheader(&pheader, p->data, p->dlen);
    }
#endif

    LTRACEF("sending udp packet, len %u\n", p->dlen);

    status_t err;
    if (handle->netif) {
        // XXX: does this always use the broadcast mac?
        err = minip_ipv4_send_raw(p, handle->remote_ip, IP_PROTO_UDP, bcast_mac, handle->netif);
    } else {
        err = minip_ipv4_send(p, handle->remote_ip, IP_PROTO_UDP);
    }

    return err;
}

status_t udp_send(void *buf, size_t len, udp_socket_t *handle) {
    iovec_t iov;

    LTRACEF("buf %p, len %zu, handle %p\n", buf, len, handle);

    if (buf == NULL || len == 0) {
        return -EINVAL;
    }

    iov.iov_base = buf;
    iov.iov_len = len;

    return udp_send_iovec(&iov, 1, handle);
}

void udp_input(netif_t *netif, pktbuf_t *p, uint32_t src_ip) {
    udp_hdr_t *udp;
    struct udp_listener *e;
    uint16_t port;

    if ((udp = pktbuf_consume(p, sizeof(udp_hdr_t))) == NULL) {
        return;
    }

    port = ntohs(udp->dst_port);

    list_for_every_entry(&udp_list, e, struct udp_listener, list) {
        if (e->port == port) {
            e->callback(p->data, p->dlen, src_ip, ntohs(udp->src_port), e->arg);
            return;
        }
    }
}
