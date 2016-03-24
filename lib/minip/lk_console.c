/*
 * Copyright (c) 2014 Chris Anderson
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

#include <lib/console.h>
#include <kernel/thread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <platform.h>
#include <kernel/timer.h>
#include <err.h>

#if WITH_LIB_CONSOLE

uint32_t str_ip_to_int(const char *s, size_t len)
{
    uint8_t ip[4] = { 0, 0, 0, 0 };
    uint8_t pos = 0, i = 0;

    while (pos < len) {
        char c = s[pos];
        if (c == '.') {
            i++;
        } else {
            ip[i] *= 10;
            ip[i] += c - '0';
        }
        pos++;
    }

    return IPV4_PACK(ip);
}

void arp_usage(void)
{
    printf("arp list                        print arp table\n");
    printf("arp query <ipv4 address>        query arp address\n");
}

static int cmd_arp(int argc, const cmd_args *argv)
{
    const char *cmd;

    if (argc == 1) {
        arp_usage();
        return -1;
    }

    cmd = argv[1].str;
    if (argc == 2 && strncmp(cmd, "list", sizeof("list")) == 0) {
        arp_cache_dump();
    } else if (argc == 3 && strncmp(cmd, "query", sizeof("query")) == 0) {
        const char *addr_s = argv[2].str;
        uint32_t addr = str_ip_to_int(addr_s, strlen(addr_s));

        arp_send_request(addr);
    } else {
        arp_usage();
    }

    return 0;
}

static int cmd_minip(int argc, const cmd_args *argv)
{
    if (argc == 1) {
minip_usage:
        printf("minip commands\n");
        printf("mi [a]rp                        dump arp table\n");
        printf("mi [s]tatus                     print ip status\n");
        printf("mi [t]est [dest] [port] [cnt]   send <cnt> test packets to the dest:port\n");
    } else {
        switch (argv[1].str[0]) {

            case 'a':
                arp_cache_dump();
                break;

            case 's': {
                uint32_t ipaddr = minip_get_ipaddr();

                printf("hostname: %s\n", minip_get_hostname());
                printf("ip: %u.%u.%u.%u\n", IPV4_SPLIT(ipaddr));
            }
            break;
            case 't': {
                uint32_t count = 1;
                uint32_t host = 0x0100000A; // 10.0.0.1
                uint32_t port = 1025;
                udp_socket_t *handle;

                switch (argc) {
                    case 5:
                        count = argv[4].u;
                    case 4:
                        port = argv[3].u;
                    case 3:
                        host = str_ip_to_int(argv[2].str, strlen(argv[2].str));
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

                lk_time_t t = current_time();
                uint32_t failures = 0;
                for (uint32_t i = 0; i < count; i++) {
                    if (udp_send(buf, BUFSIZE, handle) != 0) {
                        failures++;
                    }
                    buf[128]++;
                }
                t = current_time() - t;
                printf("%d pkts failed\n", failures);
                uint64_t total_count = (uint64_t)count * BUFSIZE;
                printf("wrote %llu bytes in %u msecs (%llu bytes/sec)\n",
                       total_count, (uint32_t)t, total_count * 1000 / t);

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
STATIC_COMMAND_END(minip);
#endif
