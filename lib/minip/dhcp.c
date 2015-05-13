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

#include "minip-internal.h"

#include <err.h>
#include <platform/gem.h>
#include <platform.h>
#include <stdio.h>
#include <debug.h>

#include <kernel/thread.h>
#include <sys/types.h>
#include <endian.h>
#include <string.h>

#define TRACE_DHCP 0

typedef struct dhcp_msg {
	u8 opcode;
	u8 hwtype;	// hw addr type
	u8 hwalen;	// hw addr length
	u8 hops;
	u32 xid;	// txn id
	u16 secs;	// seconds since dhcp process start
	u16 flags;
	u32 ciaddr;	// Client IP Address
	u32 yiaddr;	// Your IP Address
	u32 siaddr;	// Server IP Address
	u32 giaddr;	// Gateway IP Address
	u8 chaddr[16];	// Client HW Address
	u8 sname[64];   // Server Hostname, AsciiZ
	u8 file[128];	// Boot File Name, AsciiZ
	u32 cookie;
	u8 options[0];
} dhcp_msg_t;

udp_socket_t *dhcp_udp_handle;

#define DHCP_FLAG_BROADCAST 0x8000

#define DHCP_REQUEST	1
#define DHCP_REPLY	2

#define OP_DHCPDISCOVER	1	// Client: Broadcast to find Server
#define OP_DHCPOFFER	2	// Server response to Discover
#define OP_DHCPREQUEST	3	// Client accepts offer
#define OP_DHCPDECLINE	4	// Client notifies address already in use
#define OP_DHCPACK	5	// Server confirms accept
#define OP_DHCPNAK	6	// Server disconfirms or lease expires
#define OP_DHCPRELEASE	7	// Client releases address

#define OPT_NET_MASK	1	// len 4, mask
#define OPT_ROUTERS	3	// len 4n, gateway0, ...
#define OPT_DNS		6	// len 4n, nameserver0, ...
#define OPT_HOSTNAME	12
#define OPT_REQUEST_IP	50	// len 4
#define OPT_MSG_TYPE	53	// len 1, type same as op
#define OPT_SERVER_ID	54	// len 4, server ident ipaddr
#define OPT_DONE	255

#define DHCP_CLIENT_PORT	68
#define DHCP_SERVER_PORT	67

#define HW_ETHERNET	1

static u8 mac[6];

static void printip(const char *name, u32 x) {
	union {
		u32 u;
		u8 b[4];
	} ip;
	ip.u = x;
	printf("%s %d.%d.%d.%d\n", name, ip.b[0], ip.b[1], ip.b[2], ip.b[3]);
}

static volatile int configured = 0;
static int cfgstate = 0;

static void dhcp_discover(u32 xid) {
	struct {
		dhcp_msg_t msg;
		u8 opt[128];
	} s;
	u8 *opt = s.opt;
	const char *hostname = minip_get_hostname();
	memset(&s, 0, sizeof(s));
	s.msg.opcode = DHCP_REQUEST;
	s.msg.hwtype = HW_ETHERNET;
	s.msg.hwalen = 6;
	s.msg.xid = xid;
	s.msg.cookie = 0x63538263;
	minip_get_macaddr(s.msg.chaddr);

	*opt++ = OPT_MSG_TYPE;
	*opt++ = 1;
	*opt++ = OP_DHCPDISCOVER;

	if (hostname && hostname[0]) {
		size_t len = strlen(hostname);
		*opt++ = OPT_HOSTNAME;
		*opt++ = len;
		memcpy(opt, hostname, len);
		opt += len;
	}

	*opt++ = OPT_DONE;

	udp_send(&s.msg, sizeof(dhcp_msg_t) + (opt - s.opt), dhcp_udp_handle);
	status_t ret = udp_send(&s.msg, sizeof(dhcp_msg_t) + (opt - s.opt), dhcp_udp_handle);
	if (ret != NO_ERROR) {
		printf("DHCP_DISCOVER failed: %d\n", ret);
	}
}

