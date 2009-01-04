/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
 * Copyright (c) 2008 Travis Geiselbrecht
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
#include <debug.h>
#include <stdlib.h>
#include <rand.h>
#include <string.h>
#include <err.h>
#include <compiler.h>
#include <lib/net/hash.h>
#include <lib/net/udp.h>
#include <lib/net/ipv4.h>
#include <lib/net/misc.h>
#include <kernel/mutex.h>
#include <kernel/event.h>

typedef struct udp_header {
	uint16_t source_port;
	uint16_t dest_port;
	uint16_t length;
	uint16_t checksum;
} __PACKED udp_header;

typedef struct udp_pseudo_header {
	ipv4_addr source_addr;
	ipv4_addr dest_addr;
	uint8_t zero;
	uint8_t protocol;
	uint16_t udp_length;
} __PACKED udp_pseudo_header;

typedef struct udp_queue_elem {
	struct udp_queue_elem *next;
	struct udp_queue_elem *prev;
	ipv4_addr src_address;
	ipv4_addr target_address;
	uint16_t src_port;
	uint16_t target_port;
	int len;
	cbuf *buf;
} udp_queue_elem;

typedef struct udp_queue {
	udp_queue_elem *next;
	udp_queue_elem *prev;
	int count;
} udp_queue;

typedef struct udp_endpoint {
	struct udp_endpoint *next;
	mutex_t lock;
	event_t blocking_event;
	uint16_t port;
	udp_queue q;
	int ref_count;
} udp_endpoint;

static udp_endpoint *endpoints;
static mutex_t endpoints_lock;
static int next_ephemeral_port;

static int udp_endpoint_compare_func(void *_e, const void *_key)
{
	udp_endpoint *e = _e;
	const uint16_t *port = _key;

	if(e->port == *port)
		return 0;
	else
		return 1;
}

static unsigned int udp_endpoint_hash_func(void *_e, const void *_key, unsigned int range)
{
	udp_endpoint *e = _e;
	const uint16_t *port = _key;

	if(e)
		return e->port % range;
	else
		return *port % range;
}

static void udp_init_queue(udp_queue *q)
{
	q->count = 0;
	q->next = q->prev = (udp_queue_elem *)q;
}

static udp_queue_elem *udp_queue_pop(udp_queue *q)
{
	if(q->next != (udp_queue_elem *)q) {
		udp_queue_elem *e = q->next;

		q->next = e->next;
		e->next->prev = (udp_queue_elem *)q;
		q->count--;
		e->next = e->prev = NULL;
		return e;
	} else {
		return NULL;
	}
}

static void udp_queue_push(udp_queue *q, udp_queue_elem *e)
{
	e->prev = q->prev;
	e->next = (udp_queue_elem *)q;
	q->prev->next = e;
	q->prev = e;
	q->count++;
}

static void udp_endpoint_acquire_ref(udp_endpoint *e)
{
	atomic_add(&e->ref_count, 1);
}

static void udp_endpoint_release_ref(udp_endpoint *e)
{
	if(atomic_add(&e->ref_count, -1) == 1) {
		udp_queue_elem *qe;

		mutex_destroy(&e->lock);
		event_destroy(&e->blocking_event);

		// clear out the queue of packets
		for(qe = udp_queue_pop(&e->q); qe; qe = udp_queue_pop(&e->q)) {
			if(qe->buf)
				cbuf_free_chain(qe->buf);
			free(qe);
		}
	}
}

static int udp_allocate_ephemeral_port(void)
{
	return atomic_add(&next_ephemeral_port, 1) % 0x10000;
}

