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

#if WITH_LIB_CONSOLE
static int cmd_minip(int argc, const cmd_args *argv)
{
    static minip_fd_t fd = {IPV4(192, 168, 1, 1), 65456};

    if (argc == 1) {
minip_usage:
        printf("minip commands\n");
        printf("mi [a]rp                        dump arp table\n");
        printf("mi [c]onfig <ip addr> <port>    set default dest to <ip addr>:<port>\n");
        printf("mi [s]tatus                     print ip status\n");
        printf("mi [t]est <cnt>                 send <cnt> test packets to the configured dest\n");
    } else {
        switch(argv[1].str[0]) {

            case 'a':
                arp_cache_dump();
                break;

            case 'c':
                if (argc < 4)
                    goto minip_usage;

                int i = 0, pos = 0, len = strlen(argv[2].str);
                memset(&fd, 0, sizeof(fd));
                uint8_t ip[4] = { 0, 0, 0, 0 };

                while (pos < len) {
                    char c = argv[2].str[pos];
                    if (c == '.') {
                        i++;
                    } else {
                        ip[i] *= 10;
                        ip[i] += c - '0';
                    }
                    pos++;
                }
                memcpy(&fd.addr, ip, 4);
                fd.port = argv[3].u;

                if (arp_cache_lookup(fd.addr) == NULL) {
                    send_arp_request(fd.addr);
                }

                printf("new config: %u.%u.%u.%u:%u\n",
                       ip[0], ip[1], ip[2], ip[3], fd.port);
                break;

            case 'l': {
                uint32_t buf[256];
                uint32_t wait = 0;
                memset(buf, 0x00, sizeof(buf));
                wait = argv[2].u;

                while (1) {
                    send(&fd, buf, sizeof(buf), 0);
                    if (wait > 0) {
                        thread_sleep(wait);
                    }
                }
            }
            break;
            case 's': {
                uint32_t ipaddr = minip_get_ipaddr();

                printf("hostname: %s\n", minip_get_hostname());
                printf("ip: %u.%u.%u.%u\n", IPV4_SPLIT(ipaddr));
            }
            break;
            case 't': {
                uint32_t buf[256];
                uint32_t c = 1;
                if (argc > 2) {
                    c = argv[2].u;
                }

                memset(buf, 0x00, sizeof(buf));
                printf("sending %u packet(s)\n", c);

                while (c--) {
                    buf[255] = c;
                    send(&fd, buf, sizeof(buf), 0);
                }
            }
            break;
        }
    }

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("mi", "minip commands", &cmd_minip)
STATIC_COMMAND_END(minip);
#endif

// vim: set ts=4 sw=4 expandtab:
