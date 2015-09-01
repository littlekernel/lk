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
#include <bits.h>
#include <pow2.h>
#include <sys/types.h>
#include <lib/cbuf.h>
#include <kernel/timer.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <kernel/spinlock.h>
#include <kernel/debug.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/gem.h>
#include <platform.h>
#include <kernel/event.h>
#include <kernel/semaphore.h>

#include <lib/pktbuf.h>
#include <lib/pool.h>

#define LOCAL_TRACE         0

/* Allow targets to override these values */
#ifndef GEM_RX_DESC_CNT
#define GEM_RX_DESC_CNT     32
#endif

#ifndef GEM_TX_DESC_CNT
#define GEM_TX_DESC_CNT      32
#endif

#ifndef GEM_RX_BUF_SIZE
#define GEM_RX_BUF_SIZE     1536
#endif

#ifndef GEM_TX_BUF_SIZE
#define GEM_TX_BUF_SIZE     1536
#endif

pool_t rx_buf_pool;
static spin_lock_t lock = SPIN_LOCK_INITIAL_VALUE;

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
 *  The current position to write new tx descriptors to is maintained by gem.tx_head. As frames are
 *  queued in tx_tbl their pktbufs are stored in the list queued_pbufs. As frame transmission is
 *  completed these pktbufs are released back to the pool by the interrupt handler for TX_COMPLETE
 */
struct gem_descs {
    struct gem_desc rx_tbl[GEM_RX_DESC_CNT];
    struct gem_desc tx_tbl[GEM_TX_DESC_CNT];
};

struct gem_state {
    volatile struct gem_regs *regs;

    struct gem_descs *descs;
    paddr_t descs_phys;

    unsigned int tx_head;
    unsigned int tx_tail;
    unsigned int tx_count;
    struct list_node tx_queue;
    struct list_node queued_pbufs;

    gem_cb_t rx_callback;
    event_t rx_pending;
    event_t tx_complete;
    bool debug_rx;
    pktbuf_t *rx_pbufs[GEM_RX_DESC_CNT];
};

struct gem_state gem;

static void debug_rx_handler(pktbuf_t *p)
{
    static uint32_t pkt = 0;

    printf("[%10u] packet %u, %zu bytes:\n", (uint32_t)current_time(), ++pkt, p->dlen);
    hexdump8(p->data, p->dlen);
    putchar('\n');
}

static int free_completed_pbuf_frames(void) {
    int ret = 0;

    gem.regs->tx_status = gem.regs->tx_status;

    while (gem.tx_count > 0 &&
            (gem.descs->tx_tbl[gem.tx_tail].ctrl & TX_DESC_USED)) {

        bool eof;
        do {
            pktbuf_t *p = list_remove_head_type(&gem.queued_pbufs, pktbuf_t, list);
            DEBUG_ASSERT(p);
            eof = p->flags & PKTBUF_FLAG_EOF;
            ret += pktbuf_free(p, false);
        } while (!eof);

        gem.tx_tail = (gem.tx_tail + 1) % GEM_TX_DESC_CNT;
        gem.tx_count--;
    }

    return ret;
}

