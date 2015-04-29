/*
 * Copyright (c) 2014 Christopher Anderson
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
#include <assert.h>
#include <lib/console.h>
#include <debug.h>
#include <list.h>
#include <err.h>
#include <errno.h>
#include <reg.h>
#include <endian.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <trace.h>
#include <pow2.h>
#include <sys/types.h>
#include <lib/cbuf.h>
#include <kernel/timer.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/gem.h>
#include <platform.h>
#include <kernel/event.h>
#include <kernel/semaphore.h>

#include <lib/pktbuf.h>

#define LOCAL_TRACE         0
#define GEM_RX_BUF_CNT      64
#define GEM_TX_BUF_CNT      32
#define GEM_RX_BUF_SIZE     1536
#define GEM_TX_BUF_SIZE     1536

struct list_node tx_queue;
struct list_node queued_pbufs;
gem_cb_t rx_callback = NULL;

struct gem_desc {
    uint32_t addr;
    uint32_t ctrl;
};

/* Quick overview:
 * RX:
 *  rx_tbl contains rx descriptors. A pktbuf is allocated for each of these and a descriptor
 *  entry in the table points to a buffer in the pktbuf. rx_tbl[X]'s pktbuf is stored in rx_pbufs[X]
 *
 * TX:
 *  The current position to write new tx descriptors to is maintained by tx_pos. As frames are
 *  queued in tx_tbl their pktbufs are stored in the list queued_pbufs. As frame transmission is
 *  completed these pktbufs are released back to the pool by the interrupt handler for TX_COMPLETE
 */
struct gem_state {
    struct gem_desc rx_tbl[GEM_RX_BUF_CNT];
    struct gem_desc tx_tbl[GEM_TX_BUF_CNT];
    pktbuf_t *rx_pbufs[GEM_RX_BUF_CNT];
    unsigned int tx_pos;
};


static event_t rx_pending;
static event_t tx_complete;
static bool debug_rx = false;
static struct gem_state *state;
static paddr_t state_phys;
static volatile struct gem_regs *regs = NULL;

static void debug_rx_handler(pktbuf_t *p)
{
    static uint32_t pkt = 0;

    printf("[%10lu] packet %u, %zu bytes:\n", current_time(), ++pkt, p->dlen);
    hexdump8(p->data, p->dlen);
    putchar('\n');

    if (rx_callback) {
        rx_callback(p);
    }
}

static int free_completed_pbuf_frame(void) {
    pktbuf_t *p;
    bool eof;
    int ret = 0;

    if (!list_is_empty(&queued_pbufs)) {
        do {
            p = list_remove_head_type(&queued_pbufs, pktbuf_t, list);
            eof = p->eof;
            ret += pktbuf_free(p, false);
        } while (!eof);
    }

    return ret;
}

void queue_pkts_in_tx_tbl(void) {
    pktbuf_t *p;
    unsigned int cur_pos;

    enter_critical_section();
    if (list_is_empty(&tx_queue)) {
        goto exit;
    }

    /* Queue packets in the descriptor table until we're either out of space in the table
     * or out of packets in our tx queue. Any packets left will remain in the list and be
     * processed the next time available */
    while ((state->tx_tbl[state->tx_pos].ctrl & TX_DESC_USED) &&
            ((p = list_remove_head_type(&tx_queue, pktbuf_t, list)) != NULL)) {
        cur_pos = state->tx_pos;

        state->tx_tbl[cur_pos].addr = pktbuf_data_phys(p);
        state->tx_tbl[cur_pos].ctrl &= TX_DESC_WRAP; /* Protect the wrap bit the table's end */
        state->tx_tbl[cur_pos].ctrl |= TX_BUF_LEN(p->dlen);

        /* Mark end of frame for if we've reached the end of a pktbuf chain */
        if (p->eof) {
            state->tx_tbl[cur_pos].ctrl |= TX_LAST_BUF;
        }

        state->tx_pos = (state->tx_pos + 1) % GEM_TX_BUF_CNT;
        list_add_tail(&queued_pbufs, &p->list);
    }

    regs->net_ctrl |= NET_CTRL_START_TX;

exit:
    exit_critical_section();
}

