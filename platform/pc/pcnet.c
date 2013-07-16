/*
 * Copyright (c) 2013 Corey Tabaka
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

#include <reg.h>
#include <err.h>
#include <pcnet.h>
#include <debug.h>
#include <assert.h>
#include <arch/x86.h>
#include <platform/pc.h>
#include <platform/pcnet.h>
#include <platform/interrupts.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <kernel/event.h>
#include <dev/class/netif.h>
#include <dev/pci.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <lwip/pbuf.h>

#define LOCAL_TRACE 0

#define PCNET_INIT_TIMEOUT 20000
#define MAX_PACKET_SIZE 1518

#define QEMU_IRQ_BUG_WORKAROUND 1

struct pcnet_state {
	int irq;
	addr_t base;
	
	uint8_t padr[6];

	struct init_block_32 *ib;

	struct rd_style3 *rd;
	struct td_style3 *td;

	struct pbuf **rx_buffers;
	struct pbuf **tx_buffers;

	/* queue accounting */
	int rd_head;
	int td_head;
	int td_tail;

	int rd_count;
	int td_count;

	int tx_pending;

	mutex_t tx_lock;

	/* bottom half state */
	event_t event;
	event_t initialized;
	bool done;

	struct netstack_state *netstack_state;
};

static status_t pcnet_init(struct device *dev);
static status_t pcnet_read_pci_config(struct device *dev, pci_location_t *loc);

static enum handler_return pcnet_irq_handler(void *arg);

static int pcnet_thread(void *arg);
static bool pcnet_service_tx(struct device *dev);
static bool pcnet_service_rx(struct device *dev);

static status_t pcnet_set_state(struct device *dev, struct netstack_state *state);
static ssize_t pcnet_get_hwaddr(struct device *dev, void *buf, size_t max_len);
static ssize_t pcnet_get_mtu(struct device *dev);

static status_t pcnet_output(struct device *dev, struct pbuf *p);

static struct netif_ops pcnet_ops = {
	.std = {
		.init = pcnet_init,
	},

	.set_state = pcnet_set_state,
	.get_hwaddr = pcnet_get_hwaddr,
	.get_mtu = pcnet_get_mtu,

	.output = pcnet_output,
};

DRIVER_EXPORT(netif, &pcnet_ops.std);

static inline uint32_t pcnet_read_csr(struct device *dev, uint8_t rap)
{
	struct pcnet_state *state = dev->state;

	outpd(state->base + REG_RAP, rap);
	return inpd(state->base + REG_RDP);
}

static inline void pcnet_write_csr(struct device *dev, uint8_t rap, uint16_t data)
{
	struct pcnet_state *state = dev->state;

	outpd(state->base + REG_RAP, rap);
	outpd(state->base + REG_RDP, data);
}

static inline uint32_t pcnet_read_bcr(struct device *dev, uint8_t rap)
{
	struct pcnet_state *state = dev->state;

	outpd(state->base + REG_RAP, rap);
	return inpd(state->base + REG_BDP);
}

static inline void pcnet_write_bcr(struct device *dev, uint8_t rap, uint16_t data)
{
	struct pcnet_state *state = dev->state;

	outpd(state->base + REG_RAP, rap);
	outpd(state->base + REG_BDP, data);
}

static status_t pcnet_init(struct device *dev)
{
	status_t res = NO_ERROR;
	pci_location_t loc;
	int i;

	const struct platform_pcnet_config *config = dev->config;

	if (!config)
		return ERR_NOT_CONFIGURED;
	
	if (pci_find_pci_device(&loc, config->device_id, config->vendor_id, config->index) != _PCI_SUCCESSFUL)
		return ERR_NOT_FOUND;
	
	struct pcnet_state *state = calloc(1, sizeof(struct pcnet_state));
	if (!state)
		return ERR_NO_MEMORY;
	
	dev->state = state;

	res = pcnet_read_pci_config(dev, &loc);
	if (res)
		goto error;
	
	for (i=0; i < 6; i++)
		state->padr[i] = inp(state->base + i);
	
	LTRACEF("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", state->padr[0], state->padr[1], state->padr[2],
			state->padr[3], state->padr[4], state->padr[5]);

	/* put the controller into 32bit wide mode by performing a 32bit write to CSR0 */
	outpd(state->base + 0, 0);

	/* stop the controller for configuration */
	pcnet_write_csr(dev, 0, CSR0_STOP);

	/* setup 32bit (style 3) structures, burst, all CSR4 bits valid, TDM1[29] is ADD_FCS */
	pcnet_write_csr(dev, 58, 3);

	/* DMA plus enable */
	pcnet_write_csr(dev, 4, pcnet_read_csr(dev, 4) | CSR4_DMAPLUS);

	/* allocate 128 tx and 128 rx descriptor rings */
	state->td_count = 128;
	state->rd_count = 128;
	state->td = memalign(16, state->td_count * DESC_SIZE);
	state->rd = memalign(16, state->rd_count * DESC_SIZE);

	state->rx_buffers = calloc(state->rd_count, sizeof(struct pbuf *));
	state->tx_buffers = calloc(state->td_count, sizeof(struct pbuf *));

	state->tx_pending = 0;

	if (!state->td || !state->rd || !state->tx_buffers || !state->rx_buffers) {
		res = ERR_NO_MEMORY;
		goto error;
	}

	memset(state->td, 0, state->td_count * DESC_SIZE);
	memset(state->rd, 0, state->rd_count * DESC_SIZE);

	/* allocate temporary init block space */
	state->ib = memalign(4, sizeof(struct init_block_32));
	if (!state->ib) {
		res = ERR_NO_MEMORY;
		goto error;
	}

	LTRACEF("Init block addr: %p\n", state->ib);

	/* setup init block */
	state->ib->tlen = 7; // 128 descriptors
	state->ib->rlen = 7; // 128 descriptors
	state->ib->mode = 0;
	
	state->ib->ladr = ~0;
	state->ib->tdra = (uint32_t) state->td;
	state->ib->rdra = (uint32_t) state->rd;

	memcpy(state->ib->padr, state->padr, 6);

	/* load the init block address */
	pcnet_write_csr(dev, 1, (uint32_t) state->ib);
	pcnet_write_csr(dev, 2, (uint32_t) state->ib >> 16);

	/* setup receive descriptors */
	for (i=0; i < state->rd_count; i++) {
		//LTRACEF("Allocating pbuf %d\n", i);
		struct pbuf *p = pbuf_alloc(PBUF_RAW, MAX_PACKET_SIZE, PBUF_RAM);

		state->rd[i].rbadr = (uint32_t) p->payload;
		state->rd[i].bcnt = -p->tot_len;
		state->rd[i].ones = 0xf;
		state->rd[i].own = 1;

		state->rx_buffers[i] = p;
	}

	mutex_init(&state->tx_lock);

	state->done = false;
	event_init(&state->event, false, EVENT_FLAG_AUTOUNSIGNAL);
	event_init(&state->initialized, false, 0);

	/* start up a thread to process packet activity */
	thread_resume(thread_create("[pcnet bh]", pcnet_thread, dev, DEFAULT_PRIORITY,
				DEFAULT_STACK_SIZE));

	register_int_handler(state->irq, pcnet_irq_handler, dev);
	unmask_interrupt(state->irq);

#if QEMU_IRQ_BUG_WORKAROUND
	register_int_handler(INT_BASE + 15, pcnet_irq_handler, dev);
	unmask_interrupt(INT_BASE + 15);
#endif

	/* wait for initialization to complete */
	res = event_wait_timeout(&state->initialized, PCNET_INIT_TIMEOUT);
	if (res) {
		/* TODO: cancel bottom half thread and tear down device instance */
		LTRACEF("Failed to wait for IDON: %d\n", res);
		return res;
	}

	LTRACE_EXIT;
	return res;

error:
	LTRACEF("Error: %d\n", res);

	if (state) {
		free(state->td);
		free(state->rd);
		free(state->ib);
		free(state->tx_buffers);
		free(state->rx_buffers);
	}

	free(state);

	return res;
}