void queue_pkts_in_tx_tbl(void) {
    pktbuf_t *p;
    unsigned int cur_pos;

    if (list_is_empty(&gem.tx_queue)) {
        return;
    }

    // XXX handle multi part buffers

    /* Queue packets in the descriptor table until we're either out of space in the table
     * or out of packets in our tx queue. Any packets left will remain in the list and be
     * processed the next time available */
    while (gem.tx_count < GEM_TX_DESC_CNT &&
            ((p = list_remove_head_type(&gem.tx_queue, pktbuf_t, list)) != NULL)) {
        cur_pos = gem.tx_head;

        uint32_t addr = pktbuf_data_phys(p);
        uint32_t ctrl = gem.descs->tx_tbl[cur_pos].ctrl & TX_DESC_WRAP; /* protect the wrap bit */
        ctrl |= TX_BUF_LEN(p->dlen);

        DEBUG_ASSERT(p->flags & PKTBUF_FLAG_EOF); // a multi part buffer would have caused a race condition w/hardware
        if (p->flags & PKTBUF_FLAG_EOF) {
            ctrl |= TX_LAST_BUF;
        }

        /* fill in the descriptor, control word last (in case hardware is racing us) */
        gem.descs->tx_tbl[cur_pos].addr = addr;
        gem.descs->tx_tbl[cur_pos].ctrl = ctrl;

        gem.tx_head = (gem.tx_head + 1) % GEM_TX_DESC_CNT;
        gem.tx_count++;
        list_add_tail(&gem.queued_pbufs, &p->list);
    }

    DMB;
    gem.regs->net_ctrl |= NET_CTRL_START_TX;
}

int gem_send_raw_pkt(struct pktbuf *p)
{
    status_t ret = NO_ERROR;

    if (!p || !p->dlen) {
        ret = -1;
        goto err;
    }

    /* make sure the output buffer is fully written to memory before
     * placing on the outgoing list. */

    // XXX handle multi part buffers
    arch_clean_cache_range((vaddr_t)p->data, p->dlen);

    spin_lock_saved_state_t irqstate;
    spin_lock_irqsave(&lock, irqstate);
    list_add_tail(&gem.tx_queue, &p->list);
    queue_pkts_in_tx_tbl();
    spin_unlock_irqrestore(&lock, irqstate);

err:
    return ret;
}


enum handler_return gem_int_handler(void *arg) {
    uint32_t intr_status;
    bool resched = false;

    intr_status = gem.regs->intr_status;

    spin_lock(&lock);

    while (intr_status) {
        // clear any pending status
        gem.regs->intr_status = intr_status;

        // Received an RX complete
        if (intr_status & INTR_RX_COMPLETE) {
            event_signal(&gem.rx_pending, false);

            gem.regs->rx_status |= INTR_RX_COMPLETE;

            resched = true;
        }

        if (intr_status & INTR_RX_USED_READ) {

            for (int i = 0; i < GEM_RX_DESC_CNT; i++) {
                gem.descs->rx_tbl[i].addr &= ~RX_DESC_USED;
            }

            gem.regs->rx_status &= ~RX_STATUS_BUFFER_NOT_AVAIL;
            gem.regs->net_ctrl &= ~NET_CTRL_RX_EN;
            gem.regs->net_ctrl |= NET_CTRL_RX_EN;
            printf("GEM overflow, dumping pending packets\n");
        }

        if (intr_status & INTR_TX_CORRUPT) {
            printf("tx ahb error!\n");
            if (free_completed_pbuf_frames() > 0) {
                resched = true;
            }
        }

        /* A frame has been completed so we can clean up ownership of its buffers */
        if (intr_status & INTR_TX_COMPLETE) {
            if (free_completed_pbuf_frames() > 0) {
                resched = true;
            }
        }

        /* The controller has processed packets until it hit a buffer owned by the driver */
        if (intr_status & INTR_TX_USED_READ) {
            queue_pkts_in_tx_tbl();
            gem.regs->tx_status |= TX_STATUS_USED_READ;
        }

        /* see if we have any more */
        intr_status = gem.regs->intr_status;
    }

    spin_unlock(&lock);

