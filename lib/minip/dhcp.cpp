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

struct dhcp_msg {
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
};

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

#define OPT_PAD     0
#define OPT_NET_MASK    1   // len 4, mask
#define OPT_ROUTERS 3   // len 4n, gateway0, ...
#define OPT_DNS     6   // len 4n, nameserver0, ...
#define OPT_HOSTNAME    12
#define OPT_REQUEST_IP  50  // len 4
#define OPT_MSG_TYPE    53  // len 1, type same as op
#define OPT_SERVER_ID   54  // len 4, server ident ipaddr
#define OPT_CLASSLESS_STATIC_ROUTE 121 // RFC3442 classless static routes
#define OPT_DONE    255

#define DHCP_CLIENT_PORT    68
#define DHCP_SERVER_PORT    67

#define HW_ETHERNET 1

class dhcp {
public:
    dhcp() = default;
    ~dhcp() = default;

    status_t start(netif_t *netif);

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
    netif_t *netif_ = nullptr;

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
        dhcp_msg msg;
        u8 opt[128];
    } s = {};
    u8 *opt = s.opt;
    s.msg.opcode = DHCP_REQUEST;
    s.msg.hwtype = HW_ETHERNET;
    s.msg.hwalen = 6;
    s.msg.xid = xid_;
    s.msg.cookie = 0x63538263;
    mac_addr_copy(s.msg.chaddr, netif_->mac_address);

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
    status_t ret = udp_send(&s.msg, sizeof(dhcp_msg) + (opt - s.opt), dhcp_udp_handle_);
    if (ret != NO_ERROR) {
        printf("DHCP_DISCOVER failed: %d\n", ret);
    }

    return ret;
}

status_t dhcp::send_request(u32 server, u32 reqip) {
    DEBUG_ASSERT(lock_.is_held());

    struct {
        dhcp_msg msg;
        u8 opt[128];
    } s = {};
    u8 *opt = s.opt;
    s.msg.opcode = DHCP_REQUEST;
    s.msg.hwtype = HW_ETHERNET;
    s.msg.hwalen = 6;
    s.msg.xid = xid_;
    s.msg.cookie = 0x63538263;
    mac_addr_copy(s.msg.chaddr, netif_->mac_address);

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
    status_t ret = udp_send(&s.msg, sizeof(dhcp_msg) + (opt - s.opt), dhcp_udp_handle_);
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
    const dhcp_msg *msg = (dhcp_msg *)data;
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

    if (sz < sizeof(dhcp_msg)) return;

    if (memcmp(msg->chaddr, netif_->mac_address, sizeof(netif_->mac_address))) return;

#if TRACE_DHCP
    printf("received DHCP op %d, len %zu, from p %d, ip=", msg->opcode, sz, srcport);
    print_ipv4_address(srcip);
    printf("\n");
#endif

    if (configured_) {
        printf("already configured\n");
        return;
    }
#if TRACE_DHCP
    print_ipv4_address_named("\tciaddr", msg->ciaddr);
    print_ipv4_address_named(" yiaddr", msg->yiaddr);
    print_ipv4_address_named(" siaddr", msg->siaddr);
    print_ipv4_address_named(" giaddr", msg->giaddr);
    printf(" chaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
           msg->chaddr[0], msg->chaddr[1], msg->chaddr[2],
           msg->chaddr[3], msg->chaddr[4], msg->chaddr[5]);
    printf("\toptions: ");
#endif
    dhcp_options_t options;
    dhcp_parse_options(msg->options, sz - sizeof(dhcp_msg), &options);
    op = options.op;
    netmask = options.netmask;
    gateway = options.gateway;
    dns = options.dns;
    server = options.server;

#if TRACE_DHCP
    printf("\n\t");
    if (server) print_ipv4_address_named("server", server);
    if (netmask) print_ipv4_address_named(" netmask", netmask);
    if (gateway) print_ipv4_address_named(" gateway", gateway);
    if (dns) print_ipv4_address_named(" dns", dns);
    printf("\n");
#endif
    if (state_ == DISCOVER_SENT) {
        if (op == OP_DHCPOFFER) {
#if TRACE_DHCP
            print_ipv4_address_named("dhcp: offer:", msg->yiaddr);
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
            print_ipv4_address_named("dhcp: ack:", msg->yiaddr);
            printf("\n");
#endif
            printf("DHCP configured\n");
            uint8_t netwidth = 32;
            if (netmask && ~netmask) {
                // note: a /32 lease leaves ~netmask == 0, where __builtin_ctz
                // is undefined; the initializer above already covers it.
                netwidth = __builtin_ctz(~netmask);
                //printf("netmask %#x netwidth %u\n", netmask, netwidth);
            }
            netif_set_ipv4_addr(netif_, msg->yiaddr, netwidth);
            if (gateway) {
                minip_set_gateway(gateway);
            }
            if (dns) {
                minip_set_dns_server(dns);
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

status_t dhcp::start(netif_t *netif) {
    AutoLock guard(lock_);

    netif_ = netif;

    // open a udp socket bound to the interface we're dhcping
    int ret = udp_open_raw(IPV4_BCAST, DHCP_CLIENT_PORT, DHCP_SERVER_PORT, netif_, &dhcp_udp_handle_);
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

/* Walk the option area of a DHCP message, picking out the handful of options
 * minip acts on. Kept out of the state machine (and out of the anonymous
 * namespace) so it can be fed canned blobs by the unit tests.
 *
 * Addresses are assembled with IPV4_PACK() rather than copied in as four
 * raw bytes: an ipv4_addr_t holds the first octet in its low byte, which on
 * a big endian target is not what the wire order lands as.
 */
void dhcp_parse_options(const void *_options, size_t len, dhcp_options_t *out) {
    DEBUG_ASSERT(_options || len == 0);
    DEBUG_ASSERT(out);

    const u8 *opt = (const u8 *)_options;

    memset(out, 0, sizeof(*out));
    out->op = -1;

    while (len > 0) {
        u8 code = opt[0];

        // Pad and end are single-byte options.
        if (code == OPT_PAD) {
            opt++;
            len--;
            continue;
        }
        if (code == OPT_DONE) {
            break;
        }

        if (len < 2) {
            break;
        }
        u8 optlen = opt[1];
        if (len < static_cast<size_t>(2 + optlen)) {
            break;
        }
#if TRACE_DHCP
        printf("#%d (%d), ", code, optlen);
#endif
        switch (code) {
            case OPT_MSG_TYPE:
                if (optlen == 1) out->op = opt[2];
                break;
            case OPT_NET_MASK:
                if (optlen == 4) out->netmask = IPV4_PACK(opt + 2);
                break;
            case OPT_ROUTERS:
                if (optlen >= 4) out->gateway = IPV4_PACK(opt + 2);
                break;
            case OPT_DNS:
                if (optlen >= 4) out->dns = IPV4_PACK(opt + 2);
                break;
            case OPT_SERVER_ID:
                if (optlen == 4) out->server = IPV4_PACK(opt + 2);
                break;
            case OPT_CLASSLESS_STATIC_ROUTE: {
                // RFC3442: list of [prefix-width][dest bytes][router-ip].
                // If present and option 3 is absent, use the default route (0/0).
                const u8 *p = opt + 2;
                size_t left = optlen;
                while (left > 0) {
                    u8 prefix_width = p[0];
                    p++;
                    left--;

                    size_t dst_bytes = (prefix_width + 7) / 8;
                    if (left < dst_bytes + 4) {
                        break;
                    }

                    if ((prefix_width == 0) && (out->gateway == 0)) {
                        out->gateway = IPV4_PACK(p + dst_bytes);
                    }

                    p += dst_bytes + 4;
                    left -= dst_bytes + 4;
                }
                break;
            }
            default:
                break;
        }
        opt += optlen + 2;
        len -= static_cast<size_t>(optlen + 2);
    }
}

void minip_start_dhcp(netif_t *netif) {
    DEBUG_ASSERT(netif);

    auto *d = new dhcp();
    d->start(netif);
}
