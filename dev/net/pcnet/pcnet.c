/*
 * Copyright (c) 2013 Corey Tabaka
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch/x86.h>
#include <assert.h>
#include <dev/bus/pci.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <kernel/thread.h>
#include <lib/minip.h>
#include <lib/minip/netif.h>
#include <lib/pktbuf.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <malloc.h>
#include <pcnet.h>
#include <platform/interrupts.h>
#include <platform/pc.h>
#include <stdlib.h>
#include <string.h>

#define LOCAL_TRACE 0

#define PCNET_INIT_TIMEOUT 20000
#define MAX_PACKET_SIZE    1518

#define QEMU_IRQ_BUG_WORKAROUND 1

struct pcnet_state {
    unsigned unit;

    int irq;
    addr_t base;

    uint8_t padr[6];

    struct init_block_32 *ib;

    struct rd_style3 *rd;
    struct td_style3 *td;

    pktbuf_t **rx_buffers;
    pktbuf_t **tx_buffers;

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

    netif_t netif;
};

static status_t pcnet_init_device(struct pcnet_state *state, pci_location_t loc);
static status_t pcnet_read_pci_config(struct pcnet_state *state, pci_location_t loc);

static enum handler_return pcnet_irq_handler(void *arg);

static int pcnet_thread(void *arg);
static bool pcnet_service_tx(struct pcnet_state *state);
static bool pcnet_service_rx(struct pcnet_state *state);

static status_t pcnet_output(struct pcnet_state *state, pktbuf_t *p);
static int pcnet_send_minip_pkt(void *arg, pktbuf_t *p);

static inline uint32_t pcnet_read_csr(struct pcnet_state *state, uint8_t rap) {
    outpd(state->base + REG_RAP, rap);
    return inpd(state->base + REG_RDP);
}

static inline void pcnet_write_csr(struct pcnet_state *state, uint8_t rap, uint16_t data) {
    outpd(state->base + REG_RAP, rap);
    outpd(state->base + REG_RDP, data);
}

static inline uint32_t pcnet_read_bcr(struct pcnet_state *state, uint8_t rap) {
    outpd(state->base + REG_RAP, rap);
    return inpd(state->base + REG_BDP);
}

static inline void pcnet_write_bcr(struct pcnet_state *state, uint8_t rap, uint16_t data) {
    outpd(state->base + REG_RAP, rap);
    outpd(state->base + REG_BDP, data);
}

static void pcnet_free_buffers(struct pcnet_state *state) {
    if (!state) {
        return;
    }

    if (state->rx_buffers) {
        for (int i = 0; i < state->rd_count; i++) {
            if (state->rx_buffers[i]) {
                pktbuf_free(state->rx_buffers[i], true);
                state->rx_buffers[i] = NULL;
            }
        }
    }

    if (state->tx_buffers) {
        for (int i = 0; i < state->td_count; i++) {
            if (state->tx_buffers[i]) {
                pktbuf_free(state->tx_buffers[i], true);
                state->tx_buffers[i] = NULL;
            }
        }
    }
}

static status_t pcnet_init_device(struct pcnet_state *state, pci_location_t loc) {
    status_t res = NO_ERROR;

    char loc_str[14];
    dprintf(ALWAYS, "pcnet: init start at %s\n", pci_loc_string(loc, loc_str));

    res = pcnet_read_pci_config(state, loc);
    if (res) {
        goto error;
    }

    for (int i = 0; i < 6; i++) {
        state->padr[i] = inp(state->base + i);
    }

    dprintf(ALWAYS, "pcnet: io base %#lx irq %d mac %02x:%02x:%02x:%02x:%02x:%02x\n", state->base,
            state->irq, state->padr[0], state->padr[1], state->padr[2], state->padr[3],
            state->padr[4], state->padr[5]);

    /* put the controller into 32bit wide mode by performing a 32bit write to CSR0 */
    outpd(state->base + 0, 0);

    /* stop the controller for configuration */
    pcnet_write_csr(state, 0, CSR0_STOP);

    /* setup 32bit (style 3) structures, burst, all CSR4 bits valid, TDM1[29] is ADD_FCS */
    pcnet_write_csr(state, 58, 3);

    /* DMA plus enable */
    pcnet_write_csr(state, 4, pcnet_read_csr(state, 4) | CSR4_DMAPLUS);

    /* Allocate a TX ring of 128 and a smaller RX ring so pktbuf pool is not exhausted.
     * pktbuf_alloc consumes two pool objects per RX buffer.
     */
    state->td_count = 128;
    state->rd_count = 64;
    state->td = memalign(16, state->td_count * DESC_SIZE);
    state->rd = memalign(16, state->rd_count * DESC_SIZE);

    state->rx_buffers = (pktbuf_t **)calloc(state->rd_count, sizeof(pktbuf_t *));
    state->tx_buffers = (pktbuf_t **)calloc(state->td_count, sizeof(pktbuf_t *));

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

    /* setup init block */
    state->ib->tlen = 7; // 128 descriptors
    state->ib->rlen = 7; // 128 descriptors
    state->ib->mode = 0;

    state->ib->ladr = ~0;
    state->ib->tdra = (uint32_t)(uintptr_t)state->td;
    state->ib->rdra = (uint32_t)(uintptr_t)state->rd;

    memcpy(state->ib->padr, state->padr, 6);

    /* load the init block address */
    pcnet_write_csr(state, 1, (uint32_t)(uintptr_t)state->ib);
    pcnet_write_csr(state, 2, (uint32_t)((uintptr_t)state->ib >> 16));

    /* setup receive descriptors */
    for (int i = 0; i < state->rd_count; i++) {
        pktbuf_t *p = pktbuf_alloc();
        if (!p) {
            res = ERR_NO_MEMORY;
            goto error;
        }

        pktbuf_reset(p, 0);

        state->rd[i].rbadr = pktbuf_data_phys(p);
        state->rd[i].bcnt = -(int16_t)p->blen;
        state->rd[i].ones = 0xf;
        state->rd[i].own = 1;

        state->rx_buffers[i] = p;
    }

    mutex_init(&state->tx_lock);

    state->done = false;
    event_init(&state->event, false, EVENT_FLAG_AUTOUNSIGNAL);
    event_init(&state->initialized, false, 0);

    /* start up a thread to process packet activity */
    thread_resume(
        thread_create("[pcnet bh]", pcnet_thread, state, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE));

    register_int_handler(state->irq, pcnet_irq_handler, state);
    unmask_interrupt(state->irq);

