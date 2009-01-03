/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
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
#include <kernel/khash.h>
#include <kernel/lock.h>
#include <kernel/heap.h>
#include <kernel/vm.h>
#include <kernel/fs/devfs.h>
#include <kernel/net/socket.h>
#include <newos/socket_api.h>
#include <string.h>

typedef struct socket_dev {
	sock_id id;
} socket_dev;

static int socket_dev_open(dev_ident ident, dev_cookie *cookie)
{
	socket_dev *s;

	s = (socket_dev *)kmalloc(sizeof(socket_dev));
	if(!s)
		return ERR_NO_MEMORY;

	s->id = -1;

	*cookie = s;

	return 0;
}

static int socket_dev_close(dev_cookie cookie)
{
	socket_dev *s = (socket_dev *)cookie;

	if(s->id >= 0)
		return socket_close(s->id);
	else
		return 0;
}

static int socket_dev_freecookie(dev_cookie cookie)
{
	kfree(cookie);
	return 0;
}

static int socket_dev_seek(dev_cookie cookie, off_t pos, seek_type st)
{
	return ERR_NOT_ALLOWED;
}

static int socket_dev_ioctl(dev_cookie cookie, int op, void *buf, size_t len)
{
	socket_dev *s = (socket_dev *)cookie;
	_socket_api_args_t args;
	int err;

	// copy the args over from user space
	err = user_memcpy(&args, buf, min(sizeof(args), len));
	if(err < 0)
		return err;

	if(s->id < 0) {
		switch(op) {
			case _SOCKET_API_CREATE:
				err = s->id = socket_create(args.u.create.type, args.u.create.flags);
				break;
			case _SOCKET_API_ASSOCIATE_SOCKET:
				s->id = args.u.associate.id;
				err = NO_ERROR;
				break;
			default:
				err = ERR_INVALID_ARGS;
		}
	} else {
		switch(op) {
			case _SOCKET_API_BIND:
				err = socket_bind(s->id, args.u.bind.saddr);
				break;
			case _SOCKET_API_CONNECT:
				err = socket_connect(s->id, args.u.connect.saddr);
				break;
			case _SOCKET_API_LISTEN:
				err = socket_listen(s->id);
				break;
			case _SOCKET_API_ACCEPT: {
				// this one is a little tricky, we have a new socket we need to associate with a file descriptor
				sock_id id;
				int fd;
				_socket_api_args_t new_args;
				char socket_dev_path[SYS_MAX_PATH_LEN];

				id = socket_accept(s->id, args.u.accept.saddr);
				if(id < 0) {
					err = id;
					break;
				}

				// we have the new id, open a new file descriptor in user space
				strcpy(socket_dev_path, "/dev/net/socket");
				fd = vfs_open(socket_dev_path, 0, false);
				if(fd < 0) {
					socket_close(id);
					err = fd;
					break;
				}

				// now do a special call on this file descriptor that associates it with this socket
				new_args.u.associate.id = id;
				err = vfs_ioctl(fd, _SOCKET_API_ASSOCIATE_SOCKET, &new_args, sizeof(new_args), false);
				if(err < 0) {
					socket_close(id);
					break;
				}

				err = fd;
				break;
			}
			case _SOCKET_API_RECVFROM:
				err = socket_recvfrom(s->id, args.u.transfer.buf, args.u.transfer.len, args.u.transfer.saddr);
				break;
			case _SOCKET_API_RECVFROM_ETC:
				err = socket_recvfrom_etc(s->id, args.u.transfer.buf, args.u.transfer.len, args.u.transfer.saddr, args.u.transfer.flags, args.u.transfer.timeout);
				break;
			case _SOCKET_API_SENDTO:
				err = socket_sendto(s->id, args.u.transfer.buf, args.u.transfer.len, args.u.transfer.saddr);
				break;
			default:
				err = ERR_INVALID_ARGS;
		}
	}

	return err;
}

static ssize_t socket_dev_read(dev_cookie cookie, void *buf, off_t pos, ssize_t len)
{
	socket_dev *s = (socket_dev *)cookie;

	if(s->id >= 0)
		return socket_read(s->id, buf, len);
	else
		return ERR_NET_NOT_CONNECTED;
}

static ssize_t socket_dev_write(dev_cookie cookie, const void *buf, off_t pos, ssize_t len)
{
	socket_dev *s = (socket_dev *)cookie;

	if(s->id >= 0)
		return socket_write(s->id, buf, len);
	else
		return ERR_NET_NOT_CONNECTED;
}

static struct dev_calls socket_dev_hooks = {
	&socket_dev_open,
	&socket_dev_close,
	&socket_dev_freecookie,
	&socket_dev_seek,
	&socket_dev_ioctl,
	&socket_dev_read,
	&socket_dev_write,
	/* no paging from /dev/null */
	NULL,
	NULL,
	NULL
};

int socket_dev_init(void)
{
	// create device node
	devfs_publish_device("net/socket", NULL, &socket_dev_hooks);

	return 0;
}

