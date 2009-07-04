/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
 * Copyright (c) 2008-2009 Travis Geiselbrecht
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
#include <err.h>
#include <stdlib.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/event.h>
#include <lib/net/misc.h>
#include <lib/net/socket.h>
#include <lib/net/hash.h>
#include <lib/net/udp.h>
#include <lib/net/tcp.h>

typedef struct netsocket {
	struct netsocket *next;
	sock_id id;
	int type;
	void *prot_data;
} netsocket;

static netsocket *sock_table;
static mutex_t sock_mutex;
static sock_id next_sock_id;

static int sock_compare_func(void *_s, const void *_key)
{
	netsocket *s = _s;
	const sock_id *id = _key;

	if(s->id == *id)
		return 0;
	else
		return 1;
}

static unsigned int sock_hash_func(void *_s, const void *_key, unsigned int range)
{
	netsocket *s = _s;
	const sock_id *id = _key;

	if(s)
		return s->id % range;
	else
		return *id % range;
}

static netsocket *lookup_socket(sock_id id)
{
	netsocket *s;

	// pull it from in the socket list
	mutex_acquire(&sock_mutex);
	s = hash_lookup(sock_table, &id);
	mutex_release(&sock_mutex);

	return s;
}

static netsocket *create_netsocket(int type, void *prot_data)
{
	netsocket *s;

	// create the net socket
	s = malloc(sizeof(netsocket));
	if(!s)
		return NULL;

	// set it up
	s->id = atomic_add(&next_sock_id, 1);
	s->prot_data = prot_data;
	s->type = type;

	// insert it in the socket list
	mutex_acquire(&sock_mutex);
	hash_insert(sock_table, s);
	mutex_release(&sock_mutex);

	return s;
}

static int destroy_netsocket(netsocket *s)
{
	mutex_acquire(&sock_mutex);
	hash_remove(sock_table, s);
	mutex_release(&sock_mutex);

	free(s);

	return 0;
}

sock_id socket_create(int type, int flags)
{
	netsocket *s;
	void *prot_data;
	int err;

	switch(type) {
		case SOCK_PROTO_UDP:
			err = udp_open(&prot_data);
			break;
		case SOCK_PROTO_TCP:
			err = tcp_open(&prot_data);
			break;
		default:
			prot_data = NULL;
			err = ERR_INVALID_ARGS;
	}

	if(err < 0)
		return err;

	s = create_netsocket(type, prot_data);
	if(!s) {
		err = ERR_NO_MEMORY;
		goto err;
 	}

	return s->id;

err:
	switch(type) {
		case SOCK_PROTO_UDP:
			udp_close(prot_data);
			break;
		case SOCK_PROTO_TCP:
			tcp_close(prot_data);
			break;
	}
	return err;
}

int socket_bind(sock_id id, sockaddr *addr)
{
	netsocket *s;
	ssize_t err;

	s = lookup_socket(id);
	if(!s)
		return ERR_INVALID_HANDLE;

	switch(s->type) {
		case SOCK_PROTO_UDP:
			err = udp_bind(s->prot_data, addr);
			break;
		case SOCK_PROTO_TCP:
			err = tcp_bind(s->prot_data, addr);
			break;
		default:
			err = ERR_INVALID_ARGS;
	}
	return err;
}

int socket_connect(sock_id id, sockaddr *addr)
{
	netsocket *s;
	ssize_t err;

	s = lookup_socket(id);
	if(!s)
		return ERR_INVALID_HANDLE;

	switch(s->type) {
		case SOCK_PROTO_UDP:
			err = udp_connect(s->prot_data, addr);
			break;
		case SOCK_PROTO_TCP:
			err = tcp_connect(s->prot_data, addr);
			break;
		default:
			err = ERR_INVALID_ARGS;
	}
	return err;
}