static void dhcp_request(u32 xid, u32 server, u32 reqip) {
	struct {
		dhcp_msg_t msg;
		u8 opt[128];
	} s;
	u8 *opt = s.opt;
	const char *hostname = minip_get_hostname();
	memset(&s, 0, sizeof(s));
	s.msg.opcode = DHCP_REQUEST;
	s.msg.hwtype = HW_ETHERNET;
	s.msg.hwalen = 6;
	s.msg.xid = xid;
	s.msg.cookie = 0x63538263;
	minip_get_macaddr(s.msg.chaddr);

	*opt++ = OPT_MSG_TYPE;
	*opt++ = 1;
	*opt++ = OP_DHCPREQUEST;

	*opt++ = OPT_SERVER_ID;
	*opt++ = 4;
	memcpy(opt, &server, 4);
	opt += 4;

	*opt++ = OPT_REQUEST_IP;
	*opt++ = 4;
	memcpy(opt, &reqip, 4);
	opt += 4;

	if (hostname && hostname[0]) {
		size_t len = strlen(hostname);
		*opt++ = OPT_HOSTNAME;
		*opt++ = len;
		memcpy(opt, hostname, len);
		opt += len;
	}

	*opt++ = OPT_DONE;

	status_t ret = udp_send(&s.msg, sizeof(dhcp_msg_t) + (opt - s.opt), dhcp_udp_handle);
	if (ret != NO_ERROR) {
		printf("DHCP_REQUEST failed: %d\n", ret);
	}
}

static void dhcp_cb(void *data, size_t sz, uint32_t srcip, uint16_t srcport, void *arg) {
	dhcp_msg_t *msg = data;
	u8 *opt;
	u32 netmask = 0;
	u32 gateway = 0;
	u32 dns = 0;
	u32 server = 0;
	int op = -1;

	if (sz < sizeof(dhcp_msg_t)) return;

	if (memcmp(msg->chaddr, mac, 6)) return;

#if TRACE_DHCP
	printf("dhcp op=%d len=%d from p=%d ip=", msg->opcode, sz, srcport);
	printip("", srcip);
#endif

	if (configured) {
		printf("already configured\n");
		return;
	}
#if TRACE_DHCP
	printip("ciaddr", msg->ciaddr);
	printip("yiaddr", msg->yiaddr);
	printip("siaddr", msg->siaddr);
	printip("giaddr", msg->giaddr);
	printf("chaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
		msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
		msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
#endif
	sz -= sizeof(dhcp_msg_t);
	opt = msg->options;
	while (sz >= 2) {
		sz -= 2;
		if (opt[1] > sz) {
			break;
		}
#if TRACE_DHCP
		printf("#%d (%d), ", opt[0], opt[1]);
#endif
		switch (opt[0]) {
		case OPT_MSG_TYPE:
			if (opt[1] == 1) op = opt[2];
			break;	
		case OPT_NET_MASK:
			if (opt[1] == 4) memcpy(&netmask, opt + 2, 4);
			break;
		case OPT_ROUTERS:
			if (opt[1] >= 4) memcpy(&gateway, opt + 3, 4);
			break;
		case OPT_DNS:
			if (opt[1] >= 4) memcpy(&dns, opt + 3, 4);
			break;
		case OPT_SERVER_ID:
			if (opt[1] == 4) memcpy(&server, opt + 3, 4);
			break;
		case OPT_DONE:
			goto done;
		}
		opt += opt[1] + 2;
		sz -= opt[1];
	}
done:
#if TRACE_DHCP
	printf("\n");
	if (server) printip("server", server);
	if (netmask) printip("netmask", netmask);
	if (gateway) printip("gateway", gateway);
	if (dns) printip("dns", dns);
#endif
	if (cfgstate == 0) {
		if (op == OP_DHCPOFFER) {
			printip("dhcp: offer:", msg->yiaddr);
			if (server) {
				dhcp_request(0xaabbccdd, server, msg->yiaddr);
				cfgstate = 1;
			}
		}
	} else if (cfgstate == 1) {
		if (op == OP_DHCPACK) {
			printip("dhcp: ack:", msg->yiaddr);
			minip_set_ipaddr(msg->yiaddr);
			configured = 1;
		}
	}
}

static int dhcp_thread(void *arg) {
	for (;;) {
		if (configured) break;
		thread_sleep(500);
		if (configured) break;
		dhcp_discover(0xaabbccdd);
	}
	return 0;
}

static thread_t *dhcp_thr;

void minip_init_dhcp(tx_func_t tx_func, void *tx_arg) {
	minip_get_macaddr(mac);

	minip_init(tx_func, tx_arg, IPV4_NONE, IPV4_NONE, IPV4_NONE);

	int ret = udp_open(IPV4_BCAST, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, &dhcp_udp_handle);
	printf("dhcp opened udp: %d\n", ret);

	udp_listen(DHCP_CLIENT_PORT, dhcp_cb, NULL);

	dhcp_thr = thread_create("dhcp", dhcp_thread, NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
	thread_detach_and_resume(dhcp_thr);
}

// vim: set noexpandtab:
