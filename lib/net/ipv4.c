/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
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
#include <lib/net/misc.h>
#include <lib/net/ethernet.h>
#include <lib/net/ipv4.h>
#include <lib/net/icmp.h>
#include <lib/net/udp.h>
#include <lib/net/tcp.h>
#include <lib/net/arp.h>
#include <lib/net/net_timer.h>
#include <lib/net/hash.h>

typedef struct ipv4_header {
	uint8_t version_length;
	uint8_t tos;
	uint16_t total_length;
	uint16_t identification;
	uint16_t flags_frag_offset;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t header_checksum;
	ipv4_addr src;
	ipv4_addr dest;
} __PACKED ipv4_header;

#define IPV4_FLAG_MORE_FRAGS   0x2000
#define IPV4_FLAG_MAY_NOT_FRAG 0x4000
#define IPV4_FRAG_OFFSET_MASK  0x1fff

typedef struct ipv4_routing_entry {
	struct ipv4_routing_entry *next;
	ipv4_addr network_addr;
	ipv4_addr netmask;
	ipv4_addr gw_addr;
	ipv4_addr if_addr;
	if_id interface_id;
	int flags;
} ipv4_routing_entry;

#define ROUTE_FLAGS_GW 1

// routing table
static ipv4_routing_entry *route_table;
static mutex_t route_table_mutex;

typedef struct ipv4_fragment {
	struct ipv4_fragment *hash_next;
	struct ipv4_fragment *frag_next;
	cbuf *buf;
	uint16_t offset;
	uint16_t len;
	time_t entry_time;
	uint16_t total_len;
	// copied from the header, enough data to uniquely identify the frag
	ipv4_addr src;
	ipv4_addr dest;
	uint16_t identification;
	uint8_t protocol;
} ipv4_fragment;

typedef struct ipv4_fragment_key {
	ipv4_addr src;
	ipv4_addr dest;
	uint16_t identification;
	uint8_t protocol;
} ipv4_fragment_key;

// current ip identification number
static uint32_t curr_identification;

// fragment hash table
static void *frag_table;
static mutex_t frag_table_mutex;
static net_timer_event frag_killer_event;

#define FRAG_KILLER_QUANTUM 5000 /* 5 secs */
#define MAX_FRAG_AGE 60000 /* 1 min */

static int frag_compare_func(void *_frag, const void *_key)
{
	ipv4_fragment *frag = _frag;
	const ipv4_fragment_key *key = _key;

	if(frag->src == key->src && frag->dest == key->dest &&
	   frag->identification == key->identification && frag->protocol == key->protocol) {
		return 0;
	} else {
		return -1;
	}
}

// XXX lameo hash
static unsigned int frag_hash_func(void *_frag, const void *_key, unsigned int range)
{
	ipv4_fragment *frag = _frag;
	const ipv4_fragment_key *key = _key;

	if(frag)
		return (frag->src ^ frag->dest ^ frag->protocol ^ frag->identification) % range;
	else
		return (key->src ^ key->dest ^ key->protocol ^ key->identification) % range;
}

// expects hosts order
void dump_ipv4_addr(ipv4_addr addr)
{
	uint8_t *nuaddr = (uint8_t *)&addr;

	printf("%d.%d.%d.%d", nuaddr[3], nuaddr[2], nuaddr[1], nuaddr[0]);
}

static void dump_ipv4_header(ipv4_header *head)
{
#if NET_CHATTY
	printf("ipv4 header: src ");
	dump_ipv4_addr(ntohl(head->src));
	printf(" dest ");
	dump_ipv4_addr(ntohl(head->dest));
	printf(" prot %d, cksum 0x%x, len 0x%x, ident 0x%x, frag offset 0x%x\n",
		head->protocol, ntohs(head->header_checksum), ntohs(head->total_length), ntohs(head->identification), ntohs(head->flags_frag_offset) & 0x1fff);
#endif
}

static void ipv4_frag_killer(void *unused)
{
	struct hash_iterator i;
	ipv4_fragment *frag, *last;
	ipv4_fragment *free_list = NULL;
	time_t now = current_time();

	set_net_timer(&frag_killer_event, FRAG_KILLER_QUANTUM, &ipv4_frag_killer, NULL, 0);

	mutex_acquire(&frag_table_mutex);

	// cycle through the list, searching for a chain that's older than the max age
	hash_open(frag_table, &i);
	frag = hash_next(frag_table, &i);
	while(frag != NULL) {
		last = frag;
		frag = hash_next(frag_table, &i);

		// see if last is eligable for death
		if(now - last->entry_time > MAX_FRAG_AGE) {
			hash_remove(frag_table, last);
			last->hash_next = free_list;
			free_list = last;
		}
	}

	mutex_release(&frag_table_mutex);

	// erase the frags we scheduled to be killed
	while(free_list) {
		frag = free_list;
		free_list = frag->hash_next;

		// walk this frag chain
		while(frag) {
			last = frag;
			frag = frag->frag_next;

			// kill last
			cbuf_free_chain(last->buf);
			free(last);
		}
	}
}

static int ipv4_route_add_etc(ipv4_addr network_addr, ipv4_addr netmask, ipv4_addr if_addr, if_id interface_num, int flags, ipv4_addr gw_addr)
{
	ipv4_routing_entry *e;
	ipv4_routing_entry *temp;
	ipv4_routing_entry *last;

	// make sure the netmask makes sense
	if((netmask | (netmask - 1)) != 0xffffffff) {
		return ERR_INVALID_ARGS;
	}

	e = malloc(sizeof(ipv4_routing_entry));
	if(!e)
		return ERR_NO_MEMORY;

	e->network_addr = network_addr;
	e->netmask = netmask;
	e->gw_addr = gw_addr;
	e->if_addr = if_addr;
	e->interface_id = interface_num;
	e->flags = flags;

	mutex_acquire(&route_table_mutex);

	// add it to the list, sorted by netmask 'completeness'
	last = NULL;
	for(temp = route_table; temp; temp = temp->next) {
		if((netmask | e->netmask) == e->netmask) {
			// insert our route entry here
			break;
		}
		last = temp;
	}
	if(last)
		last->next = e;
	else
		route_table = e;
	e->next = temp;

	mutex_release(&route_table_mutex);

	return NO_ERROR;
}

int ipv4_route_add(ipv4_addr network_addr, ipv4_addr netmask, ipv4_addr if_addr, if_id interface_num)
{
	return ipv4_route_add_etc(network_addr, netmask, if_addr, interface_num, 0, 0);
}

int ipv4_route_add_gateway(ipv4_addr network_addr, ipv4_addr netmask, ipv4_addr if_addr, if_id interface_num, ipv4_addr gw_addr)
{
	return ipv4_route_add_etc(network_addr, netmask, if_addr, interface_num, ROUTE_FLAGS_GW, gw_addr);
}

static int ipv4_route_match(ipv4_addr ip_addr, if_id *interface_num, ipv4_addr *target_addr, ipv4_addr *if_addr)
{
	ipv4_routing_entry *e;
	ipv4_routing_entry *last_e = NULL;
	int err;

	// walk through the routing table, finding the last entry to match
	mutex_acquire(&route_table_mutex);
	for(e = route_table; e; e = e->next) {
		ipv4_addr masked_addr = ip_addr & e->netmask;
		if(masked_addr == e->network_addr)
			last_e = e;
	}

	if(last_e) {
		*interface_num = last_e->interface_id;
		*if_addr = last_e->if_addr;
		if(last_e->flags & ROUTE_FLAGS_GW) {
			*target_addr = last_e->gw_addr;
		} else {
			*target_addr = ip_addr;
		}
		err = NO_ERROR;
	} else {
		*interface_num = -1;
		*target_addr = 0;
		*if_addr = 0;
		err = ERR_NET_NO_ROUTE;
	}
	mutex_release(&route_table_mutex);

	return err;
}

int ipv4_lookup_srcaddr_for_dest(ipv4_addr dest_addr, ipv4_addr *src_addr)
{
	if_id id;
	ipv4_addr target_addr;

	return ipv4_route_match(dest_addr, &id, &target_addr, src_addr);
}

int ipv4_get_mss_for_dest(ipv4_addr dest_addr, uint32_t *mss)
{
	if_id id;
	ifnet *i;
	ipv4_addr target_addr;
	ipv4_addr src_addr;
	int err;

	err = ipv4_route_match(dest_addr, &id, &target_addr, &src_addr);
	if(err < 0)
		return err;

	i = if_id_to_ifnet(id);
	if(i == NULL)
		return ERR_NET_NO_ROUTE;

	*mss = i->hook->mtu - sizeof(ipv4_header);

	return NO_ERROR;
}