int socket_listen(sock_id id)
{
	netsocket *s;
	ssize_t err;

	s = lookup_socket(id);
	if(!s)
		return ERR_INVALID_HANDLE;

	switch(s->type) {
		case SOCK_PROTO_UDP:
			err = udp_listen(s->prot_data);
			break;
		case SOCK_PROTO_TCP:
			err = tcp_listen(s->prot_data);
			break;
		default:
			err = ERR_INVALID_ARGS;
	}
	return err;
}

int socket_accept(sock_id id, sockaddr *saddr)
{
	netsocket *s;
	netsocket *new_socket;
	void *new_prot_data;
	ssize_t err;

	s = lookup_socket(id);
	if(!s)
		return ERR_INVALID_HANDLE;

	switch(s->type) {
		case SOCK_PROTO_UDP:
			err = udp_accept(s->prot_data, saddr, &new_prot_data);
			break;
		case SOCK_PROTO_TCP:
			err = tcp_accept(s->prot_data, saddr, &new_prot_data);
			break;
		default:
			err = ERR_INVALID_ARGS;
	}

	if(err < 0)
		return err;

	new_socket = create_netsocket(s->type, new_prot_data);
	if(!new_socket) {
		// XXX lets close this one
		return ERR_NO_MEMORY;
	}

	return new_socket->id;
}

ssize_t socket_read(sock_id id, void *buf, ssize_t len)
{
	return socket_recvfrom_etc(id, buf, len, NULL, 0, 0);
}

ssize_t socket_recvfrom(sock_id id, void *buf, ssize_t len, sockaddr *addr)
{
	return socket_recvfrom_etc(id, buf, len, addr, 0, 0);
}

ssize_t socket_recvfrom_etc(sock_id id, void *buf, ssize_t len, sockaddr *addr, int flags, bigtime_t timeout)
{
	netsocket *s;
	ssize_t err;

	s = lookup_socket(id);
	if(!s)
		return ERR_INVALID_HANDLE;

	switch(s->type) {
		case SOCK_PROTO_UDP:
			err = udp_recvfrom(s->prot_data, buf, len, addr, flags, timeout);
			break;
		case SOCK_PROTO_TCP:
			err = tcp_recvfrom(s->prot_data, buf, len, addr, flags, timeout);
			break;
		default:
			err = ERR_INVALID_ARGS;
	}
	return err;
}

ssize_t socket_write(sock_id id, const void *buf, ssize_t len)
{
	return socket_sendto(id, buf, len, NULL);
}

ssize_t socket_sendto(sock_id id, const void *buf, ssize_t len, sockaddr *addr)
{
	netsocket *s;
	ssize_t err;

	s = lookup_socket(id);
	if(!s)
		return ERR_INVALID_HANDLE;

	switch(s->type) {
		case SOCK_PROTO_UDP:
			err = udp_sendto(s->prot_data, buf, len, addr);
			break;
		case SOCK_PROTO_TCP:
			err = tcp_sendto(s->prot_data, buf, len, addr);
			break;
		default:
			err = ERR_INVALID_ARGS;
	}
	return err;
}

int socket_close(sock_id id)
{
	netsocket *s;
	ssize_t err;

	s = lookup_socket(id);
	if(!s)
		return ERR_INVALID_HANDLE;

	switch(s->type) {
		case SOCK_PROTO_UDP:
			err = udp_close(s->prot_data);
			break;
		case SOCK_PROTO_TCP:
			err = tcp_close(s->prot_data);
			break;
		default:
			err = ERR_INVALID_ARGS;
	}

	if(err >= 0) {
		// free the socket
		destroy_netsocket(s);
	}

	return err;
}

int socket_init(void)
{
	next_sock_id = 0;

	mutex_init(&sock_mutex);

	sock_table = hash_init(256, offsetof(netsocket, next), &sock_compare_func, &sock_hash_func);
	if(!sock_table)
		return ERR_NO_MEMORY;

//	socket_dev_init();

	return 0;
}