int gem_send_raw_pkt(struct pktbuf *p)
{
    status_t ret = NO_ERROR;
    struct list_node pbuf_head = LIST_INITIAL_VALUE(pbuf_head);

    if (!p || !p->dlen) {
        ret = -1;
        goto err;
    }

    enter_critical_section();
    list_add_tail(&tx_queue, &p->list);
    queue_pkts_in_tx_tbl();
    exit_critical_section();

err:
    return ret;
}


enum handler_return gem_int_handler(void *arg) {
    uint32_t intr_status;
    bool resched = false;

    intr_status = regs->intr_status;

    // Received an RX complete
    if (intr_status & INTR_RX_COMPLETE) {
        event_signal(&rx_pending, false);

        regs->rx_status |= INTR_RX_COMPLETE;

        resched = true;
    }

    if (intr_status & INTR_RX_USED_READ) {

        for (int i = 0; i < GEM_RX_BUF_CNT; i++) {
            state->rx_tbl[i].addr &= ~RX_DESC_USED;
        }

        regs->rx_status &= ~RX_STATUS_BUFFER_NOT_AVAIL;
        regs->net_ctrl &= ~NET_CTRL_RX_EN;
        regs->net_ctrl |= NET_CTRL_RX_EN;
        printf("GEM overflow, dumping pending packets\n");
    }

    if (intr_status & INTR_TX_CORRUPT) {
        printf("tx ahb error!\n");
    }

    /* A frame has been completed so we can clean up ownership of its buffers */
    if (intr_status & INTR_TX_COMPLETE) {
        if (free_completed_pbuf_frame() > 0) {
            resched = true;
        }
        regs->tx_status |= TX_STATUS_COMPLETE;
    }

    /* The controller has processed packets until it hit a buffer owned by the driver */
    if (intr_status & INTR_TX_USED_READ) {
        queue_pkts_in_tx_tbl();
        regs->tx_status |= TX_STATUS_USED_READ;
    }