static void ipv4_arp_callback(int arp_code, void *args, ifnet *i, netaddr *link_addr)
{
	cbuf *buf = args;

	if(arp_code == ARP_CALLBACK_CODE_OK) {
		// arp found us an address and called us back with it
		i->link_output(buf, i, link_addr, PROT_TYPE_IPV4);
	} else if(arp_code == ARP_CALLBACK_CODE_FAILED) {
		// arp retransmitted and failed, so we're screwed
		cbuf_free_chain(buf);
	} else {
		// uh
		;
	}
}

int ipv4_output(cbuf *buf, ipv4_addr target_addr, int protocol)
{
	cbuf *header_buf;
	ipv4_header *header;
	netaddr link_addr;
	if_id iid;
	ifnet *i;
	ipv4_addr transmit_addr;
	ipv4_addr if_addr;
	int err;
	uint16_t len;
	uint16_t curr_offset;
	uint16_t identification;
	bool must_frag = false;

#if NET_CHATTY
	printf("ipv4_output: buf %p, target_addr ", buf);
	dump_ipv4_addr(target_addr);
	printf(", protocol %d, len %d\n", protocol, cbuf_get_len(buf));
#endif

	// figure out what interface we will send this over
	err = ipv4_route_match(target_addr, &iid, &transmit_addr, &if_addr);
	if(err < 0) {
		cbuf_free_chain(buf);
		return ERR_NO_MEMORY;
	}
	i = if_id_to_ifnet(iid);
	if(!i) {
		cbuf_free_chain(buf);
		return ERR_NO_MEMORY;
	}

	// figure out the total len
	len = cbuf_get_len(buf);
	if(len + sizeof(ipv4_header) > i->hook->mtu)
		must_frag = true;

//	LTRACEF("did route match, result iid %d, i 0x%x, transmit_addr 0x%x, if_addr 0x%x\n", iid, i, transmit_addr, if_addr);

	identification = atomic_add(&curr_identification, 1);
	identification = htons(identification);

	curr_offset = 0;
	while(len > 0) {
		uint16_t packet_len;
		uint16_t header_len;
		cbuf *send_buf;

		header_len = sizeof(ipv4_header);
		header_buf = cbuf_get_chain(header_len);
		if(!header_buf) {
			cbuf_free_chain(buf);
			return ERR_NO_MEMORY;
		}
		header = cbuf_get_ptr(header_buf, 0);

		packet_len = MIN(i->hook->mtu, (unsigned)(len + header_len));
		if(packet_len == i->hook->mtu)
			packet_len = ROUNDDOWN(packet_len - header_len, 8) + header_len;

		header->version_length = 0x4 << 4 | 5;
		header->tos = 0;
		header->total_length = htons(packet_len);
		header->identification = identification;
		header->flags_frag_offset = htons((curr_offset / 8) & IPV4_FRAG_OFFSET_MASK);
		if(packet_len != len + header_len)
			header->flags_frag_offset |= htons(IPV4_FLAG_MORE_FRAGS);
		header->ttl = 255;
		header->protocol = protocol;
		header->header_checksum = 0;
		header->src = htonl(if_addr);
		header->dest = htonl(target_addr);

		// calculate the checksum
		header->header_checksum = cksum16(header, (header->version_length & 0xf) * 4);

		if(must_frag) {
			send_buf = cbuf_duplicate_chain(buf, curr_offset, packet_len - header_len, 0);
			if(!send_buf) {
				cbuf_free_chain(header_buf);
				cbuf_free_chain(buf);
				return ERR_NO_MEMORY;
			}
		} else {
			send_buf = buf;
		}
		send_buf = cbuf_merge_chains(header_buf, send_buf);

		// do the arp thang
		err = arp_lookup(i, if_addr, transmit_addr, &link_addr, &ipv4_arp_callback, send_buf);
		if(err == ERR_NET_ARP_QUEUED) {
			// the arp request is queued up so we can just exit here
			// and the rest of the work will be done via the arp callback
		} else if(err < 0) {
	#if NET_CHATTY
			TRACEF("failed arp lookup\n");
	#endif
			cbuf_free_chain(send_buf);
		} else {
			// we got the link layer address, send the packet
			i->link_output(send_buf, i, &link_addr, PROT_TYPE_IPV4);
		}

		// update the offset
		curr_offset += packet_len - header_len;
		len -= packet_len - header_len;
	}

	if(must_frag)
		cbuf_free_chain(buf);

	return err;
}

