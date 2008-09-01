/*
 * Copyright (c) 2006 Travis Geiselbrecht
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
#include <malloc.h>
#include <err.h>
#include <dev/ethernet.h>

#include <lwip/err.h>
#include <lwip/stats.h>
#include <lwip/sys.h>
#include <lwip/ip.h>
#include <lwip/mem.h>
#include <lwip/memp.h>
#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <lwip/dhcp.h>
#include <lwip/tcpip.h>
#include <netif/etharp.h>

static void
arp_timer(void *arg)
{
  etharp_tmr();
  sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);
}

#if WITH_DHCP
static void
dhcp_coarse_timer(void *arg)
{
	dhcp_coarse_tmr();
	sys_timeout(60*1000, dhcp_coarse_timer, NULL);
}

static void
dhcp_fine_timer(void *arg)
{
	dhcp_fine_tmr();
	sys_timeout(500, dhcp_fine_timer, NULL);
}
#endif

int lwip_init(void)
{
	stats_init();
	sys_init();
	mem_init();
	memp_init();
	pbuf_init();

	tcpip_init(NULL, NULL);

	thread_sleep(1000);

	if (ethernet_init() < NO_ERROR) {
		dprintf("lwip_init: error initializing ethernet, aborting...\n");
		return ERROR;
	}

	etharp_init();
	sys_timeout(ARP_TMR_INTERVAL, arp_timer, NULL);

#if WITH_STATIC_IP
	struct ip_addr ipaddr;
	struct ip_addr gwaddr;
	struct ip_addr netmask;

	ipaddr.addr = htonl(IP_ADDR);
	gwaddr.addr = htonl(GW_ADDR);
	netmask.addr = htonl(NETMASK);

	netif_set_ipaddr(netif_default, &ipaddr);
	netif_set_netmask(netif_default, &netmask);
	netif_set_gw(netif_default, &gwaddr);
#endif

	netif_set_up(netif_default);

#if WITH_DHCP
	dprintf("starting dhcp on default netif\n");
	dhcp_start(netif_default);

	/* start some dhcp timers */
	sys_timeout(60*1000, dhcp_coarse_timer, NULL);
	sys_timeout(500, dhcp_fine_timer, NULL);
#endif
	return 0;
}
