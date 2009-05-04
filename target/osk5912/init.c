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
#include <err.h>
#include <debug.h>
#include <target.h>
#include <stdlib.h>
#include <compiler.h>
#include <dev/net/smc91c96.h>
#include <lib/net.h>
#include <lib/net/ipv4.h>
#include <lib/net/if.h>

void target_init(void)
{
	// XXX find a better place to init this
	net_init();

	smc91c96_init();
	smc91c96_start();

	// more net stuff that we need to put somewhere else
	// set the ip address for this net interface
	ifaddr *address = malloc(sizeof(ifaddr));
	address->addr.len = 4;
	address->addr.type = ADDR_TYPE_IP;
	NETADDR_TO_IPV4(address->addr) = IPV4_DOTADDR_TO_ADDR(192,168,0,99);
	address->netmask.len = 4;
	address->netmask.type = ADDR_TYPE_IP;
	NETADDR_TO_IPV4(address->netmask) = 0xffffff00; // 255.255.255.0
	address->broadcast.len = 4;
	address->broadcast.type = ADDR_TYPE_IP;
	NETADDR_TO_IPV4(address->broadcast) = IPV4_DOTADDR_TO_ADDR(192,168,0,255);
	ifnet *i = if_id_to_ifnet(1); // XXX total hack
	ASSERT(i);
	if_bind_address(i, address);

	// set up an initial routing table
	ipv4_route_add(IPV4_DOTADDR_TO_ADDR(192,168,0,0), 
		0xffffff00, 
		NETADDR_TO_IPV4(address->addr), i->id);

}