    regs->intr_status = intr_status;
    return (resched) ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

static bool wait_for_phy_idle(void)
{
    int iters = 1000;
    while (iters && !(regs->net_status & NET_STATUS_PHY_MGMT_IDLE)) {
        iters--;
    }

    if (iters == 0) {
        return false;
    }

    return true;
}

static bool gem_phy_init(void) {
    return wait_for_phy_idle();
}

static status_t gem_cfg_buffer_descs(void)
{
    memset(state, 0, sizeof(struct gem_state));

    /* Take pktbufs from the allocated target pool and assign them to the gem RX
     * descriptor table */
    for (unsigned int n = 0; n < GEM_RX_BUF_CNT; n++) {
        pktbuf_t *p = pktbuf_alloc();
        if (!p) {
            return -1;
        }

        state->rx_pbufs[n] = p;
        state->rx_tbl[n].addr = (uintptr_t) p->phys_base;
        state->rx_tbl[n].ctrl = 0;
    }

    /* Claim ownership of TX descriptors for the driver */
    for (unsigned i = 0; i < GEM_TX_BUF_CNT; i++) {
        state->tx_tbl[i].ctrl |= TX_DESC_USED;
    }

    /* Both set of descriptors need wrap bits set at the end of their tables*/
    state->rx_tbl[GEM_RX_BUF_CNT-1].addr |= RX_DESC_WRAP;
    state->tx_tbl[GEM_TX_BUF_CNT-1].ctrl |= TX_DESC_WRAP;

    /* Point the controller at the offset into state's physical location for RX descs */
    regs->rx_qbar = ((uintptr_t)&state->rx_tbl[0] - (uintptr_t)state) + state_phys;
    regs->tx_qbar = ((uintptr_t)&state->tx_tbl[0] - (uintptr_t)state) + state_phys;

    return NO_ERROR;
}

static void gem_cfg_ints(void)
{
    uint32_t gem_base = (uintptr_t)regs;

    if (gem_base == GEM0_BASE) {
        register_int_handler(ETH0_INT, gem_int_handler, NULL);
        unmask_interrupt(ETH0_INT);
    } else if (gem_base == GEM1_BASE) {
        register_int_handler(ETH1_INT, gem_int_handler, NULL);
        unmask_interrupt(ETH1_INT);
    } else {
        printf("Illegal gem periph base address 0x%08X!\n", gem_base);
        return;
    }

    /* Enable all interrupts */
    regs->intr_en = INTR_RX_COMPLETE | INTR_TX_COMPLETE | INTR_HRESP_NOT_OK | INTR_MGMT_SENT |
                    INTR_RX_USED_READ | INTR_TX_CORRUPT | INTR_TX_USED_READ | INTR_RX_OVERRUN;
}

int gem_rx_thread(void *arg)
{
    pktbuf_t *p;
    int bp = 0;

    while (1) {
        event_wait(&rx_pending);

        for (;;) {
            if (state->rx_tbl[bp].addr & RX_DESC_USED) {
                p = state->rx_pbufs[bp];
                p->dlen = RX_BUF_LEN(state->rx_tbl[bp].ctrl);
                p->data = p->buffer + 2;
                if (debug_rx) {
                    debug_rx_handler(p);
                } else if (rx_callback) {
                    rx_callback(p);
                }

                state->rx_tbl[bp].addr &= ~RX_DESC_USED;
                state->rx_tbl[bp].ctrl = 0;
                bp = (bp + 1) % GEM_RX_BUF_CNT;
            } else {
                break;
            }
        }
    }

    return 0;
}

void gem_deinit(uintptr_t base)
{
    /* reset the gem peripheral */
    uint32_t rst_mask;
    if (base == GEM0_BASE) {
        rst_mask = (1<<6) | (1<<4) | (1<<0);
    } else {
        rst_mask = (1<<7) | (1<<5) | (1<<1);
    }
    SLCR->GEM_RST_CTRL |= rst_mask;
    spin(1);
    SLCR->GEM_RST_CTRL &= ~rst_mask;


    /* Clear Network control / status registers */
    regs->net_ctrl |= NET_CTRL_STATCLR;
    regs->rx_status = 0x0F;
    regs->tx_status = 0xFF;
    /* Disable interrupts */
    regs->intr_dis  = 0x7FFFEFF;

    /* Empty out the buffer queues */
    regs->rx_qbar = 0;
    regs->tx_qbar = 0;
}

/* TODO: Fix signature */
status_t gem_init(uintptr_t gem_base)
{
    status_t ret;
    uint32_t reg_val;
    thread_t *rx_thread;
    vaddr_t state_vaddr;
    paddr_t state_paddr;

    DEBUG_ASSERT(gem_base == GEM0_BASE || gem_base == GEM1_BASE);

    /* Data structure init */
    event_init(&tx_complete, false, EVENT_FLAG_AUTOUNSIGNAL);
    event_init(&rx_pending, false, EVENT_FLAG_AUTOUNSIGNAL);
    list_initialize(&queued_pbufs);
    list_initialize(&tx_queue);

    /* allocate a block of contiguous memory for the peripheral state */
    if ((ret = vmm_alloc_contiguous(vmm_get_kernel_aspace(), "gem_desc",
            sizeof(*state), (void **)&state_vaddr, 0, 0, ARCH_MMU_FLAG_UNCACHED)) < 0) {
        return ret;
    }

    if ((ret = arch_mmu_query(state_vaddr, &state_paddr, NULL)) < 0) {
        return ret;
    }

    /* tx/rx descriptor tables and memory mapped registers */
    state = (void *)state_vaddr;
    state_phys = state_paddr;
    regs = (struct gem_regs *) gem_base;

    /* rx background thread */
    rx_thread = thread_create("gem_rx", gem_rx_thread, NULL, HIGH_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(rx_thread);

    /* Bring whatever existing configuration is up down so we can do it cleanly */
    gem_deinit(gem_base);
    gem_cfg_buffer_descs();

    /* Self explanatory configuration for the gige */
    reg_val  = NET_CFG_FULL_DUPLEX;
    reg_val |= NET_CFG_GIGE_EN;
    reg_val |= NET_CFG_SPEED_100;
    reg_val |= NET_CFG_RX_CHKSUM_OFFLD_EN;
    reg_val |= NET_CFG_FCS_REMOVE;
    reg_val |= NET_CFG_MDC_CLK_DIV(0x7);
    reg_val |= NET_CFG_RX_BUF_OFFSET(2);
    regs->net_cfg = reg_val;

    /* Set DMA to 1600 byte rx buffer, 8KB addr space for rx, 4KB addr space for tx,
     * hw checksumming, little endian, and use INCR16 ahb bursts
     */
    reg_val  = DMA_CFG_AHB_MEM_RX_BUF_SIZE(0x19);
    reg_val |= DMA_CFG_RX_PKTBUF_MEMSZ_SEL(0x3);
    reg_val |= DMA_CFG_TX_PKTBUF_MEMSZ_SEL;
    reg_val |= DMA_CFG_CSUM_GEN_OFFLOAD_EN;
    reg_val |= DMA_CFG_AHB_FIXED_BURST_LEN(0x16);
    regs->dma_cfg = reg_val;

    /* Enable VREF from GPIOB */
    SLCR_REG(GPIOB_CTRL) = 0x1;

    ret = gem_phy_init();
    if (!ret) {
        printf("Phy not idle, aborting!\n");
        return ret;
    }

    gem_cfg_ints();

    reg_val  = NET_CTRL_MD_EN;
    reg_val |= NET_CTRL_RX_EN;
    reg_val |= NET_CTRL_TX_EN;
    regs->net_ctrl = reg_val;

    return NO_ERROR;
}

void gem_disable(void)
{
    /* disable all the interrupts */
    regs->intr_en = 0;
    mask_interrupt(ETH0_INT);

    /* stop tx and rx */
    regs->net_ctrl = 0;
}

void gem_set_callback(gem_cb_t rx)
{
    rx_callback = rx;
}

void gem_set_macaddr(uint8_t mac[6]) {
    uint32_t en = regs->net_ctrl &= NET_CTRL_RX_EN | NET_CTRL_TX_EN;

    if (en) {
        regs->net_ctrl &= ~(en);
    }

    /* _top register must be written after _bot register */
    regs->spec_addr1_bot = (mac[3] << 24) | (mac[2] << 16) | (mac[1] << 8) | mac[0];
    regs->spec_addr1_top = (mac[5] << 8) | mac[4];

    if (en) {
        regs->net_ctrl |= en;
    }
}


/* Debug console commands */
static int cmd_gem(int argc, const cmd_args *argv)
{
    static uint32_t frames_rx = 0, frames_tx = 0;

    if (argc == 1) {
        printf("gem [d]ebug:      enable RX debug output\n");
        printf("gem [r]aw <iter> <length>: Send <iter> raw mac packet for testing\n");
        printf("gem [s]tatus:     print driver status\n");
    } else if (argv[1].str[0] == 'r') {
        pktbuf_t *p;
        int iter;
        if (argc < 4) {
            return 0;
        }

        if ((p = pktbuf_alloc()) == NULL) {
            printf("out of buffers\n");
        }

        iter = argv[2].u;
        p->dlen = argv[3].u;
        while (iter--) {
            memset(p->data, iter, 12);
            gem_send_raw_pkt(p);
        }
    } else if (argv[1].str[0] == 's') {
        uint32_t mac_top = regs->spec_addr1_top;
        uint32_t mac_bot = regs->spec_addr1_bot;
        printf("mac addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
            mac_top >> 8, mac_top & 0xFF, mac_bot >> 24, (mac_bot >> 16) & 0xFF,
            (mac_bot >> 8) & 0xFF, mac_bot & 0xFF);
        uint32_t rx_used = 0, tx_used = 0;
        for (int i = 0; i < GEM_RX_BUF_CNT; i++) {
            rx_used += !!(state->rx_tbl[i].addr & RX_DESC_USED);
        }

        for (int i = 0; i < GEM_TX_BUF_CNT; i++) {
            tx_used += !!(state->tx_tbl[i].ctrl & TX_DESC_USED);
        }

        frames_tx += regs->frames_tx;
        frames_rx += regs->frames_rx;
        printf("rx usage: %u/%u, tx usage %u/%u\n",
            rx_used, GEM_RX_BUF_CNT, tx_used, GEM_TX_BUF_CNT);
        printf("frames rx: %u, frames tx: %u\n",
            frames_rx, frames_tx);
    } else if (argv[1].str[0] == 'd') {
        debug_rx = !debug_rx;
    }

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("gem", "ZYNQ GEM commands", &cmd_gem)
STATIC_COMMAND_END(gem);
