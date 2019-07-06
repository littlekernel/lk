/*
 * Copyright (c) 2014 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

int udp_listen(in_addr_t addr, unsigned port, int shared);
int udp_connect(in_addr_t addr, unsigned port);

int tcp_listen(in_addr_t addr, unsigned port);
int tcp_connect(in_addr_t addr, unsigned port);

in_addr_t lookup_hostname(const char *hostname);
