/*
** Copyright 2003, Travis Geiselbrecht. All rights reserved.
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
#include <kernel/heap.h>
#include <kernel/vm.h>
#include <kernel/fs/devfs.h>
#include <kernel/net/net.h>
#include <kernel/net/net_control.h>
#include <kernel/net/net_timer.h>
#include <kernel/net/if.h>
#include <kernel/net/ethernet.h>
#include <kernel/net/loopback.h>
#include <kernel/net/arp.h>
#include <kernel/net/ipv4.h>
#include <kernel/net/udp.h>
#include <kernel/net/tcp.h>
#include <kernel/net/socket.h>
#include <kernel/net/misc.h>
#include <string.h>

#define NET_CONTROL_TRACE 0

#if NET_CONTROL_TRACE > 0
#define TRACE(x...) dprintf("net_control: " x)
#else
#define TRACE(x...)
#endif

static int net_control_dev_open(dev_ident ident, dev_cookie *cookie)
{
	*cookie = NULL;

	return 0;
}

static int net_control_dev_close(dev_cookie cookie)
{
	return 0;
}

static int net_control_dev_freecookie(dev_cookie cookie)
{
	return 0;
}

static int net_control_dev_seek(dev_cookie cookie, off_t pos, seek_type st)
{
	return ERR_NOT_ALLOWED;
}

static int net_control_dev_ioctl(dev_cookie cookie, int op, void *buf, size_t len)
{
	int err;

	union {
		struct _ioctl_net_if_control_struct if_control;
		struct _ioctl_net_route_struct route_control;
	} u;

	TRACE("ioctl: op %d, buf %p, len %ld\n", op, buf, len);

	/* copy the args over */
	switch(op) {
		case IOCTL_NET_CONTROL_IF_CREATE:
		case IOCTL_NET_CONTROL_IF_DELETE:
		case IOCTL_NET_CONTROL_IF_ADDADDR:
		case IOCTL_NET_CONTROL_IF_RMADDR:
			// copy the interface args over
			err = user_memcpy(&u.if_control, buf, sizeof(u.if_control));
			if(err < 0)
				goto out;

			TRACE("ioctl: args name '%s', if_addr 0x%x, mask_addr 0x%x, b_addr 0x%x\n",
				u.if_control.if_name, NETADDR_TO_IPV4(u.if_control.if_addr),
				NETADDR_TO_IPV4(u.if_control.mask_addr), NETADDR_TO_IPV4(u.if_control.broadcast_addr));
			break;
		case IOCTL_NET_CONTROL_ROUTE_ADD:
		case IOCTL_NET_CONTROL_ROUTE_DELETE:
			// copy the route args over
			err = user_memcpy(&u.route_control, buf, sizeof(u.route_control));
			if(err < 0)
				goto out;

			TRACE("ioctl: args name '%s', if_addr 0x%x, mask_addr 0x%x, net_addr 0x%x\n",
				u.route_control.if_name, NETADDR_TO_IPV4(u.route_control.if_addr),
				NETADDR_TO_IPV4(u.route_control.mask_addr), NETADDR_TO_IPV4(u.route_control.net_addr));
			break;
	}

	/* do the operation */
	switch(op) {
		case IOCTL_NET_CONTROL_IF_CREATE: {
			ifnet *i;

			err = if_register_interface(u.if_control.if_name, &i);
			if(err < 0)
				goto out;

			err = NO_ERROR;
			break;
		}
		case IOCTL_NET_CONTROL_IF_DELETE:
			err = ERR_UNIMPLEMENTED;
			break;
		case IOCTL_NET_CONTROL_IF_ADDADDR: {
			ifnet *i;
			ifaddr *address;

			/* look up the structure describing the interface */
			i = if_path_to_ifnet(u.if_control.if_name);
			if(i == NULL) {
				err = ERR_NOT_FOUND;
				goto out;
			}

			/* set up the address on the interface */
			address = kmalloc(sizeof(ifaddr));
			if(address == NULL) {
				err = ERR_NO_MEMORY;
				goto out;
			}
			memcpy(&address->addr, &u.if_control.if_addr, sizeof(address->addr));
			memcpy(&address->netmask, &u.if_control.mask_addr, sizeof(address->netmask));
			memcpy(&address->broadcast, &u.if_control.broadcast_addr, sizeof(address->broadcast));
 			if_bind_address(i, address);

			err = NO_ERROR;
			break;
		}
		case IOCTL_NET_CONTROL_IF_RMADDR:
		case IOCTL_NET_CONTROL_IF_LIST:
			err = ERR_UNIMPLEMENTED;
			break;
		case IOCTL_NET_CONTROL_ROUTE_ADD: {

			/* see what type of network we are dealing with here */
			if(u.route_control.net_addr.type == ADDR_TYPE_IP) {
				ifnet *i;

				i = if_path_to_ifnet(u.route_control.if_name);
				if(i == NULL) {
					err = ERR_NET_BAD_ADDRESS;
					goto out;
				}

				if(NETADDR_TO_IPV4(u.route_control.mask_addr) == 0) {
					/* this is a default gateway route */
					ipv4_route_add_gateway(0, 0,
						NETADDR_TO_IPV4(u.route_control.if_addr), i->id,
						NETADDR_TO_IPV4(u.route_control.net_addr));
				} else {
					/* regular ol' route */
					ipv4_route_add(NETADDR_TO_IPV4(u.route_control.net_addr),
						NETADDR_TO_IPV4(u.route_control.mask_addr),
						NETADDR_TO_IPV4(u.route_control.if_addr), i->id);
				}
			} else {
				err = ERR_UNIMPLEMENTED;
				break;
			}

			err = NO_ERROR;
			break;
		}
		case IOCTL_NET_CONTROL_ROUTE_DELETE:
		case IOCTL_NET_CONTROL_ROUTE_LIST:
			err = ERR_UNIMPLEMENTED;
			break;
 		default:
			err = ERR_INVALID_ARGS;
	}

out:
	return err;
}

static ssize_t net_control_dev_read(dev_cookie cookie, void *buf, off_t pos, ssize_t len)
{
	return ERR_NOT_ALLOWED;
}

static ssize_t net_control_dev_write(dev_cookie cookie, const void *buf, off_t pos, ssize_t len)
{
	return ERR_NOT_ALLOWED;
}

static struct dev_calls net_control_dev_hooks = {
	&net_control_dev_open,
	&net_control_dev_close,
	&net_control_dev_freecookie,
	&net_control_dev_seek,
	&net_control_dev_ioctl,
	&net_control_dev_read,
	&net_control_dev_write,
	NULL,
	NULL,
	NULL
};

int net_control_init(void)
{
	// create device node
	devfs_publish_device("net/ctrl", NULL, &net_control_dev_hooks);

	return 0;
}

