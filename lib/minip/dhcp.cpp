/*
 * Copyright (c) 2014 Brian Swetland
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "minip-internal.h"

#include <endian.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <malloc.h>
#include <platform.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define TRACE_DHCP 0

namespace {

typedef struct dhcp_msg {
    u8 opcode;
    u8 hwtype;  // hw addr type
    u8 hwalen;  // hw addr length
    u8 hops;
    u32 xid;    // txn id
    u16 secs;   // seconds since dhcp process start
    u16 flags;
    u32 ciaddr; // Client IP Address
    u32 yiaddr; // Your IP Address
    u32 siaddr; // Server IP Address
    u32 giaddr; // Gateway IP Address
    u8 chaddr[16];  // Client HW Address
    u8 sname[64];   // Server Hostname, AsciiZ
    u8 file[128];   // Boot File Name, AsciiZ
    u32 cookie;
    u8 options[0];
} dhcp_msg_t;

#define DHCP_FLAG_BROADCAST 0x8000

#define DHCP_REQUEST    1
#define DHCP_REPLY  2

#define OP_DHCPDISCOVER 1   // Client: Broadcast to find Server
#define OP_DHCPOFFER    2   // Server response to Discover
#define OP_DHCPREQUEST  3   // Client accepts offer
#define OP_DHCPDECLINE  4   // Client notifies address already in use
#define OP_DHCPACK  5   // Server confirms accept
#define OP_DHCPNAK  6   // Server disconfirms or lease expires
#define OP_DHCPRELEASE  7   // Client releases address

#define OPT_NET_MASK    1   // len 4, mask
#define OPT_ROUTERS 3   // len 4n, gateway0, ...
#define OPT_DNS     6   // len 4n, nameserver0, ...
#define OPT_HOSTNAME    12
#define OPT_REQUEST_IP  50  // len 4
#define OPT_MSG_TYPE    53  // len 1, type same as op
#define OPT_SERVER_ID   54  // len 4, server ident ipaddr
#define OPT_DONE    255

#define DHCP_CLIENT_PORT    68
#define DHCP_SERVER_PORT    67

#define HW_ETHERNET 1

class dhcp {
public:
    dhcp() = default;
    ~dhcp() = default;

    status_t start();

private:
    // must call the following two with the lock held
    status_t send_discover();
    status_t send_request(u32 server, u32 reqip);

    void udp_callback(void *data, size_t sz, uint32_t srcip, uint16_t srcport);
    static void dhcp_cb(void *data, size_t sz, uint32_t srcip, uint16_t srcport, void *arg);

    static int dhcp_thread(void *arg);


    Mutex lock_;
    udp_socket_t *dhcp_udp_handle_ = nullptr;
    thread_t *dhcp_thr_ = nullptr;

    volatile bool configured_ = false;
    uint32_t xid_ = rand();
    enum {
        INITIAL = 0,
        DISCOVER_SENT,
        RECV_OFFER,
        REQUEST_SENT,
        CONFIGURED,
    } state_ = INITIAL;
};

status_t dhcp::send_discover() {
    DEBUG_ASSERT(lock_.is_held());

    struct {
        dhcp_msg_t msg;
        u8 opt[128];
    } s = {};
    u8 *opt = s.opt;
    s.msg.opcode = DHCP_REQUEST;
    s.msg.hwtype = HW_ETHERNET;
    s.msg.hwalen = 6;
    s.msg.xid = xid_;
    s.msg.cookie = 0x63538263;
    minip_get_macaddr(s.msg.chaddr);

    *opt++ = OPT_MSG_TYPE;
    *opt++ = 1;
    *opt++ = OP_DHCPDISCOVER;

    const char *hostname = minip_get_hostname();
    if (hostname && hostname[0]) {
        size_t len = strlen(hostname);
        *opt++ = OPT_HOSTNAME;
        *opt++ = len;
        memcpy(opt, hostname, len);
        opt += len;
    }

    *opt++ = OPT_DONE;

#if TRACE_DHCP
    printf("sending dhcp discover\n");
#endif
    state_ = DISCOVER_SENT;
    status_t ret = udp_send(&s.msg, sizeof(dhcp_msg_t) + (opt - s.opt), dhcp_udp_handle_);
    if (ret != NO_ERROR) {
        printf("DHCP_DISCOVER failed: %d\n", ret);
    }

    return ret;
}

status_t dhcp::send_request(u32 server, u32 reqip) {
    DEBUG_ASSERT(lock_.is_held());

    struct {
        dhcp_msg_t msg;
        u8 opt[128];
    } s = {};
    u8 *opt = s.opt;
    s.msg.opcode = DHCP_REQUEST;
    s.msg.hwtype = HW_ETHERNET;
    s.msg.hwalen = 6;
    s.msg.xid = xid_;
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

    const char *hostname = minip_get_hostname();
    if (hostname && hostname[0]) {
        size_t len = strlen(hostname);
        *opt++ = OPT_HOSTNAME;
        *opt++ = len;
        memcpy(opt, hostname, len);
        opt += len;
    }

    *opt++ = OPT_DONE;

#if TRACE_DHCP
    printf("sending dhcp request\n");
#endif
    state_ = REQUEST_SENT;
    status_t ret = udp_send(&s.msg, sizeof(dhcp_msg_t) + (opt - s.opt), dhcp_udp_handle_);
    if (ret != NO_ERROR) {
        printf("DHCP_REQUEST failed: %d\n", ret);
    }

    return ret;
}

void dhcp::dhcp_cb(void *data, size_t sz, uint32_t srcip, uint16_t srcport, void *arg) {
    dhcp *d = (dhcp *)arg;

    d->udp_callback(data, sz, srcip, srcport);
}

void dhcp::udp_callback(void *data, size_t sz, uint32_t srcip, uint16_t srcport) {
    const dhcp_msg_t *msg = (dhcp_msg_t *)data;
    const u8 *opt;
    u32 netmask = 0;
    u32 gateway = 0;
    u32 dns = 0;
    u32 server = 0;
    int op = -1;

    AutoLock guard(lock_);

    // lossy testing for state machine transitions
    if (false) {
        int r = rand();
        if (r % 3) {
            printf("dropping packet for testing r %#x\n", r);
            return;
        }
    }

    if (sz < sizeof(dhcp_msg_t)) return;

    uint8_t mac[6];
    minip_get_macaddr(mac);
    if (memcmp(msg->chaddr, mac, 6)) return;

#if TRACE_DHCP
    printf("received DHCP op %d, len %zu, from p %d, ip=", msg->opcode, sz, srcport);
    printip(srcip);
    printf("\n");
#endif

    if (configured_) {
        printf("already configured\n");
        return;
    }
#if TRACE_DHCP
    printip_named("\tciaddr", msg->ciaddr);
    printip_named(" yiaddr", msg->yiaddr);
    printip_named(" siaddr", msg->siaddr);
    printip_named(" giaddr", msg->giaddr);
    printf(" chaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
           msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
           msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
    printf("\toptions: ");
#endif
    sz -= sizeof(dhcp_msg_t);
    opt = msg->options;
#if TRACE_DHCP
    printf("\toptions: ");
#endif
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
                if (opt[1] >= 4) memcpy(&gateway, opt + 2, 4);
                break;
            case OPT_DNS:
                if (opt[1] >= 4) memcpy(&dns, opt + 2, 4);
                break;
            case OPT_SERVER_ID:
                if (opt[1] == 4) memcpy(&server, opt + 2, 4);
                break;
            case OPT_DONE:
                goto done;
        }
        opt += opt[1] + 2;
        sz -= opt[1];
    }
done:
#if TRACE_DHCP
    printf("\n\t");
    if (server) printip_named("server", server);
    if (netmask) printip_named(" netmask", netmask);
    if (gateway) printip_named(" gateway", gateway);
    if (dns) printip_named(" dns", dns);
    printf("\n");
#endif
    if (state_ == DISCOVER_SENT) {
        if (op == OP_DHCPOFFER) {
#if TRACE_DHCP
            printip_named("dhcp: offer:", msg->yiaddr);
            printf("\n");
#endif
            if (server) {
                state_ = RECV_OFFER;
                send_request(server, msg->yiaddr);
            }
        }
    } else if (state_ == REQUEST_SENT) {
        if (op == OP_DHCPACK) {
#if TRACE_DHCP
            printip_named("dhcp: ack:", msg->yiaddr);
            printf("\n");
#endif
            printf("DHCP configured\n");
            minip_set_ipaddr(msg->yiaddr);
            if (netmask) {
                minip_set_netmask(netmask);
            }
            if (gateway) {
                minip_set_gateway(gateway);
            }
            state_ = CONFIGURED;
            configured_ = true;

            // signal that minip is ready to be used
            minip_set_configured();
        }
    }
}

int dhcp::dhcp_thread(void *arg) {
    dhcp *d = (dhcp *)arg;

    auto worker = [&]() {
        while (!d->configured_) {
            {
                AutoLock guard(d->lock_);
                switch (d->state_) {
                    case INITIAL:
                    case DISCOVER_SENT:
                        // for these two states, start off by sending a discover packet
                        d->send_discover();
                        break;
                    case REQUEST_SENT:
                        // if we're still in this state after some period of time,
                        // switch back to the INITIAL state and start over.
                        d->state_ = INITIAL;
                        break;
                    default:
                        ;
                }
            }
            thread_sleep(500);
        }
    };

    worker();
    return 0;
}

status_t dhcp::start() {
    AutoLock guard(lock_);

    int ret = udp_open(IPV4_BCAST, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, &dhcp_udp_handle_);
    if (ret != NO_ERROR) {
        printf("DHCP: error opening udp socket\n");
        return ret;
    }

    udp_listen(DHCP_CLIENT_PORT, dhcp::dhcp_cb, this);

    dhcp_thr_ = thread_create("dhcp", dhcp::dhcp_thread, this, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
    thread_detach_and_resume(dhcp_thr_);

    return NO_ERROR;
}

} // anonymous namespace

void minip_start_dhcp() {
    static dhcp d;
    d.start();
}
