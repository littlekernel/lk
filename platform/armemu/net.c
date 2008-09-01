#if WITH_LWIP
/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */
/*
 * ARMEMU bits
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

#include <malloc.h>
#include <dev/ethernet.h>
#include <err.h>
#include <reg.h>
#include <string.h>
#include <platform/interrupts.h>
#include <platform/armemu/memmap.h>

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include <lwip/stats.h>

#include "netif/etharp.h"

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

struct ethernetif {
  struct eth_addr *ethaddr;
  /* Add whatever per-interface state that is needed here. */
};

static const struct eth_addr ethbroadcast = {{0xff,0xff,0xff,0xff,0xff,0xff}};

/* Forward declarations. */
static void  ethernetif_input(struct netif *netif);
static err_t ethernetif_output(struct netif *netif, struct pbuf *p,
             struct ip_addr *ipaddr);

static void
low_level_init(struct netif *netif)
{
  struct ethernetif *ethernetif = netif->state;
  
  /* set MAC hardware address length */
  netif->hwaddr_len = 6;

  /* set MAC hardware address */
  netif->hwaddr[0] = 0;
  netif->hwaddr[1] = 0x01;
  netif->hwaddr[2] = 0x02;
  netif->hwaddr[3] = 0x03;
  netif->hwaddr[4] = 0x04;
  netif->hwaddr[5] = 0x05;

  /* maximum transfer unit */
  netif->mtu = 1500;
  
  /* broadcast capability */
  netif->flags = NETIF_FLAG_BROADCAST;
 
  /* Do whatever else is needed to initialize interface. */  
}

/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */

static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
  struct ethernetif *ethernetif = netif->state;
  struct pbuf *q;
  int i;
  int j;

#if ETH_PAD_SIZE
  pbuf_header(p, -ETH_PAD_SIZE);			/* drop the padding word */
#endif

  /* XXX maybe just a mutex? */
  enter_critical_section();

  i = 0;
  for(q = p; q != NULL; q = q->next) {
    /* Send the data from the pbuf to the interface, one pbuf at a
       time. The size of the data in each pbuf is kept in the ->len
       variable. */
//	debug_dump_memory_bytes(q->payload, q->len);
	for (j = 0; j < q->len; j++)
		*REG8(NET_OUT_BUF + i + j) = ((unsigned char *)q->payload)[j];
	i += q->len;
  }

  *REG(NET_SEND_LEN) = i;
  *REG(NET_SEND) = 1;

  exit_critical_section();

#if ETH_PAD_SIZE
  pbuf_header(p, ETH_PAD_SIZE);			/* reclaim the padding word */
#endif
  
#if LINK_STATS
  lwip_stats.link.xmit++;
#endif /* LINK_STATS */      

  return ERR_OK;
}

/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */

static struct pbuf *
low_level_input(struct netif *netif)
{
  struct ethernetif *ethernetif = netif->state;
  struct pbuf *p, *q;
  u16_t len;
  int i;
  int head, tail;

  /* get the head and tail pointers from the ethernet interface */
  head = *REG(NET_HEAD);
  tail = *REG(NET_TAIL);

  if (tail == head)
	  return NULL; // false alarm

  /* Obtain the size of the packet and put it into the "len"
     variable. */
  len = *REG(NET_IN_BUF_LEN);

#if ETH_PAD_SIZE
  len += ETH_PAD_SIZE;						/* allow room for Ethernet padding */
#endif

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
  if (p != NULL) {

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE);			/* drop the padding word */
#endif

    /* We iterate over the pbuf chain until we have read the entire
     * packet into the pbuf. */
    int pos = 0;
    for(q = p; q != NULL; q = q->next) {
      /* Read enough bytes to fill this pbuf in the chain. The
       * available data in the pbuf is given by the q->len
       * variable. */
	  for (i=0; i < q->len; i++) {
		 ((unsigned char *)q->payload)[i] = *REG8(NET_IN_BUF + pos);
		 pos++;
	  }
    }

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE);			/* reclaim the padding word */
#endif

#if LINK_STATS
    lwip_stats.link.recv++;
