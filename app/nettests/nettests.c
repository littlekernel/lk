/*
 * Copyright (c) 2009 Travis Geiselbrecht
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


#include <app.h>
#include <debug.h>
#include <string.h>
#include <kernel/thread.h>

#include <lib/net/misc.h>
#include <lib/net/socket.h>

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

static int net_listen_server(void *arg)
{
	int err;
	sockaddr saddr;
	sock_id listen_sock;

	memset(&saddr, 0, sizeof(saddr));
	saddr.port = 9876;

	listen_sock = socket_create(SOCK_PROTO_TCP, 0);
	printf("socket_create returns %d\n", listen_sock);

	err = socket_bind(listen_sock, &saddr);	
	printf("socket_bind returns %d\n", err);

	err = socket_listen(listen_sock);
	printf("socket_listen returns %d\n", err);

	for (;;) {
		printf("going to accept socket\n");
		int newsocket = socket_accept(listen_sock, &saddr);
		printf("socket_accept returns %d\n", newsocket);

		const char *str = "what the fuck man\n";
		socket_write(newsocket, str, strlen(str));

		socket_close(newsocket);
	}

	return 0;
}

static int net_tests(int argc, cmd_args *argv) 
{
	if (argc < 2) {
		printf("not enough arguments\n");
		return -1;
	}

	if (!strcmp(argv[1].str, "listen")) {

		thread_resume(thread_create("listen server", &net_listen_server, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));
	}

	return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("net_tests", "test the net stack", (console_cmd)&net_tests)
STATIC_COMMAND_END(nettests);

#endif

static void nettests_init(const struct app_descriptor *app)
{
}

APP_START(nettests)
	.init = nettests_init,
	.flags = 0,
APP_END