static ipv4_fragment *ipv4_create_frag_struct(ipv4_fragment_key *key, cbuf *buf, uint16_t offset, uint16_t len, bool last_frag)
{
	ipv4_fragment *frag;

	// create a new frag
	frag = malloc(sizeof(ipv4_fragment));
	if(!frag)
		return NULL;

	frag->hash_next = NULL;
	frag->frag_next = NULL;
	frag->buf = buf;
	frag->offset = offset;
	frag->len = len;
	frag->entry_time = current_time();
	frag->src = key->src;
	frag->dest = key->dest;
	frag->identification = key->identification;
	frag->protocol = key->protocol;

	// if this was the last frag, we now know the total len
	if(last_frag)
		frag->total_len = frag->offset + frag->len;
	else
		frag->total_len = 0;

	return frag;
}

static int ipv4_process_frag(cbuf *inbuf, ifnet *i, cbuf **outbuf)
{
	int err;
	ipv4_header *header;
	ipv4_fragment_key key;
	ipv4_fragment *frag;
	ipv4_fragment *temp;
	ipv4_fragment *last;
	uint16_t offset;
	uint16_t len;
	bool last_frag;

	*outbuf = NULL;

#if NET_CHATTY
	TRACEF("inbuf %p, i %p, outbuf %p\n", inbuf, i, outbuf);
#endif
	header = (ipv4_header *)cbuf_get_ptr(inbuf, 0);
	offset = (ntohs(header->flags_frag_offset) & IPV4_FRAG_OFFSET_MASK) * 8;
	len = ntohs(header->total_length) - ((header->version_length & 0xf) * 4);
	last_frag = (ntohs(header->flags_frag_offset) & IPV4_FLAG_MORE_FRAGS) ? false : true;

	// look in the hash table to see if there are any frags this will help complete
	key.src = ntohl(header->src);
	key.dest = ntohl(header->dest);
	key.identification = ntohs(header->identification);
	key.protocol = header->protocol;

#if NET_CHATTY
	TRACEF("src 0x%x dest 0x%x ident %d prot %d offset %d len %d last_frag %d\n",
		key.src, key.dest, key.identification, key.protocol, offset, len, last_frag);
#endif

	mutex_acquire(&frag_table_mutex);

	frag = hash_lookup(frag_table, &key);
	if(frag) {
		// this is part of an older frag
		bool bad_frag = false;
		bool found_spot = false;

		// find the spot where this frag would be in the frag list
		for(last = NULL, temp = frag; temp; last = temp, temp = temp->frag_next) {

#if NET_CHATTY
			TRACEF("last %p, temp %p\n", last, temp);
			TRACEF("bad_frag %d, found_spot %d\n", bad_frag, found_spot);
#endif

			// if we haven't already found a spot, look for it, and make sure
			// the new frag would insert properly (no cross-overs, etc)
			if(!found_spot) {
				// first, make sure it would insert into the list cleanly
				// see if this frag is the same as one we've received already
				if(last) {
					if(offset < last->offset + last->len) {
						bad_frag = true;
						goto done_frag_spot_search;
					}
				}
				if(temp->offset > offset && (offset + len > temp->offset)) {
					bad_frag = true;
					goto done_frag_spot_search;
				}

				// now, see if we can stop here
				if(last && offset > last->offset && offset < temp->offset) {
					found_spot = true;
				} else if(offset < temp->offset) {
					found_spot = true;
				}
			}
		}
		// if we still hadn't found a spot, do a last check to see if it'll tack on
		// to the end of the frag list properly
#if NET_CHATTY
		TRACEF("out of loop: last %p, temp %p, found_spot %d, bad_frag %d\n", last, temp, found_spot, bad_frag);
#endif
		if(!found_spot) {
			if(offset < last->offset + last->len) {
				// crosses last in list
				bad_frag = true;
				goto done_frag_spot_search;
			}
			// see if it's valid to tack on to the end of the list
			if(frag->total_len > 0) {
				// this frag chain had already received the end of frag packet
				if(offset + len > frag->total_len) {
					bad_frag = true;
					goto done_frag_spot_search;
				}
			} else {
				if(last_frag) {
					// we now know the full size of this fragment chain
					frag->total_len = offset + len;
				}
			}
		}

done_frag_spot_search:
		if(bad_frag) {
			TRACEF("received fragment is bad\n");
			cbuf_free_chain(inbuf);
			err = ERR_NET_BAD_PACKET;
			goto out;
		}

		// now we know we have the spot
		// see if we can merge it with one of the others

		// will it merge with the last one?
		if(last) {
			if(last->offset + last->len == offset) {
				// merge it
				inbuf = cbuf_truncate_head(inbuf, ((header->version_length & 0xf) * 4), 0);
				last->buf = cbuf_merge_chains(last->buf, inbuf);
				inbuf = last->buf;
				last->len += len;
				header = (ipv4_header *)cbuf_get_ptr(inbuf, 0);
				header->total_length = htons(ntohs(header->total_length) + len);

				// the free floating frag has been 'eaten'
				len = 0;
			}
		}
		// will it merge with the next one?
		if(len > 0 && temp) {
			if(offset + len == temp->offset) {
				// merge it
				ipv4_header *next_header = cbuf_get_ptr(temp->buf, 0);
				temp->buf = cbuf_truncate_head(temp->buf, ((next_header->version_length & 0xf) * 4), 0);
				temp->buf = cbuf_merge_chains(inbuf, temp->buf);
				inbuf = temp->buf;
				header = (ipv4_header *)cbuf_get_ptr(inbuf, 0);
				header->total_length = htons(ntohs(header->total_length) + temp->len);
				temp->len += len;
				temp->offset = offset;

				// the free floating frag has been 'eaten'
				len = 0;
			}
		}

		// see if last and next frag will merge
		if(last && temp) {
			if(last->offset + len == temp->offset) {
				// merge them
				ipv4_header *next_header = cbuf_get_ptr(temp->buf, 0);
				temp->buf = cbuf_truncate_head(temp->buf, ((next_header->version_length & 0xf) * 4), true);
				last->buf = cbuf_merge_chains(last->buf, temp->buf);
				inbuf = last->buf;
				header = (ipv4_header *)cbuf_get_ptr(inbuf, 0);
				header->total_length = htons(ntohs(header->total_length) + temp->len);
				last->len += temp->len;

				// delete the next frag structure
				last->frag_next = temp->frag_next;
				free(temp);
			}
		}

		// if we still have a free floating frag, create a new frag.
		// otherwise, see if we've completed a frag
		if(len > 0) {
			// create a new frag and put it in the list
			ipv4_fragment *newfrag;

			newfrag = ipv4_create_frag_struct(&key, inbuf, offset, len, last_frag);
			if(!frag) {
				cbuf_free_chain(inbuf);
				err = ERR_NO_MEMORY;
				goto out;
			}

			// are we at the beginning of the frag chain
			if(!last) {
				// we are, so this will be the new head of the list
				hash_remove(frag_table, frag);
				newfrag->frag_next = frag;
				newfrag->total_len = frag->total_len;
				hash_insert(frag_table, newfrag);
			} else {
				newfrag->frag_next = temp;
				last->frag_next = newfrag;
			}
		} else {
			// we ate the frag in some way, so see if we completed the list
			if(frag->offset == 0 && frag->total_len == frag->len) {
				// we have completed the frag
				hash_remove(frag_table, frag);
				*outbuf = frag->buf;
				if(frag->frag_next)
					panic("ipv4_process_frag: found completed frag but still has a chain! frag %p\n", frag);
				free(frag);
			}
		}
	} else {
		// create a new frag
		frag = ipv4_create_frag_struct(&key, inbuf, offset, len, last_frag);
		if(!frag) {
			cbuf_free_chain(inbuf);
			err = ERR_NO_MEMORY;
			goto out;
		}

		// add it to the list
		hash_insert(frag_table, frag);
	}

	err= NO_ERROR;

out:
	mutex_release(&frag_table_mutex);

	return err;

}