int udp_input(cbuf *buf, ifnet *i, ipv4_addr source_address, ipv4_addr target_address)
{
	udp_header *header;
	udp_endpoint *e;
	udp_queue_elem *qe;
	uint16_t port;
	int err;

	header = cbuf_get_ptr(buf, 0);

#if NET_CHATTY
	TRACEF("src port %d, dest port %d, len %d, buf len %d, checksum 0x%x\n",
		ntohs(header->source_port), ntohs(header->dest_port), ntohs(header->length), (int)cbuf_get_len(buf), ntohs(header->checksum));
#endif
	if(ntohs(header->length) > (uint16_t)cbuf_get_len(buf)) {
		err = ERR_NET_BAD_PACKET;
		goto ditch_packet;
	}

	// deal with the checksum check
	if(header->checksum) {
		udp_pseudo_header pheader;
		uint16_t checksum;

		// set up the pseudo header for checksum purposes
		pheader.source_addr = htonl(source_address);
		pheader.dest_addr = htonl(target_address);
		pheader.zero = 0;
		pheader.protocol = IP_PROT_UDP;
		pheader.udp_length = header->length;

		checksum = cbuf_ones_cksum16_2(buf, 0, ntohs(header->length), &pheader, sizeof(pheader));
		if(checksum != 0) {
#if NET_CHATTY
			TRACEF("packet failed checksum\n");
#endif
			err = ERR_NET_BAD_PACKET;
			goto ditch_packet;
		}
	}

	// see if we have an endpoint
	port = ntohs(header->dest_port);
	mutex_acquire(&endpoints_lock);
	e = hash_lookup(endpoints, &port);
	if(e)
		udp_endpoint_acquire_ref(e);
	mutex_release(&endpoints_lock);

	if(!e) {
		err = NO_ERROR;
		goto ditch_packet;
	}

	// okay, we have an endpoint, lets queue our stuff up and move on
	qe = malloc(sizeof(udp_queue_elem));
	if(!qe) {
		udp_endpoint_release_ref(e);
		err = ERR_NO_MEMORY;
		goto ditch_packet;
	}
	qe->src_port = ntohs(header->source_port);
	qe->target_port = port;
	qe->src_address = source_address;
	qe->target_address = target_address;
	qe->len = ntohs(header->length) - sizeof(udp_header);

	// trim off the udp header
	buf = cbuf_truncate_head(buf, sizeof(udp_header), true);
	qe->buf = buf;

	mutex_acquire(&e->lock);
	udp_queue_push(&e->q, qe);
	mutex_release(&e->lock);

	event_signal(&e->blocking_event, true);

	udp_endpoint_release_ref(e);

	err = NO_ERROR;
	return err;

ditch_packet:
	cbuf_free_chain(buf);

	return err;
}

int udp_open(void **prot_data)
{
	udp_endpoint *e;

	e = malloc(sizeof(udp_endpoint));
	if(!e)
		return ERR_NO_MEMORY;

	mutex_init(&e->lock);
	event_init(&e->blocking_event, false, EVENT_FLAG_AUTOUNSIGNAL);
	e->port = 0;
	e->ref_count = 1;
	udp_init_queue(&e->q);

	mutex_acquire(&endpoints_lock);
	hash_insert(endpoints, e);
	mutex_release(&endpoints_lock);

	*prot_data = e;

	return 0;
}

static int _udp_bind(udp_endpoint *e, int port)
{
	int err;

	mutex_acquire(&e->lock);

	if(e->port == 0) {

		// make up a port number if one isn't passed in
		if (port == 0)
			port = udp_allocate_ephemeral_port();

		TRACEF("setting endprint %p to port %d\n", e, port);

		if(port != e->port) {

			// XXX search to make sure this port isn't used already

			// remove it from the hashtable, stick it back with the new port
			mutex_acquire(&endpoints_lock);
			hash_remove(endpoints, e);
			e->port = port;
			hash_insert(endpoints, e);
			mutex_release(&endpoints_lock);
		}
		err = NO_ERROR;
	} else {
		err = ERR_NET_SOCKET_ALREADY_BOUND;
	}

	mutex_release(&e->lock);

	return err;
}

int udp_bind(void *prot_data, sockaddr *addr)
{
	udp_endpoint *e = prot_data;

	// XXX does not support binding src ip address
	return _udp_bind(e, addr->port);
}

int udp_connect(void *prot_data, sockaddr *addr)
{
	return ERR_NOT_ALLOWED;
}

int udp_listen(void *prot_data)
{
	return ERR_NOT_ALLOWED;
}

int udp_accept(void *prot_data, sockaddr *addr, void **new_socket)
{
	return ERR_NOT_ALLOWED;
}