static status_t pcnet_read_pci_config(struct device *dev, pci_location_t *loc)
{
	status_t res = NO_ERROR;
	pci_config_t config;
	uint8_t *buf = (uint8_t *) &config;
	unsigned i;

	DEBUG_ASSERT(dev->state);

	struct pcnet_state *state = dev->state;

	for (i=0; i < sizeof(config); i++)
		pci_read_config_byte(loc, i, buf + i);
	
	LTRACEF("Resources:\n");

	for (i=0; i < countof(config.base_addresses); i++) {
		if (config.base_addresses[i] & 0x1) {
			LTRACEF("  BAR %d  I/O REG: %04x\n", i, config.base_addresses[i] & ~0x3);
			
			state->base = config.base_addresses[i] & ~0x3;
			break;
		}
	}

	if (!state->base) {
		res = ERR_NOT_CONFIGURED;
		goto error;
	}

	if (config.interrupt_line != 0xff) {
		LTRACEF("  IRQ %u\n", config.interrupt_line);

		state->irq = config.interrupt_line + INT_BASE;
	} else {
		res = ERR_NOT_CONFIGURED;
		goto error;
	}

	LTRACEF("Command: %04x\n", config.command);
	LTRACEF("Status:  %04x\n", config.status);

	pci_write_config_half(loc, PCI_CONFIG_COMMAND,
			(config.command | PCI_COMMAND_IO_EN | PCI_COMMAND_BUS_MASTER_EN) & ~PCI_COMMAND_MEM_EN);

error:
	return res;
}

static enum handler_return pcnet_irq_handler(void *arg)
{
	struct device *dev = arg;
	struct pcnet_state *state = dev->state;

	mask_interrupt(state->irq);

#if QEMU_IRQ_BUG_WORKAROUND
	mask_interrupt(INT_BASE + 15);
#endif

	event_signal(&state->event, false);

	return INT_RESCHEDULE;
}

static int pcnet_thread(void *arg)
{
	DEBUG_ASSERT(arg);

	struct device *dev = arg;
	struct pcnet_state *state = dev->state;

	/* kick off init, enable ints, and start operation */
	pcnet_write_csr(dev, 0, CSR0_INIT | CSR0_IENA | CSR0_STRT);

	while (!state->done) {
		LTRACEF("Waiting for event.\n");
		//event_wait_timeout(&state->event, 5000);
		event_wait(&state->event);

		int csr0 = pcnet_read_csr(dev, 0);

		/* disable interrupts at the controller */
		pcnet_write_csr(dev, 0, csr0 & ~CSR0_IENA);

		LTRACEF("CSR0 = %04x\n", csr0);

#if LOCAL_TRACE
		if (csr0 & CSR0_RINT) TRACEF("RINT\n");
		if (csr0 & CSR0_TINT) TRACEF("TINT\n");
#endif

		if (csr0 & CSR0_IDON) {
			LTRACEF("IDON\n");

			/* free the init block that we no longer need */
			free(state->ib);
			state->ib = NULL;

			event_signal(&state->initialized, true);
		}

		if (csr0 & CSR0_ERR) {
			LTRACEF("ERR\n");

			/* TODO: handle errors, though not many need it */

			/* clear flags, preserve necessary enables */
			pcnet_write_csr(dev, 0, csr0 & (CSR0_TXON | CSR0_RXON | CSR0_IENA));
		}

		bool again = !!(csr0 & (CSR0_RINT | CSR0_TINT));
		while (again) {
			again = pcnet_service_tx(dev) | pcnet_service_rx(dev);
		}

		/* enable interrupts at the controller */
		pcnet_write_csr(dev, 0, CSR0_IENA);
		unmask_interrupt(state->irq);

#if QEMU_IRQ_BUG_WORKAROUND
		unmask_interrupt(INT_BASE + 15);
#endif
	}

	return 0;
}

static bool pcnet_service_tx(struct device *dev)
{
	LTRACE_ENTRY;

	struct pcnet_state *state = dev->state;

	mutex_acquire(&state->tx_lock);

	struct td_style3 *td = &state->td[state->td_tail];

	if (state->tx_pending && td->own == 0) {
		struct pbuf *p = state->tx_buffers[state->td_tail];
		DEBUG_ASSERT(p);

		state->tx_buffers[state->td_tail] = NULL;

		LTRACEF("Retiring packet: td_tail=%d p=%p tot_len=%u\n", state->td_tail, p, p->tot_len);

		state->tx_pending--;
		state->td_tail = (state->td_tail + 1) % state->td_count;

		if (td->err) {
			LTRACEF("Descriptor error status encountered\n");
			hexdump8(td, sizeof(*td));
		}
	
		mutex_release(&state->tx_lock);

		pbuf_free(p);

		LTRACE_EXIT;
		return true;
	} else {
		mutex_release(&state->tx_lock);

#if 0
		LTRACEF("Nothing to do for TX.\n");
		for (int i=0; i < state->td_count; i++)
			printf("%d ", state->td[i].own);
		printf("\n");
#endif

		LTRACE_EXIT;
		return false;
	}
}