int ipv4_input(cbuf *buf, ifnet *i)
{
	int err;
	ipv4_header *header;
	ipv4_addr src, dest;
	uint8_t protocol;

	header = (ipv4_header *)cbuf_get_ptr(buf, 0);

	if(cbuf_get_len(buf) < 4) {
		err = ERR_NET_BAD_PACKET;
		goto ditch_packet;
	}

	dump_ipv4_header(header);

	if(((header->version_length >> 4) & 0xf) != 4) {
		TRACEF("ipv4 packet has bad version\n");
		err = ERR_NET_BAD_PACKET;
		goto ditch_packet;
	}

	if(cbuf_get_len(buf) < sizeof(ipv4_header)) {
		err = ERR_NET_BAD_PACKET;
		goto ditch_packet;
	}

	if(cksum16(header, (header->version_length & 0xf) * 4) != 0) {
		TRACEF("ipv4 packet failed cksum\n");
		err = ERR_NET_BAD_PACKET;
		goto ditch_packet;
	}

	// verify that this packet is for us
	if(ntohl(header->dest) != 0xffffffff) {
		ifaddr *iaddr;
		ipv4_addr dest = ntohl(header->dest);

		for(iaddr = i->addr_list; iaddr; iaddr = iaddr->next) {
			if(iaddr->addr.type == ADDR_TYPE_IP) {
				// see if it matches one of this interface's ip addresses
				if(dest == NETADDR_TO_IPV4(iaddr->addr))
					break;
				// see if it matches the broadcast address
				if(dest == NETADDR_TO_IPV4(iaddr->broadcast))
					break;
			}
		}
		if(!iaddr) {
			TRACEF("ipv4 packet for someone else\n");
			err = NO_ERROR;
			goto ditch_packet;
		}
	}

	// do some sanity checks and buffer trimming
	{
		size_t buf_len = cbuf_get_len(buf);
		uint16_t packet_len = ntohs(header->total_length);

		// see if the packet is too short
		if(buf_len < packet_len) {
			TRACEF("ipv4 packet too short (buf_len %ld, packet len %d)\n", buf_len, packet_len);
			err = ERR_NET_BAD_PACKET;
			goto ditch_packet;
		}

		// see if we need to trim off any padding
		if(buf_len > packet_len) {
			cbuf_truncate_tail(buf, buf_len - packet_len, true);
		}
	}

	// see if it's a fragment
	if(ntohs(header->flags_frag_offset) & IPV4_FLAG_MORE_FRAGS ||
	  (ntohs(header->flags_frag_offset) & IPV4_FRAG_OFFSET_MASK) != 0) {
		cbuf *new_buf;

		err = ipv4_process_frag(buf, i, &new_buf);
		if(err < 0)
			goto ditch_packet;
		if(new_buf) {
			// it processed the frag, and built us a complete packet
			buf = new_buf;
			header = cbuf_get_ptr(buf, 0);
		} else {
			// it ate the frag, so we're done here
			err = NO_ERROR;
			goto out;
		}
	}

	// save some data
	protocol = header->protocol;
	src = ntohl(header->src);
	dest = ntohl(header->dest);

	// strip off the ip header
	buf = cbuf_truncate_head(buf, (header->version_length & 0xf) * 4, true);

	// demultiplex and hand to the proper module
	switch(protocol) {
		case IP_PROT_ICMP:
			return icmp_input(buf, i, src);
		case IP_PROT_TCP:
			return tcp_input(buf, i, src, dest);
		case IP_PROT_UDP:
			return udp_input(buf, i, src, dest);
		default:
			TRACEF("packet with unknown protocol (%d)\n", protocol);
			err = ERR_NET_BAD_PACKET;
			goto ditch_packet;
	}

	err = NO_ERROR;

ditch_packet:
	cbuf_free_chain(buf);
out:
	return err;
}

int ipv4_init(void)
{
	mutex_init(&route_table_mutex);
	mutex_init(&frag_table_mutex);

	route_table = NULL;
	curr_identification = current_time();

	frag_table = hash_init(256, offsetof(ipv4_fragment, hash_next),
		&frag_compare_func, &frag_hash_func);

	set_net_timer(&frag_killer_event, FRAG_KILLER_QUANTUM, &ipv4_frag_killer, NULL, 0);

	return 0;
}