int udp_close(void *prot_data)
{
	udp_endpoint *e = prot_data;

	mutex_acquire(&endpoints_lock);
	hash_remove(endpoints, e);
 	mutex_release(&endpoints_lock);

	udp_endpoint_release_ref(e);

	return 0;
}

ssize_t udp_recvfrom(void *prot_data, void *buf, ssize_t len, sockaddr *saddr, int flags, time_t timeout)
{
	udp_endpoint *e = prot_data;
	udp_queue_elem *qe;
	int err;
	ssize_t ret;

retry:
	if(flags & SOCK_FLAG_TIMEOUT)
		err = event_wait_timeout(&e->blocking_event, timeout);
	else
		err = event_wait(&e->blocking_event);
	if(err < 0)
		return err;

	// pop an item off the list, if there are any
	mutex_acquire(&e->lock);
	qe = udp_queue_pop(&e->q);
	mutex_release(&e->lock);

	if(!qe)
		goto retry;

	// we have the data, copy it out
	err = cbuf_memcpy_from_chain(buf, qe->buf, 0, MIN(qe->len, len));
	if(err < 0) {
		ret = err;
		goto out;
	}
	ret = qe->len;

	// copy the address out
	if(saddr) {
		saddr->addr.len = 4;
		saddr->addr.type = ADDR_TYPE_IP;
		NETADDR_TO_IPV4(saddr->addr) = qe->src_address;
		saddr->port = qe->src_port;
	}

out:
	// free this queue entry
	cbuf_free_chain(qe->buf);
	free(qe);

	return ret;
}

ssize_t udp_sendto(void *prot_data, const void *inbuf, ssize_t len, sockaddr *toaddr)
{
	udp_endpoint *e = prot_data;
	udp_header *header;
	int total_len;
	cbuf *buf;
	udp_pseudo_header pheader;
	ipv4_addr srcaddr;
	int err;

	// make sure the args make sense
	if(len < 0 || len + sizeof(udp_header) > 0xffff)
		return ERR_INVALID_ARGS;
	if(toaddr->port < 0 || toaddr->port > 0xffff)
		return ERR_INVALID_ARGS;

	// find us a local port if no one has already
	if (e->port == 0)
		_udp_bind(e, 0);

	// allocate a buffer to hold the data + header
	total_len = len + sizeof(udp_header);
	buf = cbuf_get_chain(total_len);
	if(!buf)
		return ERR_NO_MEMORY;

	// copy the data to this new buffer
	err = cbuf_memcpy_to_chain(buf, sizeof(udp_header), inbuf, len);
	if(err < 0) {
		cbuf_free_chain(buf);
		return err;
	}

	// set up the udp pseudo header
	if(ipv4_lookup_srcaddr_for_dest(NETADDR_TO_IPV4(toaddr->addr), &srcaddr) < 0) {
		cbuf_free_chain(buf);
		return ERR_NET_NO_ROUTE;
	}
	pheader.source_addr = htonl(srcaddr);
	pheader.dest_addr = htonl(NETADDR_TO_IPV4(toaddr->addr));
	pheader.zero = 0;
	pheader.protocol = IP_PROT_UDP;
	pheader.udp_length = htons(total_len);

	// start setting up the header
	header = cbuf_get_ptr(buf, 0);
	header->source_port = htons(e->port);
	header->dest_port = htons(toaddr->port);
	header->length = htons(total_len);
	header->checksum = 0;
	header->checksum = cbuf_ones_cksum16_2(buf, 0, total_len, &pheader, sizeof(pheader));
	if(header->checksum == 0)
		header->checksum = 0xffff;

	// send it away
	err = ipv4_output(buf, NETADDR_TO_IPV4(toaddr->addr), IP_PROT_UDP);

	// if it returns ARP_QUEUED, then it's actually okay
	if(err == ERR_NET_ARP_QUEUED) {
		err = 0;
	}

	return err;
}

int udp_init(void)
{
	mutex_init(&endpoints_lock);

	next_ephemeral_port = rand() % 32000 + 1024;

	endpoints = hash_init(256, offsetof(udp_endpoint, next), 
		&udp_endpoint_compare_func, &udp_endpoint_hash_func);
	if(!endpoints)
		return ERR_NO_MEMORY;

	return 0;
}