#if QEMU_IRQ_BUG_WORKAROUND
    register_int_handler(INT_BASE + 15, pcnet_irq_handler, state);
    unmask_interrupt(INT_BASE + 15);
#endif

    /* wait for initialization to complete */
    res = event_wait_timeout(&state->initialized, PCNET_INIT_TIMEOUT);
    if (res) {
        dprintf(ALWAYS, "pcnet: init timed out waiting for IDON (%d)\n", res);
        goto error;
    }

    char if_name[16];
    snprintf(if_name, sizeof(if_name), "pcnet-%u", state->unit);
    if (!netif_create(&state->netif, if_name)) {
        res = ERR_NO_MEMORY;
        goto error;
    }

    res = netif_set_eth(&state->netif, pcnet_send_minip_pkt, state, state->padr);
    if (res != NO_ERROR) {
        goto error;
    }

    res = netif_register(&state->netif);
    if (res != NO_ERROR) {
        goto error;
    }

    dprintf(ALWAYS, "pcnet: initialization complete\n");
    return NO_ERROR;

error:
    dprintf(ALWAYS, "pcnet: init failed (%d)\n", res);
    pcnet_free_buffers(state);
    free(state->td);
    state->td = NULL;
    free(state->rd);
    state->rd = NULL;
    free(state->ib);
    state->ib = NULL;
    free((void *)state->tx_buffers);
    state->tx_buffers = NULL;
    free((void *)state->rx_buffers);
    state->rx_buffers = NULL;
    return res;
}

static status_t pcnet_read_pci_config(struct pcnet_state *state, pci_location_t loc) {
    status_t res = NO_ERROR;
    pci_config_t config;

    pci_read_config(loc, &config);

    for (unsigned i = 0; i < countof(config.type0.base_addresses); i++) {
        if (config.type0.base_addresses[i] & 0x1) {
            state->base = config.type0.base_addresses[i] & ~0x3;
            break;
        }
    }

    if (!state->base) {
        res = ERR_NOT_CONFIGURED;
        goto error;
    }

    if (config.type0.interrupt_line != 0xff) {
        state->irq = config.type0.interrupt_line + INT_BASE;
    } else {
        res = ERR_NOT_CONFIGURED;
        goto error;
    }

    pci_write_config_half(loc, PCI_CONFIG_COMMAND,
                          (config.command | PCI_COMMAND_IO_EN | PCI_COMMAND_BUS_MASTER_EN) &
                              ~PCI_COMMAND_MEM_EN);

error:
    return res;
}

static enum handler_return pcnet_irq_handler(void *arg) {
    struct pcnet_state *state = arg;

    mask_interrupt(state->irq);

#if QEMU_IRQ_BUG_WORKAROUND
    mask_interrupt(INT_BASE + 15);
#endif

    event_signal(&state->event, false);

    return INT_RESCHEDULE;
}

static int pcnet_thread(void *arg) {
    DEBUG_ASSERT(arg);

    struct pcnet_state *state = arg;

    /* kick off init, enable ints, and start operation */
    pcnet_write_csr(state, 0, CSR0_INIT | CSR0_IENA | CSR0_STRT);

    while (!state->done) {
        event_wait(&state->event);

        uint32_t csr0 = pcnet_read_csr(state, 0);

        /* disable interrupts at the controller */
        pcnet_write_csr(state, 0, csr0 & ~CSR0_IENA);

        if (csr0 & CSR0_IDON) {
            /* free the init block that we no longer need */
            free(state->ib);
            state->ib = NULL;

            event_signal(&state->initialized, true);
        }

        if (csr0 & CSR0_ERR) {
            /* clear flags, preserve necessary enables */
            pcnet_write_csr(state, 0, csr0 & (CSR0_TXON | CSR0_RXON | CSR0_IENA));
        }

        bool again = !!(csr0 & (CSR0_RINT | CSR0_TINT));
        while (again) {
            again = pcnet_service_tx(state) || pcnet_service_rx(state);
        }

        /* enable interrupts at the controller */
        pcnet_write_csr(state, 0, CSR0_IENA);
        unmask_interrupt(state->irq);

#if QEMU_IRQ_BUG_WORKAROUND
        unmask_interrupt(INT_BASE + 15);
#endif
    }

    return 0;
}

