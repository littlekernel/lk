/*
 * Copyright (c) 2014 Chris Anderson
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "minip-internal.h"

#include <lk/console_cmd.h>
#include <kernel/thread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <platform.h>
#include <kernel/timer.h>
#include <lk/err.h>

static void arp_usage(void) {
    printf("arp list                        print arp table\n");
    printf("arp query <ipv4 address>        query arp address\n");
}

static int cmd_arp(int argc, const console_cmd_args *argv) {
    if (argc == 1) {
        arp_usage();
        return -1;
    }

    const char *cmd = argv[1].str;
    if (argc == 2 && strncmp(cmd, "list", sizeof("list")) == 0) {
        arp_cache_dump();
    } else if (argc == 3 && strncmp(cmd, "query", sizeof("query")) == 0) {
        const char *addr_s = argv[2].str;
        ipv4_addr_t addr;

        if (minip_parse_ipaddr_checked(addr_s, strlen(addr_s), &addr) < 0) {
            printf("bad ipv4 address '%s'\n", addr_s);
            return -1;
        }

        arp_get_dest_mac(addr);
    } else {
        arp_usage();
    }

    return 0;
}

static int cmd_minip(int argc, const console_cmd_args *argv) {
    if (argc == 1) {
minip_usage:
        printf("minip commands\n");
        printf("mi tra[c]e                      toggle packet tracing\n");
        printf("mi [i]interfaces                dump interface list\n");
        printf("mi [r]outes                     dump routing table\n");
        printf("mi [s]tatus                     print ip status\n");
        printf("mi [d]ns <hostname>             resolve a host name\n");
        printf("mi [t]est [dest] [port] [cnt]   send <cnt> test packets to the dest:port\n");
    } else {
        const char *cmd = argv[1].str;
        char sel = cmd[0];

        /* Dispatch on the whole word when one is given: matching on the
         * first letter alone made 'mi trace' run the packet blast test.
         */
        if (cmd[1] != '\0') {
            if (!strcmp(cmd, "trace")) {
                sel = 'c';
            } else if (!strcmp(cmd, "interfaces")) {
                sel = 'i';
            } else if (!strcmp(cmd, "routes")) {
                sel = 'r';
            } else if (!strcmp(cmd, "status")) {
                sel = 's';
            } else if (!strcmp(cmd, "dns")) {
                sel = 'd';
            } else if (!strcmp(cmd, "test")) {
                sel = 't';
            } else {
                goto minip_usage;
            }
        }

        switch (sel) {
            case 'c':
                minip_trace = !minip_trace;
                printf("packet tracing: %s\n", minip_trace ? "enabled" : "disabled");
                break;
            case 'i':
                netif_dump();
                break;
            case 'r':
                dump_ipv4_route_table();
                break;
            case 's':
                printf("hostname: %s\n", minip_get_hostname());
                printf("gateway: %u.%u.%u.%u\n", IPV4_SPLIT(minip_get_gateway()));
                printf("dns server: %u.%u.%u.%u\n", IPV4_SPLIT(minip_get_dns_server()));
                printf("interfaces:\n");
                netif_dump();
                printf("ipv4 routing table:\n");
                dump_ipv4_route_table();
                break;
            case 'd': {
                if (argc < 3) {
                    goto minip_usage;
                }

                ipv4_addr_t addr;
                lk_time_t t = current_time();
                status_t err = dns_resolve(argv[2].str, &addr, DNS_DEFAULT_TIMEOUT);
                t = current_time() - t;

                if (err < 0) {
                    printf("failed to resolve '%s': %d\n", argv[2].str, err);
                    return err;
                }
                printf("%s is %u.%u.%u.%u (%u ms)\n", argv[2].str, IPV4_SPLIT(addr), t);
                break;
            }
            case 't': {
                uint32_t count = 1;
                uint32_t host = 0x0100000A; // 10.0.0.1
                uint32_t port = 1025;
                udp_socket_t *handle;

                switch (argc) {
                    case 5:
                        count = argv[4].u;
                    /* fallthrough */
                    case 4:
                        port = argv[3].u;
                    /* fallthrough */
                    case 3:
                        if (minip_resolve(argv[2].str, &host) < 0) {
                            printf("failed to resolve '%s'\n", argv[2].str);
                            return -1;
                        }
                        break;
                }

                if (udp_open(host, port, port, &handle) != NO_ERROR) {
                    printf("udp_open to %u.%u.%u.%u:%u failed\n", IPV4_SPLIT(host), port);
                    return -1;
                }

#define BUFSIZE 1470
                uint8_t *buf;

                buf = malloc(BUFSIZE);
                if (!buf) {
                    udp_close(handle);
                    return -1;
                }

                memset(buf, 0x00, BUFSIZE);
                printf("sending %u packet(s) to %u.%u.%u.%u:%u\n", count, IPV4_SPLIT(host), port);

                lk_bigtime_t t = current_time_hires();
                uint32_t failures = 0;
                for (uint32_t i = 0; i < count; i++) {
                    if (udp_send(buf, BUFSIZE, handle) != 0) {
                        failures++;
                    }
                    buf[128]++;
                }
                t = current_time_hires() - t;
                if (t == 0)
                    t++;
                printf("%d pkts failed\n", failures);
                uint64_t total_count = (uint64_t)count * BUFSIZE;
                printf("wrote %llu bytes in %u msecs (%llu bytes/sec)\n",
                       total_count, (uint32_t)t, total_count * 1000000 / t);

                free(buf);
                udp_close(handle);
#undef BUFSIZE
            }
                break;
            default:
                goto minip_usage;
        }
    }

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("arp", "arp commands", &cmd_arp)
STATIC_COMMAND("mi", "minip commands", &cmd_minip)
STATIC_COMMAND("tcp", "tcp commands", &cmd_tcp)
STATIC_COMMAND_END(minip);
