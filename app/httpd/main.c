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
#include <string.h>
#include <kernel/thread.h>
#include <lwip/inet.h>
#include <lwip/sockets.h>

static int listen_socket;

static const char http_msg[] =
	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n"
	"<html>\n"
	"</html>\n";

static int handle_request(int socket)
{
	unsigned char buf[64];
	int err;

	memset(buf, 0, sizeof(buf));
	err = lwip_read(socket, buf, sizeof(buf));
	dprintf("handle_request: read %d bytes from socket\n", err);

	lwip_write(socket, http_msg, sizeof(http_msg));

	return 0;
}

static int http_server_thread(void *arg)
{
	int err;
	int new_socket;

	listen_socket = lwip_socket(AF_INET, SOCK_STREAM, 0);
	dprintf("listen_socket %d\n", listen_socket);

	struct sockaddr_in addr;
	socklen_t addrlen;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(80);

	err = lwip_bind(listen_socket, (struct sockaddr *)&addr, sizeof(addr));
	dprintf("lwip_bind returns %d\n", err);

	err = lwip_listen(listen_socket, 4);
	dprintf("lwip_listen returns %d\n", err);

	while ((new_socket = lwip_accept(listen_socket, (struct sockaddr *)&addr, &addrlen) >= 0)) {
		dprintf("new connection on socket %d\n", new_socket);
		handle_request(new_socket);
		lwip_close(new_socket);
	}

	dprintf("http server shutting down\n");

	lwip_close(listen_socket);

	return 0;
}

int httpd_init(void)
{
	thread_resume(thread_create("httpd server", &http_server_thread, NULL, DEFAULT_PRIORITY + 5, DEFAULT_STACK_SIZE));

	return 0;
}