    return (resched) ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

static bool wait_for_phy_idle(void)
{
    int iters = 1000;
    while (iters && !(gem.regs->net_status & NET_STATUS_PHY_MGMT_IDLE)) {
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
    void *rx_buf_vaddr;
    status_t ret;


    if ((ret = vmm_alloc_contiguous(vmm_get_kernel_aspace(), "gem_rx_bufs",
            GEM_RX_DESC_CNT * GEM_RX_BUF_SIZE,  (void **) &rx_buf_vaddr, 0, 0,
            ARCH_MMU_FLAG_CACHED)) < 0) {
        return ret;
    }

    /* Take pktbufs from the allocated target pool and assign them to the gem RX
     * descriptor table */
    pool_init(&rx_buf_pool, GEM_RX_BUF_SIZE, CACHE_LINE, GEM_RX_DESC_CNT, rx_buf_vaddr);
    for (unsigned int n = 0; n < GEM_RX_DESC_CNT; n++) {
        void *b = pool_alloc(&rx_buf_pool);
        pktbuf_t *p = pktbuf_alloc_empty();
        if (!p || !b) {
            return -1;
        }

        pktbuf_add_buffer(p, b, GEM_RX_BUF_SIZE, 0, PKTBUF_FLAG_CACHED, NULL, NULL);
        gem.rx_pbufs[n] = p;
        gem.descs->rx_tbl[n].addr = (uintptr_t) p->phys_base;
        gem.descs->rx_tbl[n].ctrl = 0;
    }

    /* Claim ownership of TX descriptors for the driver */
    for (unsigned i = 0; i < GEM_TX_DESC_CNT; i++) {
        gem.descs->tx_tbl[i].addr = 0;
        gem.descs->tx_tbl[i].ctrl = TX_DESC_USED;
    }

    /* Both set of descriptors need wrap bits set at the end of their tables*/
    gem.descs->rx_tbl[GEM_RX_DESC_CNT-1].addr |= RX_DESC_WRAP;
    gem.descs->tx_tbl[GEM_TX_DESC_CNT-1].ctrl |= TX_DESC_WRAP;

    /* Point the controller at the offset into state's physical location for RX descs */
    gem.regs->rx_qbar = ((uintptr_t)&gem.descs->rx_tbl[0] - (uintptr_t)gem.descs) + gem.descs_phys;
    gem.regs->tx_qbar = ((uintptr_t)&gem.descs->tx_tbl[0] - (uintptr_t)gem.descs) + gem.descs_phys;

    return NO_ERROR;
}

static void gem_cfg_ints(void)
{
    uint32_t gem_base = (uintptr_t)gem.regs;

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
    gem.regs->intr_en = INTR_RX_COMPLETE | INTR_TX_COMPLETE | INTR_HRESP_NOT_OK | INTR_MGMT_SENT |
                    INTR_RX_USED_READ | INTR_TX_CORRUPT | INTR_TX_USED_READ | INTR_RX_OVERRUN;
}

int gem_rx_thread(void *arg)
{
    pktbuf_t *p;
    int bp = 0;

    while (1) {
        event_wait(&gem.rx_pending);

        for (;;) {
            if (gem.descs->rx_tbl[bp].addr & RX_DESC_USED) {
                uint32_t ctrl = gem.descs->rx_tbl[bp].ctrl;

                p = gem.rx_pbufs[bp];
                p->dlen = RX_BUF_LEN(ctrl);
                p->data = p->buffer + 2;

                /* copy the checksum offloading bits */
                p->flags = 0;
                p->flags |= (BITS_SHIFT(ctrl, 23, 22) != 0) ? PKTBUF_FLAG_CKSUM_IP_GOOD : 0;
                p->flags |= (BITS_SHIFT(ctrl, 23, 22) == 1) ? PKTBUF_FLAG_CKSUM_UDP_GOOD : 0;
                p->flags |= (BITS_SHIFT(ctrl, 23, 22) == 2) ? PKTBUF_FLAG_CKSUM_TCP_GOOD : 0;

                /* invalidate any stale cache lines on the receive buffer to ensure
                 * the cpu has a fresh copy of incomding data. */
                arch_invalidate_cache_range((vaddr_t)p->data, p->dlen);

                if (unlikely(gem.debug_rx)) {
                    debug_rx_handler(p);
                }

                if (likely(gem.rx_callback)) {
                    gem.rx_callback(p);
                }

                /* make sure all dirty data is flushed out of the buffer before
                 * putting into the receive queue */
                arch_clean_invalidate_cache_range((vaddr_t)p->buffer, PKTBUF_SIZE);

                gem.descs->rx_tbl[bp].addr &= ~RX_DESC_USED;
                gem.descs->rx_tbl[bp].ctrl = 0;
                bp = (bp + 1) % GEM_RX_DESC_CNT;
            } else {
                break;
            }
        }
    }

    return 0;
}


int gem_stat_thread(void *arg) {
    volatile bool *run = ((bool *)arg);
    static uint32_t frames_rx = 0, frames_tx = 0;

    while (*run) {
        frames_tx += gem.regs->frames_tx;
        frames_rx += gem.regs->frames_rx;
        printf("GEM tx_head %u, tx_tail %u, tx_count %u, tx_frames %u, rx_frames %u\n",
                gem.tx_head, gem.tx_tail, gem.tx_count, frames_tx, frames_rx);
        thread_sleep(1000);
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
    gem.regs->net_ctrl |= NET_CTRL_STATCLR;
    gem.regs->rx_status = 0x0F;
    gem.regs->tx_status = 0xFF;
    /* Disable interrupts */
    gem.regs->intr_dis  = 0x7FFFEFF;

    /* Empty out the buffer queues */
    gem.regs->rx_qbar = 0;
    gem.regs->tx_qbar = 0;
}

status_t gem_init(uintptr_t gem_base)
{
    status_t ret;
    uint32_t reg_val;
    thread_t *rx_thread;
    void *descs_vaddr;
    paddr_t descs_paddr;

    DEBUG_ASSERT(gem_base == GEM0_BASE || gem_base == GEM1_BASE);

    /* Data structure init */
    event_init(&gem.tx_complete, false, EVENT_FLAG_AUTOUNSIGNAL);
    event_init(&gem.rx_pending, false, EVENT_FLAG_AUTOUNSIGNAL);
    list_initialize(&gem.queued_pbufs);
    list_initialize(&gem.tx_queue);

    /* allocate a block of uncached contiguous memory for the peripheral descriptors */
    if ((ret = vmm_alloc_contiguous(vmm_get_kernel_aspace(), "gem_desc",
            sizeof(*gem.descs), &descs_vaddr, 0, 0, ARCH_MMU_FLAG_UNCACHED_DEVICE)) < 0) {
        return ret;
    }
    descs_paddr = kvaddr_to_paddr((void *)descs_vaddr);

    /* tx/rx descriptor tables and memory mapped registers */
    gem.descs = (void *)descs_vaddr;
    gem.descs_phys = descs_paddr;
    gem.regs = (struct gem_regs *)gem_base;

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
    gem.regs->net_cfg = reg_val;

    /* Set DMA to 1600 byte rx buffer, 8KB addr space for rx, 4KB addr space for tx,
     * hw checksumming, little endian, and use INCR16 ahb bursts
     */
    reg_val  = DMA_CFG_AHB_MEM_RX_BUF_SIZE(0x19);
    reg_val |= DMA_CFG_RX_PKTBUF_MEMSZ_SEL(0x3);
    reg_val |= DMA_CFG_TX_PKTBUF_MEMSZ_SEL;
    reg_val |= DMA_CFG_CSUM_GEN_OFFLOAD_EN;
    reg_val |= DMA_CFG_AHB_FIXED_BURST_LEN(0x10);
    gem.regs->dma_cfg = reg_val;

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
    gem.regs->net_ctrl = reg_val;

    return NO_ERROR;
}

void gem_disable(void)
{
    /* disable all the interrupts */
    gem.regs->intr_en = 0;
    mask_interrupt(ETH0_INT);

    /* stop tx and rx */
    gem.regs->net_ctrl = 0;
}

void gem_set_callback(gem_cb_t rx)
{
    gem.rx_callback = rx;
}

void gem_set_macaddr(uint8_t mac[6]) {
    uint32_t en = gem.regs->net_ctrl &= NET_CTRL_RX_EN | NET_CTRL_TX_EN;

    if (en) {
        gem.regs->net_ctrl &= ~(en);
    }

    /* _top register must be written after _bot register */
    gem.regs->spec_addr1_bot = (mac[3] << 24) | (mac[2] << 16) | (mac[1] << 8) | mac[0];
    gem.regs->spec_addr1_top = (mac[5] << 8) | mac[4];

    if (en) {
        gem.regs->net_ctrl |= en;
    }
}


/* Debug console commands */
static int cmd_gem(int argc, const cmd_args *argv)
{
    static uint32_t frames_rx = 0;
    static uint32_t frames_tx = 0;
    static bool run_stats = false;
    thread_t *stat_thread;

    if (argc == 1) {
        printf("gem raw <iter> <length>: Send <iter> raw mac packet for testing\n");
        printf("gem rx_debug:      toggle RX debug output\n");
        printf("gem stats          toggle periodic output of driver stats\n");
        printf("gem status:        print driver status\n");
    } else if (strncmp(argv[1].str, "rx_debug", sizeof("rx_debug")) == 0) {
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
    } else if (strncmp(argv[1].str, "status", sizeof("status")) == 0) {
        uint32_t mac_top = gem.regs->spec_addr1_top;
        uint32_t mac_bot = gem.regs->spec_addr1_bot;
        printf("mac addr: %02x:%02x:%02x:%02x:%02x:%02x\n",
            mac_top >> 8, mac_top & 0xFF, mac_bot >> 24, (mac_bot >> 16) & 0xFF,
            (mac_bot >> 8) & 0xFF, mac_bot & 0xFF);
        uint32_t rx_used = 0, tx_used = 0;
        for (int i = 0; i < GEM_RX_DESC_CNT; i++) {
            rx_used += !!(gem.descs->rx_tbl[i].addr & RX_DESC_USED);
        }

        for (int i = 0; i < GEM_TX_DESC_CNT; i++) {
            tx_used += !!(gem.descs->tx_tbl[i].ctrl & TX_DESC_USED);
        }

        frames_tx += gem.regs->frames_tx;
        frames_rx += gem.regs->frames_rx;
        printf("rx usage: %u/%u, tx usage %u/%u\n",
            rx_used, GEM_RX_DESC_CNT, tx_used, GEM_TX_DESC_CNT);
        printf("frames rx: %u, frames tx: %u\n",
            frames_rx, frames_tx);
        printf("tx:\n");
            for (size_t i = 0; i < GEM_TX_DESC_CNT; i++) {
                uint32_t ctrl = gem.descs->tx_tbl[i].ctrl;
                uint32_t addr = gem.descs->tx_tbl[i].addr;

                printf("%3zu 0x%08X 0x%08X: len %u, %s%s%s %s%s\n",
                    i, addr, ctrl, TX_BUF_LEN(ctrl),
                    (ctrl & TX_DESC_USED) ? "driver " : "controller ",
                    (ctrl & TX_DESC_WRAP) ? "wrap " : "",
                    (ctrl & TX_LAST_BUF) ? "eof " : "",
                    (i == gem.tx_head) ? "<-- HEAD " : "",
                    (i == gem.tx_tail) ? "<-- TAIL " : "");
            }

    } else if (strncmp(argv[1].str, "stats", sizeof("stats")) == 0) {
        run_stats = !run_stats;
        if (run_stats) {
            stat_thread = thread_create("gem_stat",
                    gem_stat_thread, &run_stats, LOW_PRIORITY, DEFAULT_STACK_SIZE);
            thread_resume(stat_thread);
        }
    } else if (argv[1].str[0] == 'd') {
        gem.debug_rx = !gem.debug_rx;
    }

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("gem", "ZYNQ GEM commands", &cmd_gem)
STATIC_COMMAND_END(gem);