#endif /* LINK_STATS */      
  } else {
#if LINK_STATS
    lwip_stats.link.memerr++;
    lwip_stats.link.drop++;
#endif /* LINK_STATS */      
  }

  /* push the tail pointer up by one, giving the buffer back to the hardware */
  *REG(NET_TAIL) = (tail + 1) % NET_IN_BUF_COUNT;

  return p;  
}

/*
 * ethernetif_output():
 *
 * This function is called by the TCP/IP stack when an IP packet
 * should be sent. It calls the function called low_level_output() to
 * do the actual transmission of the packet.
 *
 */

static err_t
ethernetif_output(struct netif *netif, struct pbuf *p,
      struct ip_addr *ipaddr)
{
//	dprintf("ethernetif_output: netif %p, pbuf %p, ipaddr %p\n", netif, p, ipaddr);
  
 /* resolve hardware address, then send (or queue) packet */
  return etharp_output(netif, ipaddr, p);
 
}

/*
 * ethernetif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */

static void
ethernetif_input(struct netif *netif)
{
  struct ethernetif *ethernetif;
  struct eth_hdr *ethhdr;
  struct pbuf *p;

  ethernetif = netif->state;
  
  /* move received packet into a new pbuf */
  p = low_level_input(netif);
  /* no packet could be read, silently ignore this */
  if (p == NULL) return;
  /* points to packet payload, which starts with an Ethernet header */
  ethhdr = p->payload;

#if LINK_STATS
  lwip_stats.link.recv++;
#endif /* LINK_STATS */

  ethhdr = p->payload;

//  dprintf("ethernetif_input: type 0x%x\n", htons(ethhdr->type));
    
  switch (htons(ethhdr->type)) {
  /* IP packet? */
  case ETHTYPE_IP:
    /* update ARP table */
    etharp_ip_input(netif, p);
    /* skip Ethernet header */
    pbuf_header(p, -sizeof(struct eth_hdr));
    /* pass to network layer */
    netif->input(p, netif);
    break;
      
    case ETHTYPE_ARP:
      /* pass p to ARP module  */
      etharp_arp_input(netif, ethernetif->ethaddr, p);
      break;
    default:
      pbuf_free(p);
      p = NULL;
      break;
  }
}

static enum handler_return ethernet_int(void *arg)
{
	struct netif *netif = (struct netif *)arg;

	ethernetif_input(netif);

	return INT_RESCHEDULE;
}



/*
 * ethernetif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */

static err_t
ethernetif_init(struct netif *netif)
{
  struct ethernetif *ethernetif;
    
  ethernetif = mem_malloc(sizeof(struct ethernetif));
  
  if (ethernetif == NULL)
  {
  	LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
  	return ERR_MEM;
  }
  
  netif->state = ethernetif;
  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  netif->output = ethernetif_output;
  netif->linkoutput = low_level_output;
  
  ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
  
  low_level_init(netif);

  return ERR_OK;
}

status_t ethernet_init(void)
{
	/* check to see if the ethernet feature is turned on */
	if ((*REG(SYSINFO_FEATURES) & SYSINFO_FEATURE_NETWORK) == 0)
		return ERR_NOT_FOUND;

	struct netif *netif = calloc(sizeof(struct netif), 1);
	struct ip_addr *ipaddr = calloc(sizeof(struct ip_addr), 1);
	struct ip_addr *netmask = calloc(sizeof(struct ip_addr), 1);
	struct ip_addr *gw = calloc(sizeof(struct ip_addr), 1);

	struct netif *netifret = netif_add(netif, ipaddr, netmask, gw, NULL, &ethernetif_init, &ip_input);
	if (netifret == NULL) {
		free(netif);
		free(ipaddr);
		free(netmask);
		free(gw);
		return ERR_NOT_FOUND;
	}

	/* register for interrupt handlers */
	register_int_handler(INT_NET, ethernet_int, netif);

	netif_set_default(netif);

	unmask_interrupt(INT_NET, NULL);

	return NO_ERROR;
}

#endif // WITH_LWIP
