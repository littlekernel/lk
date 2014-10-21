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
#include <err.h>
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
#define GEM_TX_BUF_CNT      4
#define GEM_RX_BUF_SIZE     1536
#define GEM_TX_BUF_SIZE     1536


/*
 * TODO:
 * 1) TX should work with pbuf lists
 * 2) We are not in gigabit mode yet
 */

gem_cb_t rx_callback = NULL;

struct gem_desc {
    uint32_t addr;
    uint32_t ctrl;
};

struct gem_state {
    struct gem_desc rx[GEM_RX_BUF_CNT];
    struct gem_desc tx[GEM_TX_BUF_CNT];
};

static pktbuf_t *gem_rx_buffers[GEM_RX_BUF_CNT];

static event_t rx_pending;
static event_t tx_complete;
static bool debug_rx = false;
static struct gem_state *state;
static paddr_t state_phys;
static volatile struct gem_regs *regs = NULL;
static mutex_t tx_mutex;

static void dump_gem_descriptors(bool rx, bool tx) {
    if (rx) {
        for (int i = 0; i < GEM_RX_BUF_CNT; i++) {
            printf("gem rx desc %02d [%p]: [addr] = %#08x, [ctrl] = %#08x\n", i, &state->rx[i],
                    state->rx[i].addr, state->rx[i].ctrl);
        }
    }

    if (tx) {
        for (int i = 0; i < GEM_TX_BUF_CNT; i++) {
            printf("gem tx desc %02d [%p]: [addr] = %#08x, [ctrl] = %#08x\n", i, &state->tx[i],
                    state->tx[i].addr, state->tx[i].ctrl);
        }
    }
}

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


int gem_send_raw_pkt(struct pktbuf *p)
{
    status_t ret = NO_ERROR;

    mutex_acquire(&tx_mutex);

    if (!p || !p->dlen) {
        ret = -1;
        goto err;
    }

    LTRACEF("buf %p, len %zu, pkt %p\n", p->data, p->dlen, p);

    /* make sure the cache is invalidated for the packet */
    arch_clean_cache_range((addr_t)p->buffer, sizeof(p->buffer));

    /* TRM known issue #1. The TX path requires at least two descriptors
     * and the final descriptor must have the used bit set. If not done
     * then the controller will continue to wrap and send the frame multiple
     * times
     */
    state->tx[0].addr = (uintptr_t) pktbuf_data_phys(p);
    state->tx[0].ctrl = TX_BUF_LEN(p->dlen) | TX_LAST_BUF;
    state->tx[1].addr = 0;
    state->tx[1].ctrl = TX_DESC_USED | TX_DESC_WRAP;

    LTRACEF("desc 0: addr 0x%x ctrl 0x%x\n", state->tx[0].addr, state->tx[0].ctrl);
    LTRACEF("desc 1: addr 0x%x ctrl 0x%x\n", state->tx[1].addr, state->tx[1].ctrl);

    /* load the physical address of the tx descriptor */
    regs->tx_qbar = ((uintptr_t)&state->tx[0] - (uintptr_t)state) + state_phys;
    regs->net_ctrl |= NET_CTRL_START_TX;

    ret = event_wait_timeout(&tx_complete, 1000);
    if (ret == ERR_TIMED_OUT) {
        TRACEF("timed out transmitting packet\n");
    }

err:
    mutex_release(&tx_mutex);
    pktbuf_free(p);
    LTRACE_EXIT;
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
            state->rx[i].addr &= ~RX_DESC_USED;
        }

        regs->rx_status &= ~RX_STATUS_BUFFER_NOT_AVAIL;
        regs->net_ctrl &= ~NET_CTRL_RX_EN;
        regs->net_ctrl |= NET_CTRL_RX_EN;
        printf("GEM overflow, dumping pending packets\n");
    }

    if (intr_status & INTR_TX_CORRUPT) {
        printf("tx ahb error!\n");
    }

    if (intr_status & INTR_TX_COMPLETE || intr_status & INTR_TX_USED_READ) {
        state->tx[0].addr = 0;
        state->tx[0].ctrl &= TX_DESC_USED;

        state->tx[1].addr = 0;
        state->tx[1].ctrl &= TX_DESC_USED | TX_DESC_WRAP;

        regs->tx_status |= (TX_STATUS_COMPLETE | TX_STATUS_USED_READ);
        event_signal(&tx_complete, false);

        resched = true;
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

static void gem_cfg_buffer_descs(void)
{
    memset(state, 0, sizeof(struct gem_state));

    /* RX setup */
    for (int i = 0; i < GEM_RX_BUF_CNT; i++) {
        pktbuf_t *p = gem_rx_buffers[i];
        DEBUG_ASSERT(p);

        /* make sure the buffers start off with no stale data in them */
        arch_invalidate_cache_range((addr_t)p->buffer, sizeof(p->buffer));

        state->rx[i].addr = (uintptr_t) pktbuf_data_phys(p);
        state->rx[i].ctrl = 0;
    }

    memset(state->tx, 0, sizeof(state->tx));

    /* Both set of descriptors need wrap bits set */
    state->rx[GEM_RX_BUF_CNT-1].addr |= RX_DESC_WRAP;
    state->rx[GEM_TX_BUF_CNT-1].ctrl |= TX_DESC_WRAP;

    /* load the physical address of the rx descriptor */
    regs->rx_qbar = ((uintptr_t)&state->rx[0] - (uintptr_t)state) + state_phys;
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
            if (state->rx[bp].addr & RX_DESC_USED) {
                p = gem_rx_buffers[bp];
                p->dlen = RX_BUF_LEN(state->rx[bp].ctrl);
                p->data = p->buffer + 2;
                if (debug_rx) {
                    debug_rx_handler(p);
                } else if (rx_callback) {
                    rx_callback(p);
                }

                /* invalidate the buffer before putting it back */
                arch_invalidate_cache_range((addr_t)p->buffer, sizeof(p->buffer));

                state->rx[bp].addr &= ~RX_DESC_USED;
                state->rx[bp].ctrl = 0;
                bp = (bp + 1) % GEM_RX_BUF_CNT;
            } else {
                break;
            }
        }
    }

    return 0;
}

