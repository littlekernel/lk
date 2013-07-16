#include <dev/class/netif.h>
#include <kernel/event.h>
#include <arch/ops.h>
#include <netif/etharp.h>
#include <lwip/netif.h>
#include <lwip/dhcp.h>
#include <debug.h>
#include <assert.h>
#include <list.h>
#include <err.h>

#define LOCAL_TRACE 0

struct local_netif {
	struct netif netif;
	struct device *dev;
};

static event_t netif_up_event = EVENT_INITIAL_VALUE(netif_up_event, false, 0);
static volatile int netif_up_count = 0;

static err_t local_linkoutput(struct netif *netif, struct pbuf *p)
{
	LTRACE_ENTRY;

	struct local_netif *nif = containerof(netif, struct local_netif, netif);
	DEBUG_ASSERT(nif);

	status_t res = class_netif_output(nif->dev, p);

	LTRACE_EXIT;
	
	switch (res) {
		case NO_ERROR: return ERR_OK;
		case ERR_NO_MEMORY: return ERR_MEM;
		case ERR_TIMED_OUT: return ERR_TIMEOUT;
		default: return ERR_IF;
	}
}

static void local_netif_status(struct netif *netif)
{
	struct local_netif *nif = containerof(netif, struct local_netif, netif);
	DEBUG_ASSERT(nif);

	if (netif->flags & NETIF_FLAG_UP) {
		TRACEF("netif %c%c ip %u.%u.%u.%u netmask %u.%u.%u.%u gw %u.%u.%u.%u\n",
				netif->name[0], netif->name[1],
				ip4_addr1_16(&netif->ip_addr),
				ip4_addr2_16(&netif->ip_addr),
				ip4_addr3_16(&netif->ip_addr),
				ip4_addr4_16(&netif->ip_addr),
				ip4_addr1_16(&netif->netmask),
				ip4_addr2_16(&netif->netmask),
				ip4_addr3_16(&netif->netmask),
				ip4_addr4_16(&netif->netmask),
				ip4_addr1_16(&netif->gw),
				ip4_addr2_16(&netif->gw),
				ip4_addr3_16(&netif->gw),
				ip4_addr4_16(&netif->gw));

		if (atomic_add(&netif_up_count, 1) >= 0)
			event_signal(&netif_up_event, true);
	} else {
		if (atomic_add(&netif_up_count, -1) == 1)
			event_unsignal(&netif_up_event);
	}
}

static err_t local_netif_init(struct netif *netif)
{
	LTRACE_ENTRY;

	struct local_netif *nif = containerof(netif, struct local_netif, netif);
	DEBUG_ASSERT(nif);
	
	netif->linkoutput = local_linkoutput;
	netif->output = etharp_output;

	netif->hwaddr_len = class_netif_get_hwaddr(nif->dev, netif->hwaddr, sizeof(netif->hwaddr));
	netif->mtu = class_netif_get_mtu(nif->dev);

	netif->name[0] = 'e';
	netif->name[1] = 'n';
	netif->num = 0;
	netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

	LTRACE_EXIT;

	return ERR_OK;
}

status_t class_netstack_wait_for_network(lk_time_t timeout)
{
	status_t res;

	LTRACE_ENTRY;

	res = event_wait_timeout(&netif_up_event, timeout);
	LTRACEF("res=%d\n", res);

	LTRACE_EXIT;
	return res;
}

status_t class_netif_add(struct device *dev)
{
	status_t err;
	ip_addr_t ipaddr, netmask, gw;

	struct local_netif *nif = malloc(sizeof(struct local_netif));
	if (!nif)
		return ERR_NO_MEMORY;
	
	nif->dev = dev;

	err = class_netif_set_state(dev, (struct netstack_state *) nif);
	if (err)
		goto done;
	
	IP4_ADDR(&gw, 0, 0, 0, 0);
	IP4_ADDR(&ipaddr, 0, 0, 0, 0);
	IP4_ADDR(&netmask, 255, 255, 255, 255);

	netif_add(&nif->netif, &ipaddr, &netmask, &gw, nif, local_netif_init, ethernet_input);
	netif_set_default(&nif->netif);
	netif_set_status_callback(&nif->netif, local_netif_status);
	dhcp_start(&nif->netif);

	err = NO_ERROR;

done:
	return err;
}

status_t class_netstack_input(struct device *dev, struct netstack_state *state, struct pbuf *p)
{
	LTRACE_ENTRY;

	struct local_netif *nif = (struct local_netif *) state;
	if (!nif)
		return ERR_INVALID_ARGS;

	if (nif->netif.input(p, &nif->netif) != ERR_OK)
		pbuf_free(p);
	
	LTRACE_EXIT;

	return NO_ERROR;
}

