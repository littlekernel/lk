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
#include <kernel/kernel.h>
#include <kernel/debug.h>
#include <kernel/cbuf.h>
#include <kernel/net/misc.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/ipv4.h>
#include <kernel/net/arp.h>
#include <string.h>

#define MIN_ETHERNET2_LEN 46

typedef struct ethernet2_header {
	ethernet_addr dest;
	ethernet_addr src;
	uint16 type;
} ethernet2_header;

void dump_ethernet_addr(ethernet_addr addr)
{
#if NET_CHATTY
	dprintf("%x:%x:%x:%x:%x:%x",
		addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
#endif
}

static void dump_ethernet_header(ethernet2_header *head)
{
#if NET_CHATTY
	dprintf("ethernet 2 header: dest ");
	dump_ethernet_addr(head->dest);
	dprintf(" src ");
	dump_ethernet_addr(head->src);
	dprintf(" type 0x%x\n", ntohs(head->type));
#endif
}

int ethernet_input(cbuf *buf, ifnet *i)
{
	int err;
	ethernet2_header *e2_head;
	uint16 type;

	if(cbuf_get_len(buf) < MIN_ETHERNET2_LEN)
		return -1;

	e2_head = cbuf_get_ptr(buf, 0);
	type = ntohs(e2_head->type);

	dump_ethernet_header(e2_head);

	// strip out the ethernet header
	buf = cbuf_truncate_head(buf, sizeof(ethernet2_header), true);

	switch(type) {
		case PROT_TYPE_IPV4:
			err = ipv4_input(buf, i);
			break;
		case PROT_TYPE_ARP:
			err = arp_input(buf, i);
			break;
		default:
			dprintf("ethernet_receive: unknown ethernet type 0x%x\n", type);
			err = -1;
	}

	return err;
}

int ethernet_output(cbuf *buf, ifnet *i, netaddr *target, int protocol_type)
{
	cbuf *eheader_buf;
	ethernet2_header *eheader;

	if(target->type != ADDR_TYPE_ETHERNET) {
		cbuf_free_chain(buf);
		return ERR_INVALID_ARGS;
	}

	eheader_buf = cbuf_get_chain(sizeof(ethernet2_header));
	if(!eheader_buf) {
		dprintf("ethernet_output: error allocating cbuf for eheader\n");
		cbuf_free_chain(buf);
		return ERR_NO_MEMORY;
	}

	// put together an ethernet header
	eheader = (ethernet2_header *)cbuf_get_ptr(eheader_buf, 0);
	memcpy(&eheader->dest, &target->addr[0], 6);
	memcpy(&eheader->src, &i->link_addr->addr.addr[0], 6);
	eheader->type = htons(protocol_type);

	// chain the buffers together
	buf = cbuf_merge_chains(eheader_buf, buf);

	return if_output(buf, i);
}

int ethernet_init(void)
{
	return 0;
}