static bool pcnet_service_rx(struct device *dev)
{
	LTRACE_ENTRY;

	struct pcnet_state *state = dev->state;

	struct rd_style3 *rd = &state->rd[state->rd_head];

	if (rd->own == 0) {
		struct pbuf *p = state->rx_buffers[state->rd_head];
		DEBUG_ASSERT(p);
		
		LTRACEF("Processing RX descriptor %d\n", state->rd_head);

		if (rd->err) {
			LTRACEF("Descriptor error status encountered\n");
			hexdump8(rd, sizeof(*rd));
		} else {
			if (rd->mcnt <= p->tot_len) {

				pbuf_realloc(p, rd->mcnt);

#if LOCAL_TRACE
				LTRACEF("payload=%p len=%u\n", p->payload, p->tot_len);
				hexdump8(p->payload, p->tot_len);
#endif
				
				class_netstack_input(dev, state->netstack_state, p);

				p = state->rx_buffers[state->rd_head] = pbuf_alloc(PBUF_RAW, MAX_PACKET_SIZE, PBUF_RAM);
			} else {
				LTRACEF("RX packet size error: mcnt = %u, buf len = %u\n", rd->mcnt, p->tot_len);
			}
		}

		memset(rd, 0, sizeof(*rd));
		memset(p->payload, 0, p->tot_len);

		rd->rbadr = (uint32_t) p->payload;
		rd->bcnt = -p->tot_len;
		rd->ones = 0xf;
		rd->own = 1;

		state->rd_head = (state->rd_head + 1) % state->rd_count;
		
		LTRACE_EXIT;
		return true;
	} else {
#if 0
		LTRACEF("Nothing to do for RX: rd_head=%d.\n", state->rd_head);
		for (int i=0; i < state->rd_count; i++)
			printf("%d ", state->rd[i].own);
		printf("\n");
#endif
	}

	LTRACE_EXIT;
	return false;
}

static status_t pcnet_set_state(struct device *dev, struct netstack_state *netstack_state)
{
	if (!dev)
		return ERR_INVALID_ARGS;
	
	if (!dev->state)
		return ERR_NOT_CONFIGURED;
	
	struct pcnet_state *state = dev->state;

	state->netstack_state = netstack_state;

	return NO_ERROR;
}

static ssize_t pcnet_get_hwaddr(struct device *dev, void *buf, size_t max_len)
{
	if (!dev || !buf)
		return ERR_INVALID_ARGS;
	
	if (!dev->state)
		return ERR_NOT_CONFIGURED;
	
	struct pcnet_state *state = dev->state;

	memcpy(buf, state->padr, MIN(sizeof(state->padr), max_len));

	return sizeof(state->padr);
}

static ssize_t pcnet_get_mtu(struct device *dev)
{
	if (!dev)
		return ERR_INVALID_ARGS;
	
	return 1500;
}

static status_t pcnet_output(struct device *dev, struct pbuf *p)
{
	LTRACE_ENTRY;

	if (!dev || !p)
		return ERR_INVALID_ARGS;
	
	if (!dev->state)
		return ERR_NOT_CONFIGURED;
	
	status_t res = NO_ERROR;
	struct pcnet_state *state = dev->state;

	mutex_acquire(&state->tx_lock);

	struct td_style3 *td = &state->td[state->td_head];

	if (td->own) {
		LTRACEF("TX descriptor ring full\n");
		res = ERR_NOT_READY; // maybe this should be ERR_NOT_ENOUGH_BUFFER?
		goto done;
	}

	pbuf_ref(p);
	p = pbuf_coalesce(p, PBUF_RAW);

#if LOCAL_TRACE
	LTRACEF("Queuing packet: td_head=%d p=%p tot_len=%u\n", state->td_head, p, p->tot_len);
	hexdump8(p->payload, p->tot_len);
#endif

	/* clear flags */
	memset(td, 0, sizeof(*td));

	td->tbadr = (uint32_t) p->payload;
	td->bcnt = -p->tot_len;
	td->stp = 1;
	td->enp = 1;
	td->add_no_fcs = 1;
	td->ones = 0xf;

	state->tx_buffers[state->td_head] = p;
	state->tx_pending++;

	state->td_head = (state->td_head + 1) % state->td_count;

	td->own = 1;

	/* trigger tx */
	pcnet_write_csr(dev, 0, CSR0_TDMD);

done:
	mutex_release(&state->tx_lock);
	LTRACE_EXIT;
	return res;
}

