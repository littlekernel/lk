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
#include <lib/net/loopback.h>
#include <lib/net/ethernet.h>
#include <lib/net/if.h>
#include <lib/net/hash.h>
#include <lib/net/queue.h>
#include <platform.h>

#if NET_CHATTY
#define LOCAL_TRACE 1
#else
#define LOCAL_TRACE 0
#endif

#define TX_QUEUE_SIZE 64

#define LOSE_RX_PACKETS 0
#define LOSE_RX_PERCENTAGE 5

#define LOSE_TX_PACKETS 0
#define LOSE_TX_PERCENTAGE 5

static void *ifhash;
static mutex_t ifhash_lock;
static if_id next_id;

static int if_compare_func(void *_i, const void *_key)
{
	struct ifnet *i = _i;
	const if_id *id = _key;

	if(i->id == *id) return 0;
	else return 1;
}

static unsigned int if_hash_func(void *_i, const void *_key, unsigned int range)
{
	struct ifnet *i = _i;
	const if_id *id = _key;

	if(i)
		return (i->id % range);
	else
		return (*id % range);
}

ifnet *if_id_to_ifnet(if_id id)
{
	ifnet *i;

	mutex_acquire(&ifhash_lock);
	i = hash_lookup(ifhash, &id);
	mutex_release(&ifhash_lock);

	return i;
}

#if 0
ifnet *if_path_to_ifnet(const char *path)
{
	ifnet *i;
	struct hash_iterator iter;

	mutex_acquire(&ifhash_lock);
	hash_open(ifhash, &iter);
	while((i = hash_next(ifhash, &iter)) != NULL) {
		if(!strcmp(path, i->path))
			break;
	}
	hash_close(ifhash, &iter, false);
	mutex_release(&ifhash_lock);

	return i;
}
#endif

int if_register_interface(const ifhook *hook, ifnet **_i)
{
	ifnet *i;
	int err;
	ifaddr *address;

	i = malloc(sizeof(ifnet));
	if(!i) {
		err = ERR_NO_MEMORY;
		goto err;
	}
	memset(i, 0, sizeof(ifnet));

	i->hook = hook;	

	// find the appropriate function calls to the link layer drivers
	switch (hook->type) {
		case IF_TYPE_LOOPBACK:
			i->link_input = &loopback_input;
			i->link_output = &loopback_output;
			break;
		case IF_TYPE_ETHERNET:
			i->link_input = &ethernet_input;
			i->link_output = &ethernet_output;

			/* bind the link address */
			address = malloc(sizeof(ifaddr));

			memcpy(&address->addr, &hook->linkaddr, sizeof(netaddr));

			address->broadcast.len = 6;
			address->broadcast.type = ADDR_TYPE_ETHERNET;
			memset(&address->broadcast.addr[0], 0xff, 6);
			address->netmask.type = ADDR_TYPE_NULL;
			if_bind_link_address(i, address);
			break;
		default:
			err = ERR_INVALID_ARGS;
			goto err1;
	}

	i->id = atomic_add(&next_id, 1);
	i->type = hook->type;
	i->rx_thread = NULL;
	i->tx_thread = NULL;
	event_init(&i->tx_queue_event, false, 0);
	mutex_init(&i->tx_queue_lock);
	fixed_queue_init(&i->tx_queue, TX_QUEUE_SIZE);

	mutex_acquire(&ifhash_lock);
	hash_insert(ifhash, i);
	mutex_release(&ifhash_lock);

	/* start the rx and tx threads on this interface */
	err = if_boot_interface(i);
	if(err < 0)
		goto err2;

	*_i = i;

	return NO_ERROR;

err2:
	event_destroy(&i->tx_queue_event);
	mutex_destroy(&i->tx_queue_lock);
err1:
	free(i);
err:
	return err;
}

void if_bind_address(ifnet *i, ifaddr *addr)
{
	addr->if_owner = i;
	addr->next = i->addr_list;
	i->addr_list = addr;
}

void if_bind_link_address(ifnet *i, ifaddr *addr)
{
	i->link_addr = addr;
}

int if_output(cbuf *b, ifnet *i)
{
	bool signal_event = false;
	bool enqueue_failed = false;

	// stick the buffer on a transmit queue
	mutex_acquire(&i->tx_queue_lock);
	if(fixed_queue_enqueue(&i->tx_queue, b) < 0)
		enqueue_failed = true;
	if(i->tx_queue.count == 1)
		signal_event = true;
	mutex_release(&i->tx_queue_lock);

	if(enqueue_failed) {
		cbuf_free_chain(b);
		return ERR_NO_MEMORY;
	}

	if(signal_event)
		event_signal(&i->tx_queue_event, true);

	return NO_ERROR;
}

static int if_tx_thread(void *args)
{
	ifnet *i = args;
	cbuf *buf;
	ssize_t len;

	for(;;) {
		event_wait(&i->tx_queue_event);

		for(;;) {
	 		// pull a packet out of the queue
			mutex_acquire(&i->tx_queue_lock);
			buf = fixed_queue_dequeue(&i->tx_queue);
			mutex_release(&i->tx_queue_lock);
			if(!buf)
				break;

#if LOSE_TX_PACKETS
			if(rand() % 100 < LOSE_TX_PERCENTAGE) {
				cbuf_free_chain(buf);
				continue;
			}
#endif

			// put the cbuf chain into a flat buffer
			len = cbuf_get_len(buf);
			cbuf_memcpy_from_chain(i->tx_buf, buf, 0, len);

			cbuf_free_chain(buf);

			LTRACEF("sending packet size %u\n", len);
			i->hook->if_output(i->hook->cookie, i->tx_buf, len);
		}
	}

	return 0;
}

static int if_rx_thread(void *args)
{
	ifnet *i = args;
	cbuf *b;

	for(;;) {
		ssize_t len;

		len = i->hook->if_input(i->hook->cookie, i->rx_buf, sizeof(i->rx_buf));

		LTRACEF("got packet, size %d\n", len);

		if(len < 0) {
			thread_sleep(10);
			continue;
		}
		if(len == 0)
			continue;

#if LOSE_RX_PACKETS
		if(rand() % 100 < LOSE_RX_PERCENTAGE) {
			TRACEF("purposely lost packet, size %d\n", len);
			continue;
		}
#endif

		// check to see if we have a link layer address attached to us
		if(!i->link_addr) {
			LTRACEF("dumping packet because of no link address (%p)\n", i);
			continue;
		}

		// for now just move it over into a cbuf
		b = cbuf_get_chain(len);
		if(!b) {
			TRACEF("could not allocate cbuf to hold ethernet packet\n");
			continue;
		}
		cbuf_memcpy_to_chain(b, 0, i->rx_buf, len);

		i->link_input(b, i);
	}

	return 0;
}

int if_boot_interface(ifnet *i)
{
	// create the receive thread
	if (i->hook->if_input) {
		i->rx_thread = thread_create("net_rx_thread", &if_rx_thread, i, HIGHEST_PRIORITY - 2, DEFAULT_STACK_SIZE);
		thread_resume(i->rx_thread);
	}

	// create the transmit thread
	if (i->hook->if_output) {
		i->tx_thread = thread_create("net_tx_thread", &if_tx_thread, i, HIGHEST_PRIORITY - 2, DEFAULT_STACK_SIZE);
		thread_resume(i->tx_thread);
	}

	return NO_ERROR;
}

int if_init(void)
{
	next_id = 0;

	// create a hash table to store the interface list
	ifhash = hash_init(16, offsetof(ifnet, next),
		&if_compare_func, &if_hash_func);
	mutex_init(&ifhash_lock);

	return NO_ERROR;
}

