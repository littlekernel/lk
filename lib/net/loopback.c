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
#include <lib/net/ipv4.h>
#include <lib/net/arp.h>

static int _loopback_input(cbuf *buf, ifnet *i, int protocol_type)
{
	int err;

	switch(protocol_type) {
		case PROT_TYPE_IPV4:
			err = ipv4_input(buf, i);
			break;
		case PROT_TYPE_ARP:
			err = arp_input(buf, i);
			break;
		default:
			err = -1;
	}

	return err;
}

int loopback_input(cbuf *buf, ifnet *i)
{
	// What? you can't call this directly
	cbuf_free_chain(buf);
	return NO_ERROR;
}

int loopback_output(cbuf *buf, ifnet *i, netaddr *target, int protocol_type)
{
	_loopback_input(buf, i, protocol_type);

	return NO_ERROR;
}

int loopback_init(void)
{
	ifnet *i;
	ifaddr *address;
	int err;

	return 0;

	// set up an initial device
	err = if_register_interface("loopback", &i);
	if(err < 0)
		return err;

	address = malloc(sizeof(ifaddr));
	address->addr.type = ADDR_TYPE_NULL;
	address->broadcast.type = ADDR_TYPE_NULL;
	address->netmask.type = ADDR_TYPE_NULL;
	if_bind_link_address(i, address);

	// set the ip address for this net interface
	address = malloc(sizeof(ifaddr));
	address->addr.len = 4;
	address->addr.type = ADDR_TYPE_IP;
	NETADDR_TO_IPV4(address->addr) = 0x7f000001; // 127.0.0.1
	address->netmask.len = 4;
	address->netmask.type = ADDR_TYPE_IP;
	NETADDR_TO_IPV4(address->netmask) = 0xff000000; // 255.255.255.0
	address->broadcast.len = 4;
	address->broadcast.type = ADDR_TYPE_IP;
	NETADDR_TO_IPV4(address->broadcast) = 0x7fffffff; // 127.255.255.255
	if_bind_address(i, address);

	// set up an initial routing table
	ipv4_route_add(0x7f000000, 0xff000000, 0x7f000001, i->id);

	return 0;
}