status_t gem_init(uintptr_t base, uint32_t dmasize)
{
    int n;
    status_t ret;
    thread_t *rx_thread;
    DEBUG_ASSERT(base == GEM0_BASE || base == GEM1_BASE);

    /* make sure we can allocate at least enough memory for a gem_state
     * + some buffers
     */
    if (dmasize <= PAGE_ALIGN(sizeof(struct gem_state)))
        return ERR_INVALID_ARGS;

    /* allocate a block of contiguous memory for the descriptors */
    vaddr_t dmabase;
    ret = vmm_alloc_contiguous(vmm_get_kernel_aspace(), "gem_desc",
            sizeof(*state), (void **)&dmabase, 0, 0, ARCH_MMU_FLAG_UNCACHED);
    if (ret < 0)
        return ret;

    /* get the physical address */
    paddr_t dmabase_phys;
    ret = arch_mmu_query(dmabase, &dmabase_phys, NULL);
    if (ret < 0)
        return ret;

    TRACEF("dmabase 0x%lx, dmabase_phys 0x%lx, size %zu\n", dmabase, dmabase_phys, sizeof(*state));

    /* tx/rx descriptor tables */
    state = (void *)dmabase;
    state_phys = dmabase_phys;

    /* allocate packet buffers */
    ret = vmm_alloc_contiguous(vmm_get_kernel_aspace(), "gem_desc",
            dmasize, (void **)&dmabase, 0, 0, ARCH_MMU_FLAG_CACHED);
    if (ret < 0)
        return ret;

    ret = arch_mmu_query(dmabase, &dmabase_phys, NULL);
    if (ret < 0)
        return ret;

    TRACEF("packetbuf 0x%lx, packetbuf_phys 0x%lx, size %zu\n", dmabase, dmabase_phys, dmasize);

    /* allocate packet buffers */
    while (dmasize >= PKTBUF_SIZE) {
        pktbuf_create((void *)dmabase, dmabase_phys, PKTBUF_SIZE);
        dmasize -= PKTBUF_SIZE;
        dmabase += PKTBUF_SIZE;
        dmabase_phys += PKTBUF_SIZE;
    }

    for (n = 0; n < GEM_RX_BUF_CNT; n++) {
        if ((gem_rx_buffers[n] = pktbuf_alloc()) == NULL) {
            printf("cannot allocate %d rx buffers\n", GEM_RX_BUF_CNT);
            return -1;
        }
        gem_rx_buffers[n]->data = gem_rx_buffers[n]->buffer;
    }

    /* Lock / scheduling init */
    mutex_init(&tx_mutex);
    event_init(&tx_complete, false, EVENT_FLAG_AUTOUNSIGNAL);
    event_init(&rx_pending, false, EVENT_FLAG_AUTOUNSIGNAL);

    /* rx background thread */
    rx_thread = thread_create("gem_rx", gem_rx_thread, NULL, HIGH_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(rx_thread);

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

    regs = (struct gem_regs *) base;
    /* Clear Network control / status registers */
    regs->net_ctrl |= NET_CTRL_STATCLR;
    regs->rx_status = 0x0F;
    regs->tx_status = 0xFF;
    /* Disable interrupts */
    regs->intr_dis  = 0x7FFFEFF;
    /* Empty out the buffer queues */
    regs->rx_qbar = 0;
    regs->tx_qbar = 0;

    /* Configure for:
     * Ghz enabled, 100mhz default
     * broadcast / multicast enabled, hw checksums,
     * clock divider 48, assuming 80MHz < cpu_1xclk < 120MHz
     * skip first two bytes of rx buffer (ensure ip header alignment)
     */
    regs->net_cfg = NET_CFG_FULL_DUPLEX | NET_CFG_GIGE_EN | NET_CFG_SPEED_100 |
        NET_CFG_RX_CHKSUM_OFFLD_EN | NET_CFG_FCS_REMOVE | NET_CFG_MDC_CLK_DIV(0x7) |
        NET_CFG_RX_BUF_OFFSET(2);

    /* Set DMA to 1600 byte rx buffer, 8KB addr space for rx, 4KB addr space for tx,
     * hw checksumming, little endian, and use INCR16 ahb bursts
     */
    regs->dma_cfg = DMA_CFG_AHB_MEM_RX_BUF_SIZE(0x19) | DMA_CFG_RX_PKTBUF_MEMSZ_SEL(0x3) |
            DMA_CFG_TX_PKTBUF_MEMSZ_SEL | DMA_CFG_CSUM_GEN_OFFLOAD_EN |
            DMA_CFG_AHB_FIXED_BURST_LEN(0x16);

    /* Enable VREF from GPIOB */
    SLCR_REG(GPIOB_CTRL) = 0x1;

    ret = gem_phy_init();
    if (!ret) {
        printf("Phy not idle, aborting!\n");
        return ret;
    }

    gem_cfg_buffer_descs();
    gem_cfg_ints();

    regs->net_ctrl = NET_CTRL_MD_EN | NET_CTRL_RX_EN | NET_CTRL_TX_EN;

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
            rx_used += !!(state->rx[i].addr & RX_DESC_USED);
        }

        for (int i = 0; i < GEM_TX_BUF_CNT; i++) {
            tx_used += !!(state->tx[i].ctrl & TX_DESC_USED);
        }

        frames_tx += regs->frames_tx;
        frames_rx += regs->frames_rx;
        printf("rx usage: %u/%u, tx usage %u/%u\n",
            rx_used, GEM_RX_BUF_CNT, tx_used, GEM_TX_BUF_CNT);
        printf("frames rx: %u, frames tx: %u\n",
            frames_rx, frames_tx);
    } else if (argv[1].str[0] == 'd') {
        debug_rx = !debug_rx;
    } else if (argv[1].str[0] == 'p') {
        dump_gem_descriptors(true, true);
    } else if (argv[1].str[0] == 'r') {
        gem_cfg_buffer_descs();
    }

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("gem", "ZYNQ GEM commands", &cmd_gem)
STATIC_COMMAND_END(gem);
