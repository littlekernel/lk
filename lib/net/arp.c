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
#include <string.h>
#include <err.h>
#include <compiler.h>
#include <kernel/mutex.h>
#include <kernel/event.h>
#include <lib/net/misc.h>
#include <lib/net/ethernet.h>
#include <lib/net/ipv4.h>
#include <lib/net/arp.h>
#include <lib/net/hash.h>
#include <platform.h>

#define MIN_ARP_SIZE 28

typedef struct arp_packet {
	uint16_t hard_type;
	uint16_t prot_type;
	uint8_t  hard_size;
	uint8_t  prot_size;
	uint16_t op;
	ethernet_addr sender_ethernet;
	ipv4_addr sender_ipv4;
	ethernet_addr target_ethernet;
	ipv4_addr target_ipv4;
} __PACKED arp_packet;

enum {
	ARP_OP_REQUEST = 1,
	ARP_OP_REPLY,
	ARP_OP_RARP_REQUEST,
	ARP_OP_RARP_REPLY
};

enum {
	ARP_HARD_TYPE_ETHERNET = 1
};

typedef struct arp_cache_entry {
	struct arp_cache_entry *next;
	struct arp_cache_entry *all_next;
	ipv4_addr ip_addr;
	netaddr link_addr;
	time_t last_used_time;
} arp_cache_entry;

// arp cache
static void *arp_table;
static mutex_t arp_table_mutex;
static arp_cache_entry *arp_cache_entries;

typedef struct arp_wait_request {
	struct arp_wait_request *next;
	ipv4_addr sender_ipaddr;
	ipv4_addr ip_addr;
	time_t last_attempt_time;
	int attempt_count;
	void (*callback)(int code, void *, ifnet *, netaddr *);
	void *callback_args;
	ifnet *i;
} arp_wait_request;

// list of threads blocked on arp requests
static arp_wait_request *arp_waiters;
static mutex_t arp_wait_mutex;
static event_t arp_wait_event;

static int arp_cache_compare(void *_e, const void *_key)
{
	arp_cache_entry *e = _e;
	const ipv4_addr *addr = _key;

	if(e->ip_addr == *addr)
		return 0;
	else
		return 1;
}

static unsigned int arp_cache_hash(void *_e, const void *_key, unsigned int range)
{
	arp_cache_entry *e = _e;
	const ipv4_addr *key = _key;
	const ipv4_addr *addr;

	if(e)
		addr = &e->ip_addr;
	else
		addr = key;

	// XXX make this smarter
	return ((*addr) ^ (*addr >> 8) ^ (*addr >> 16) ^ (*addr >> 24)) % range;
}

static void dump_arp_packet(arp_packet *arp)
{
#if NET_CHATTY
	printf("arp packet: src ");
	dump_ethernet_addr(arp->sender_ethernet);
	printf(" ");
	dump_ipv4_addr(ntohl(arp->sender_ipv4));
	printf(" dest ");
	dump_ethernet_addr(arp->target_ethernet);
	printf(" ");
	dump_ipv4_addr(ntohl(arp->target_ipv4));
	printf(" op 0x%x\n", ntohs(arp->op));
#endif
}

int arp_input(cbuf *buf, ifnet *i)
{
	arp_packet *arp;

	arp = (arp_packet *)cbuf_get_ptr(buf, 0);

	if(cbuf_get_len(buf) < MIN_ARP_SIZE)
		return -1;

	dump_arp_packet(arp);

	// right now we can only deal with ipv4 arps dealing with ethernet
	if(ntohs(arp->prot_type) != PROT_TYPE_IPV4)
		goto out;
	if(ntohs(arp->hard_type) != ARP_HARD_TYPE_ETHERNET)
		goto out;

	switch(ntohs(arp->op)) {
		case ARP_OP_REPLY: {
			netaddr eth;

			eth.len = 6;
			eth.type = ADDR_TYPE_ETHERNET;
			memcpy(&eth.addr[0], &arp->sender_ethernet, 6);
			arp_insert(ntohl(arp->sender_ipv4), &eth);
			break;
		}
		case ARP_OP_REQUEST: {
			// we have an arp request, see if it matches with this interface
			ifaddr *iaddr = NULL;
			ifaddr *addr;

			// insert an arp entry for the sender of the arp, regardless of if it's
			// for us or not.
			{
				netaddr eth;

				eth.len = 6;
				eth.type = ADDR_TYPE_ETHERNET;
				memcpy(&eth.addr[0], &arp->sender_ethernet, 6);
				arp_insert(ntohl(arp->sender_ipv4), &eth);
			}

			for(addr = i->addr_list; addr; addr = addr->next) {
				if(addr->addr.type == ADDR_TYPE_IP) {
					if(*(ipv4_addr *)&addr->addr.addr[0] == ntohl(arp->target_ipv4)) {
						iaddr = addr;
					}
				}
			}

			if(!iaddr)
				break; // drop it

			if(iaddr) {
				// the arp was for us, lets respond
				memcpy(&arp->target_ethernet, &arp->sender_ethernet, 6);
				arp->target_ipv4 = arp->sender_ipv4;
				memcpy(&arp->sender_ethernet, &i->link_addr->addr.addr[0], 6);
				arp->sender_ipv4 = htonl(*(ipv4_addr *)&iaddr->addr.addr[0]);
				arp->op = htons(ARP_OP_REPLY);

#if NET_CHATTY
				TRACEF("arp was for us, responding...\n");
#endif

				// send it out
				{
					netaddr naddr;
					naddr.len = 6;
					naddr.type = ADDR_TYPE_ETHERNET;
					memcpy(&naddr.addr[0], arp->target_ethernet, 6);
					i->link_output(buf, i, &naddr, PROT_TYPE_ARP);
				}
				// the chain gets deleted in the output, so we can return here
				return 0;
			}
			break;
		}
#if NET_CHATTY
		default:
			TRACEF("unhandled arp request type 0x%x\n", ntohs(arp->op));
#endif
	}

out:
	cbuf_free_chain(buf);

	return 0;
}

