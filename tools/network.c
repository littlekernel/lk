/*
 * Copyright (c) 2014 Brian Swetland
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "network.h"

in_addr_t lookup_hostname(const char *hostname)
{
    int err;
    struct addrinfo *info, *temp;
    in_addr_t addr = 0;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    err = getaddrinfo(hostname, NULL, &hints, &info);
    if (err < 0) {
        printf("getaddrinfo() returns %d, error '%s'\n", err, gai_strerror(err));
        return 0;
    }

    for (temp = info; temp; temp = temp->ai_next) {
//      printf("flags 0x%x, family %d, socktype %d, protocol %d, addrlen %d\n",
//          temp->ai_flags, temp->ai_family, temp->ai_socktype, temp->ai_protocol, temp->ai_addrlen);

        if (temp->ai_family == AF_INET && temp->ai_protocol == IPPROTO_TCP) {
            struct sockaddr_in *sa = (struct sockaddr_in *)temp->ai_addr;
//          printf("port %d, addr 0x%x\n", sa->sin_port, sa->sin_addr.s_addr);
            addr = sa->sin_addr.s_addr;
        }
    }

    freeaddrinfo(info);

    return addr;
}

static int inet_listen(in_addr_t addr, int type, unsigned port, int shared)
{
    struct sockaddr_in sa;
    int s;
    if ((s = socket(AF_INET, type, 0)) < 0) {
        return -1;
    }
    if (shared) {
        int opt = 1;
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        //setsockopt(s, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
    }
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = addr;
    if (bind(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        close(s);
        return -1;
    }
    return s;
}

static int inet_connect(in_addr_t addr, int type, unsigned port)
{
    struct sockaddr_in sa;
    int s;
    if ((s = socket(AF_INET, type, 0)) < 0) {
        return -1;
    }
    if (addr == 0xFFFFFFFF) {
        int opt = 1;
        setsockopt(s, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    }
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = addr;
    if (connect(s, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
        close(s);
        return -1;
    }
    return s;
}

int udp_listen(in_addr_t addr, unsigned port, int shared)
{
    return inet_listen(addr, SOCK_DGRAM, port, shared);
}

int udp_connect(in_addr_t addr, unsigned port)
{
    return inet_connect(addr, SOCK_DGRAM, port);
}

int tcp_listen(in_addr_t addr, unsigned port)
{
    return inet_listen(addr, SOCK_STREAM, port, 0);
}

int tcp_connect(in_addr_t addr, unsigned port)
{
    return inet_connect(addr, SOCK_STREAM, port);
}