static bool pcnet_service_tx(struct pcnet_state *state) {
    mutex_acquire(&state->tx_lock);

    struct td_style3 *td = &state->td[state->td_tail];

    if (state->tx_pending && td->own == 0) {
        pktbuf_t *p = state->tx_buffers[state->td_tail];
        DEBUG_ASSERT(p);

        state->tx_buffers[state->td_tail] = NULL;

        state->tx_pending--;
        state->td_tail = (state->td_tail + 1) % state->td_count;

        mutex_release(&state->tx_lock);

        pktbuf_free(p, true);
        return true;
    }

    mutex_release(&state->tx_lock);
    return false;
}

static bool pcnet_service_rx(struct pcnet_state *state) {
    struct rd_style3 *rd = &state->rd[state->rd_head];

    if (rd->own == 0) {
        pktbuf_t *p = state->rx_buffers[state->rd_head];
        DEBUG_ASSERT(p);

        if (!rd->err) {
            if (rd->mcnt <= p->blen) {
                p->dlen = rd->mcnt;

#if LOCAL_TRACE
                LTRACEF("payload=%p len=%u\n", p->data, p->dlen);
                hexdump8(p->data, p->dlen);
#endif

                minip_rx_driver_callback(&state->netif, p);
            } else {
                LTRACEF("RX packet size error: mcnt = %u, buf len = %u\n", rd->mcnt, p->blen);
            }
        }

        pktbuf_reset(p, 0);

        memset(rd, 0, sizeof(*rd));
        rd->rbadr = pktbuf_data_phys(p);
        rd->bcnt = -(int16_t)p->blen;
        rd->ones = 0xf;
        rd->own = 1;

        state->rd_head = (state->rd_head + 1) % state->rd_count;
        return true;
    }

    return false;
}

static status_t pcnet_output(struct pcnet_state *state, pktbuf_t *p) {
    if (!state || !p) {
        return ERR_INVALID_ARGS;
    }

    status_t res = NO_ERROR;

    mutex_acquire(&state->tx_lock);

    struct td_style3 *td = &state->td[state->td_head];

    if (td->own) {
        res = ERR_NOT_READY;
        goto done;
    }

    if (p->dlen > MAX_PACKET_SIZE) {
        res = ERR_TOO_BIG;
        goto done;
    }

#if LOCAL_TRACE
    LTRACEF("Queuing packet: td_head=%d p=%p len=%u\n", state->td_head, p, p->dlen);
    hexdump8(p->data, p->dlen);
#endif

    /* clear flags */
    memset(td, 0, sizeof(*td));

    td->tbadr = pktbuf_data_phys(p);
    td->bcnt = -(int16_t)p->dlen;
    td->stp = 1;
    td->enp = 1;
    td->add_no_fcs = 1;
    td->ones = 0xf;

    state->tx_buffers[state->td_head] = p;
    state->tx_pending++;

    state->td_head = (state->td_head + 1) % state->td_count;

    td->own = 1;

    /* trigger tx */
    pcnet_write_csr(state, 0, CSR0_TDMD);

done:
    mutex_release(&state->tx_lock);
    return res;
}

static int pcnet_send_minip_pkt(void *arg, pktbuf_t *p) {
    struct pcnet_state *state = arg;
    return pcnet_output(state, p);
}

static void pcnet_init_hook(uint level) {
    dprintf(ALWAYS, "pcnet: probing PCI for AMD PCnet devices\n");

    unsigned found = 0;
    for (size_t i = 0;; i++) {
        pci_location_t loc;
        status_t err = pci_bus_mgr_find_device(&loc, 0x2000, 0x1022, i);
        if (err != NO_ERROR) {
            break;
        }

        found++;
        char loc_str[14];
        dprintf(ALWAYS, "pcnet: found candidate %zu at %s\n", i, pci_loc_string(loc, loc_str));

        struct pcnet_state *state = calloc(1, sizeof(struct pcnet_state));
        if (!state) {
            break;
        }

        state->unit = found - 1;

        err = pcnet_init_device(state, loc);
        if (err != NO_ERROR) {
            dprintf(ALWAYS, "pcnet: device init failed (%d)\n", err);
            free(state);
            continue;
        }

        dprintf(ALWAYS, "pcnet: device registered as minip interface '%s'\n", state->netif.name);
    }

    if (found == 0) {
        dprintf(ALWAYS, "pcnet: no matching PCI devices found\n");
    }
}

LK_INIT_HOOK(pcnet, &pcnet_init_hook, LK_INIT_LEVEL_PLATFORM);