static int arp_send_request(ifnet *i, ipv4_addr sender_ipaddr, ipv4_addr ip_addr)
{
	cbuf *buf;
	arp_packet *arp;

	buf = cbuf_get_chain(sizeof(arp_packet));
	if(!buf)
		return ERR_NO_MEMORY;

	arp = (arp_packet *)cbuf_get_ptr(buf, 0);

	arp->hard_type = htons(ARP_HARD_TYPE_ETHERNET);
	arp->prot_type = htons(PROT_TYPE_IPV4);
	arp->hard_size = 6;
	arp->prot_size = 4;
	arp->op = ntohs(ARP_OP_REQUEST);
	memcpy(&arp->sender_ethernet, &i->link_addr->addr.addr[0], 6);
	arp->sender_ipv4 = htonl(sender_ipaddr);
	memset(&arp->target_ethernet, 0, 6);
	arp->target_ipv4 = htonl(ip_addr);

	return i->link_output(buf, i, &i->link_addr->broadcast, PROT_TYPE_ARP);
}

int arp_lookup(ifnet *i, ipv4_addr sender_ipaddr, ipv4_addr ip_addr, netaddr *link_addr, void (*arp_callback)(int, void *args, ifnet *, netaddr *), void *callback_args)
{
	arp_cache_entry *e;
	arp_wait_request *wait;
	bool wakeup_retransmit_thread;

	// if it's a loopback interface, just return, the address wont matter anyway
	if(i->type == IF_TYPE_LOOPBACK)
		return NO_ERROR;

	// look in the arp table first
	mutex_acquire(&arp_table_mutex);
	e = hash_lookup(arp_table, &ip_addr);
	if(e) {
		memcpy(link_addr, &e->link_addr, sizeof(netaddr));
		e->last_used_time = current_time();
	}
	mutex_release(&arp_table_mutex);

	if(e)
		return NO_ERROR;

	if(!arp_callback)
		return ERR_NET_FAILED_ARP;

	// guess we're gonna have to send an arp request and set up a callback
	wait = malloc(sizeof(arp_wait_request));
	if(!wait)
		return ERR_NO_MEMORY;
	wait->sender_ipaddr = sender_ipaddr;
	wait->ip_addr = ip_addr;
	wait->last_attempt_time = current_time();
	wait->attempt_count = 0;
	wait->callback = arp_callback;
	wait->callback_args = callback_args;
	wait->i = i;

	// put this on the list
	wakeup_retransmit_thread = false;
	mutex_acquire(&arp_wait_mutex);
	if(!arp_waiters)
		wakeup_retransmit_thread = true;
	wait->next = arp_waiters;
	arp_waiters = wait;
	mutex_release(&arp_wait_mutex);

	// we just put the first item on the list, wake up the retransmit thread
	if(wakeup_retransmit_thread)
		event_signal(&arp_wait_event, true);

	arp_send_request(i, sender_ipaddr, ip_addr);

	return ERR_NET_ARP_QUEUED;
}

