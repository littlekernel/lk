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

int if_register_interface(const char *path, ifnet **_i)
{
	ifnet *i;
	int type;
	int err;
	ifaddr *address;

	i = malloc(sizeof(ifnet));
	if(!i) {
		err = ERR_NO_MEMORY;
		goto err;
	}
	memset(i, 0, sizeof(ifnet));

	/* open the device */
	if(!strcmp(path, "loopback")) {
		// the 'loopback' device is special
		type = IF_TYPE_LOOPBACK;
		i->fd = -1;
	} else {
		i->fd = sys_open(path, 0);
		if(i->fd < 0) {
			err = i->fd;
			goto err1;
		}
		/* find the device's type */
		err = sys_ioctl(i->fd, IOCTL_NET_IF_GET_TYPE, &type, sizeof(type));
		if(err < 0) {
			goto err2;
		}
	}

	// find the appropriate function calls to the link layer drivers
	switch(type) {
		case IF_TYPE_LOOPBACK:
			i->link_input = &loopback_input;
			i->link_output = &loopback_output;
			i->mtu = 65535;
			break;
		case IF_TYPE_ETHERNET:
			i->link_input = &ethernet_input;
			i->link_output = &ethernet_output;
			i->mtu = ETHERNET_MAX_SIZE - ETHERNET_HEADER_SIZE;

			/* bind the ethernet link address */
			address = malloc(sizeof(ifaddr));
			address->addr.len = 6;
			address->addr.type = ADDR_TYPE_ETHERNET;
			err = sys_ioctl(i->fd, IOCTL_NET_IF_GET_ADDR, &address->addr.addr[0], 6);
			if(err < 0) {
				free(address);
				goto err2;
			}
			address->broadcast.len = 6;
			address->broadcast.type = ADDR_TYPE_ETHERNET;
			memset(&address->broadcast.addr[0], 0xff, 6);
			address->netmask.type = ADDR_TYPE_NULL;
			if_bind_link_address(i, address);
			break;
		default:
			err = ERR_NET_GENERAL;
			goto err1;
	}

	i->id = atomic_add(&next_id, 1);
	strlcpy(i->path, path, sizeof(i->path));
	i->type = type;
	i->rx_thread = -1;
	i->tx_thread = -1;
	i->tx_queue_sem = sem_create(0, "tx_queue_sem");
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
	sys_close(i->fd);
err1:
	free(i);
err:
	return err;
}
#endif

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

	if(i->fd < 0)
		return -1;

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

#if NET_CHATTY
		TRACEF("sending packet size %Ld\n", (long long)len);
#endif
			// XXX fix
//			sys_write(i->fd, i->tx_buf, 0, len);
		}
	}
}

static int if_rx_thread(void *args)
{
	ifnet *i = args;
	cbuf *b;

	if(i->fd < 0)
		return -1;

	for(;;) {
		ssize_t len;

//		len = sys_read(i->fd, i->rx_buf, 0, sizeof(i->rx_buf));
		len  = -1; // XXX fix
#if NET_CHATTY
		TRACEF("got ethernet packet, size %Ld\n", (long long)len);
#endif
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
#if NET_CHATTY
			TRACEF("dumping packet because of no link address (%p)\n", i);
#endif
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
	i->rx_thread = thread_create("net_rx_thread", &if_rx_thread, i, HIGHEST_PRIORITY - 2, DEFAULT_STACK_SIZE);

	// create the transmit thread
	i->tx_thread = thread_create("net_tx_thread", &if_tx_thread, i, HIGHEST_PRIORITY - 2, DEFAULT_STACK_SIZE);

	// start the threads
	thread_resume(i->rx_thread);
	thread_resume(i->tx_thread);

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