int arp_insert(ipv4_addr ip_addr, netaddr *link_addr)
{
	arp_cache_entry *e;
	arp_wait_request *wait;
	arp_wait_request *removed_waiters;
	arp_wait_request *last;
	arp_wait_request *temp;

#if NET_CHATTY
	printf("arp_insert: ip addr ");
	dump_ipv4_addr(ip_addr);
	printf(" link_addr: type %d len %d addr ", link_addr->type, link_addr->len);
	dump_ethernet_addr(*(ethernet_addr *)&link_addr->addr[0]);
	printf("\n");
#endif

	// see if it's already there
	mutex_acquire(&arp_table_mutex);
	e = hash_lookup(arp_table, &ip_addr);
	if(e) {
		if(e->link_addr.type == link_addr->type) {
			if(cmp_netaddr(&e->link_addr, link_addr)) {
				// replace this entry
				hash_remove(arp_table, e);
				mutex_release(&arp_table_mutex);
				goto use_old_entry;
			}
		}
	}
	mutex_release(&arp_table_mutex);

	// see if it's already there
	if(e)
		return 0;

	e = malloc(sizeof(arp_cache_entry));
	if(!e)
		return ERR_NO_MEMORY;

use_old_entry:
	// create the arp entry
	e->ip_addr = ip_addr;
	memcpy(&e->link_addr, link_addr, sizeof(netaddr));
	e->last_used_time = current_time();

	// insert it into the arp cache
	mutex_acquire(&arp_table_mutex);
	hash_insert(arp_table, e);
	mutex_release(&arp_table_mutex);

	// now, cycle through the arp wait list and do any callbacks we were set to do
	removed_waiters = NULL;
	last = NULL;
	mutex_acquire(&arp_wait_mutex);
	wait = arp_waiters;
	while(wait) {
		if(wait->ip_addr == ip_addr) {
			if(last)
				last->next = wait->next;
			else
				arp_waiters = wait->next;
			temp = wait;
			wait = wait->next;

			// put this item on the just freed list
			temp->next = removed_waiters;
			removed_waiters = temp;
			continue;
		}
		last = wait;
		wait = wait->next;
	}
	mutex_release(&arp_wait_mutex);

	// now we have a list of waiters that were woken up
	while(removed_waiters) {
		temp = removed_waiters;
		removed_waiters = removed_waiters->next;

		if(temp->callback)
			temp->callback(ARP_CALLBACK_CODE_OK, temp->callback_args, temp->i, link_addr);
		free(temp);
	}

	return 0;
}

static int arp_retransmit_thread(void *unused)
{
	arp_wait_request *wait;
	arp_wait_request *removed_waiters;
	arp_wait_request *last;
	arp_wait_request *temp;
	time_t now;
	bool empty_queue;

	for(;;) {
		event_wait(&arp_wait_event);

		for(;;) {
			now = current_time();
			removed_waiters = NULL;
			last = NULL;
			empty_queue = true;
			mutex_acquire(&arp_wait_mutex);
			wait = arp_waiters;
			while(wait) {
				empty_queue = false;
				if(now - wait->last_attempt_time > 1000) {
					if(++wait->attempt_count > 5) {
						// it's been tried too many times, put it on the free list
						if(last)
							last->next = wait->next;
						else
							arp_waiters = wait->next;
						temp = wait;
						wait = wait->next;

						// put this item on the just freed list
						temp->next = removed_waiters;
						removed_waiters = temp;
						continue;
					} else {
						// retransmit
						wait->last_attempt_time = now;
						arp_send_request(wait->i, wait->sender_ipaddr, wait->ip_addr);
					}
				}
				last = wait;
				wait = wait->next;
			}
			mutex_release(&arp_wait_mutex);

			// the queue is empty, lets break out of here and potentially go back to blocking forever
			if(empty_queue)
				break;

			// now we have a list of waiters that were woken up
			while(removed_waiters) {
				temp = removed_waiters;
				removed_waiters = removed_waiters->next;

				if(temp->callback)
					temp->callback(ARP_CALLBACK_CODE_FAILED, temp->callback_args, temp->i, 0);
				free(temp);
			}

			thread_sleep(500); // 1/2 sec
		}
	}
}

static int arp_cleanup_thread(void *unused)
{
	arp_cache_entry *e;
	arp_cache_entry *last;
	arp_cache_entry *temp;
	arp_cache_entry *free_list;
	time_t now;

	for(;;) {
		thread_sleep(1000 * 60); // 1 min

		free_list = NULL;
		now = current_time();

		mutex_acquire(&arp_table_mutex);

		free_list = NULL;
		last = NULL;
		e = arp_cache_entries;
		while(e) {
			if(now - e->last_used_time > 1000LL * 60 * 5) {
				// remove it from the list
				if(last)
					last->all_next = e->all_next;
				else
					arp_cache_entries = e->all_next;
				temp = e;
				e = e->all_next;

				// add it to the free list
				temp->all_next = free_list;
				free_list = temp;
				continue;
			}
			last = e;
			e = e->all_next;
		}
		mutex_release(&arp_table_mutex);

		// free any entries that we pulled out of the cache
		while(free_list) {
#if NET_CHATTY
			printf("arp_cleanup_thread: pruning arp entry for ");
			dump_ipv4_addr(e->ip_addr);
			printf("\n");
#endif
			temp = free_list;
			free_list = free_list->all_next;
			free(temp);
		}
	}
}

int arp_init(void)
{
	mutex_init(&arp_table_mutex);
	mutex_init(&arp_wait_mutex);
	event_init(&arp_wait_event, false, EVENT_FLAG_AUTOUNSIGNAL);

	arp_waiters = NULL;
	arp_cache_entries = NULL;

	arp_table = hash_init(256, offsetof(arp_cache_entry, next), &arp_cache_compare, &arp_cache_hash);
	if(!arp_table)
		return -1;

	thread_resume(thread_create("arp cache cleaner", &arp_cleanup_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
	thread_resume(thread_create("arp retransmit thread", &arp_retransmit_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));

	return 0;
}

