//
// Copyright (c) 2021 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <arch/atomic.h>
#include <dev/bus/pci.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <lib/minip.h>
#include <lib/minip/netif.h>
#include <lib/pktbuf.h>
#include <lk/cpp.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/list.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <platform/interrupts.h>
#include <string.h>

#include "e1000_hw.h"
#include "e1000_ids.h"

#define LOCAL_TRACE 0

namespace {
class e1000;

class e1000 {
  public:
    e1000();
    ~e1000();

    status_t init_device(pci_location_t loc, const e1000_id_features *id);

    int tx(pktbuf_t *p);

    bool is_e1000e() const { return id_feat_->e1000e; }

    const uint8_t *mac_addr() const { return mac_addr_; }
    netif_t *netif() { return &netif_; }

    int unit() const { return unit_; }
    pci_location_t loc() const { return loc_; }

#if LK_DEBUGLEVEL >= 2
    uint32_t get_reg(e1000_reg reg) { return read_reg(reg); }
    void set_reg(e1000_reg reg, uint32_t val) { write_reg(reg, val); }
    void dump_status();

    template <typename F> static void foreach_instance(F func) {
        for (auto *inst : instances_) {
            if (inst) {
                func(inst);
            }
        }
    }
#endif

  private:
#if LK_DEBUGLEVEL >= 2
    static e1000 *instances_[8];
#endif
    static const size_t rxring_len = 64;
    static const size_t txring_len = 64;
    static const size_t rxbuffer_len = 2048;

    uint32_t read_reg(e1000_reg reg);
    void write_reg(e1000_reg reg, uint32_t val);
    uint16_t read_eeprom(uint8_t offset);
    status_t write_reg_and_poll_bit(e1000_reg reg, uint32_t val, uint32_t bit_mask, bool set,
                                    int timeout_count = 10000);

    handler_return irq_handler();

    void add_pktbuf_to_rxring(pktbuf_t *pkt);
    void add_pktbuf_to_rxring_locked(pktbuf_t *pkt);

    // counter of configured deices
    static volatile int global_count_;
    int unit_ = 0;

    // main spinlock
    spin_lock_t lock_ = SPIN_LOCK_INITIAL_VALUE;

    // configuration
    pci_location_t loc_ = {};
    void *bar0_regs_ = nullptr;
    uint8_t mac_addr_[6] = {};
    const e1000_id_features *id_feat_ = nullptr;

    // minip network interface
    netif_t netif_ = {};

    // rx ring
    rdesc *rxring_ = nullptr;
    uint32_t rx_last_head_ = 0;
    uint32_t rx_tail_ = 0;
    pktbuf_t *rx_pktbuf_[rxring_len] = {};
    uint8_t *rx_buf_ = nullptr; // rxbuffer_len * rxring_len byte buffer that rx_pktbuf[] points to
    pktbuf_t *rx_pending_pkt_ = nullptr;

    // rx worker thread
    list_node rx_queue_ = LIST_INITIAL_VALUE(rx_queue_);
    event_t rx_event_ = EVENT_INITIAL_VALUE(rx_event_, 0, EVENT_FLAG_AUTOUNSIGNAL);
    thread_t *rx_worker_thread_ = nullptr;
    int rx_worker_routine();

    // tx ring
    tdesc *txring_ = nullptr;
    uint32_t tx_last_head_ = 0;
    uint32_t tx_tail_ = 0;
    pktbuf_t *tx_pktbuf_[txring_len] = {};
};

volatile int e1000::global_count_ = 0;
#if LK_DEBUGLEVEL >= 2
e1000 *e1000::instances_[8] = {};
#endif

uint32_t e1000::read_reg(e1000_reg reg) {
    auto *r = reinterpret_cast<volatile uint32_t *>(reinterpret_cast<uintptr_t>(bar0_regs_) +
                                                    static_cast<size_t>(reg));

    return mmio_read32(r);
}

void e1000::write_reg(e1000_reg reg, uint32_t val) {
    auto *r = reinterpret_cast<volatile uint32_t *>(reinterpret_cast<uintptr_t>(bar0_regs_) +
                                                    static_cast<size_t>(reg));

    mmio_write32(r, val);
}

uint16_t e1000::read_eeprom(uint8_t offset) {
    // 8257x+ seems to have a different EERD layout
    uint32_t val;
    if (is_e1000e()) {
        write_reg(e1000_reg::EERD, (offset << 2) | 0x1); // data + start bit

        // spin while bit 1 (DONE) is clear
        int timeout = 10000;
        while (((val = read_reg(e1000_reg::EERD)) & (1 << 1)) == 0) {
            if (--timeout == 0) {
                return 0xffff;
            }
        }
    } else {
        write_reg(e1000_reg::EERD, (offset << 8) | 0x1); // data + start bit

        // spin while bit 4 (DONE) is clear
        int timeout = 10000;
        while (((val = read_reg(e1000_reg::EERD)) & (1 << 4)) == 0) {
            if (--timeout == 0) {
                return 0xffff;
            }
        }
    }
    return val >> 16;
}

status_t e1000::write_reg_and_poll_bit(e1000_reg reg, uint32_t val, uint32_t bit_mask, bool set,
                                       int timeout_count) {
    write_reg(reg, val);
    while (((read_reg(reg) & bit_mask) != 0) != set) {
        if (--timeout_count == 0) {
            return ERR_TIMED_OUT;
        }
    }
    return NO_ERROR;
}

e1000::e1000() = default;
e1000::~e1000() {
    // TODO: handle cleanup
#if LK_DEBUGLEVEL >= 2
    for (auto &inst : instances_) {
        if (inst == this) {
            inst = nullptr;
            break;
        }
    }
#endif
}

handler_return e1000::irq_handler() {
    // read the interrupt cause register, which also auto clears all bits
    auto icr = read_reg(e1000_reg::ICR);
    if (!icr) {
        return INT_NO_RESCHEDULE;
    }

    LTRACEF("icr %#x\n", icr);

    AutoSpinLockNoIrqSave guard(&lock_);

    handler_return ret = INT_NO_RESCHEDULE;

    if (icr & E1000_ICR_TXDW) { // TXDW - transmit descriptor written back
        // Walk from last known head to current TDH, freeing completed TX pktbufs.
        auto tdh = read_reg(e1000_reg::TDH);
        while (tx_last_head_ != tdh) {
            if (tx_pktbuf_[tx_last_head_]) {
                pktbuf_free(tx_pktbuf_[tx_last_head_], false);
                tx_pktbuf_[tx_last_head_] = nullptr;
                ret = INT_RESCHEDULE;
            }
            tx_last_head_ = (tx_last_head_ + 1) % txring_len;
        }
    }
    if (icr & E1000_ICR_LSC) { // LSC - link status change
        LTRACEF("link status change, STATUS=%#x\n", read_reg(e1000_reg::STATUS));
    }
    if (icr & E1000_ICR_TXQE) { // TXQE - transmit queue empty
        // nothing to really do here
    }
    if (icr & E1000_ICR_RXO) {
        LTRACEF("RX overrun\n");

        // Any in-flight multi-descriptor frame is no longer trustworthy after overrun.
        // Drop it so subsequent fragments do not get appended to stale packet state.
        if (rx_pending_pkt_) {
            pktbuf_reset(rx_pending_pkt_, 0);
            add_pktbuf_to_rxring_locked(rx_pending_pkt_);
            rx_pending_pkt_ = nullptr;
        }
    }
    if (icr & (E1000_ICR_RXTO | E1000_ICR_RXO)) { // RXTO/RXO - rx work pending
        // Packets may be ready, or descriptors may need draining after overrun.
        auto rdh = read_reg(e1000_reg::RDH);
        auto rdt = read_reg(e1000_reg::RDT);

        while (rx_last_head_ != rdh) {
            // copy the current rx descriptor locally for better cache performance
            rdesc rxd;
            copy(&rxd, rxring_ + rx_last_head_);

            LTRACEF("last_head %#x RDH %#x RDT %#x\n", rx_last_head_, rdh, rdt);
            if (LOCAL_TRACE) {
                rxd.dump();
            }

            // recover the pktbuf we queued in this spot
            DEBUG_ASSERT(rx_pktbuf_[rx_last_head_]);
            DEBUG_ASSERT(pktbuf_data_phys(rx_pktbuf_[rx_last_head_]) == rxd.addr);
            pktbuf_t *pkt = rx_pktbuf_[rx_last_head_];

            bool consumed_pkt = false;
            if (rxd.status & E1000_RXD_STAT_DD) { // descriptor done, we own it now
                bool eop = (rxd.status & E1000_RXD_STAT_EOP);

                if (rxd.errors == 0) {
                    if (rx_pending_pkt_) {
                        // We are in the middle of a multi-descriptor packet. Append this fragment.
                        if (pktbuf_avail_tail(rx_pending_pkt_) >= rxd.length) {
                            // minip consumes a single contiguous pktbuf per frame. Copying this
                            // fragment lets us present one complete packet while returning this
                            // descriptor buffer immediately to the RX ring.
                            pktbuf_append_data(rx_pending_pkt_, pkt->data, rxd.length);

                            // This fragment buffer was consumed by copy. Recycle it to the rx ring.
                            pktbuf_reset(pkt, 0);
                            add_pktbuf_to_rxring_locked(pkt);
                            consumed_pkt = true;

                            if (eop) {
                                // Packet is now complete.
                                rx_pending_pkt_->flags |= PKTBUF_FLAG_EOF;
                                list_add_tail(&rx_queue_, &rx_pending_pkt_->list);
                                rx_pending_pkt_ = nullptr;
                                event_signal(&rx_event_, false);
                                ret = INT_RESCHEDULE;
                            }
                        } else {
                            // Coalesced packet exceeded our fixed receive buffer. Drop and recover.
                            pktbuf_reset(rx_pending_pkt_, 0);
                            add_pktbuf_to_rxring_locked(rx_pending_pkt_);
                            rx_pending_pkt_ = nullptr;
                        }
                    } else {
                        // Start or finish a packet from this descriptor.
                        pkt->dlen = rxd.length;
                        if (eop) {
                            pkt->flags |= PKTBUF_FLAG_EOF;
                            list_add_tail(&rx_queue_, &pkt->list);
                            event_signal(&rx_event_, false);
                            ret = INT_RESCHEDULE;
                            consumed_pkt = true;
                        } else {
                            // Save first fragment until we see EOP.
                            pkt->flags &= ~PKTBUF_FLAG_EOF;
                            rx_pending_pkt_ = pkt;
                            consumed_pkt = true;
                        }
                    }
                } else {
                    // Descriptor has errors. Drop this packet and any in-progress coalesced frame.
                    if (rx_pending_pkt_) {
                        pktbuf_reset(rx_pending_pkt_, 0);
                        add_pktbuf_to_rxring_locked(rx_pending_pkt_);
                        rx_pending_pkt_ = nullptr;
                    }
                }
            }
            if (!consumed_pkt) {
                // TODO: return the pkt to the ring
                add_pktbuf_to_rxring_locked(pkt);
            }

            rx_last_head_ = (rx_last_head_ + 1) % rxring_len;
        }
    }
    return ret;
}

int e1000::rx_worker_routine() {
    for (;;) {
        event_wait(&rx_event_);

        // pull some packets from the received queue
        for (;;) {
            pktbuf_t *p;

            {
                AutoSpinLock guard(&lock_);

                p = list_remove_head_type(&rx_queue_, pktbuf_t, list);
            }

            if (!p) {
                break; // nothing left in the queue, go back to waiting
            }

            if (LOCAL_TRACE) {
                LTRACEF("got packet: ");
                pktbuf_dump(p);
            }

            // push it up the stack
            minip_rx_driver_callback(&netif_, p);

            // we own the pktbuf again

            // set the data pointer to the start of the buffer and set dlen to 0
            pktbuf_reset(p, 0);

            // add it back to the rx ring at the current tail
            add_pktbuf_to_rxring(p);
        }
    }

    return 0;
}

int e1000::tx(pktbuf_t *p) {
    LTRACE;
    if (LOCAL_TRACE) {
        pktbuf_dump(p);
    }

    // build a tx descriptor and stuff it in the tx ring
    tdesc td = {};
    td.addr = pktbuf_data_phys(p);
    td.length = p->dlen;
    td.cmd = (1 << 3) | (1 << 1) | (1 << 0); // RS | IFCS | EOP
    copy(&txring_[tx_tail_], &td);

    // save a copy of the pktbuf in our list
    tx_pktbuf_[tx_tail_] = p;

    // bump tail forward
    tx_tail_ = (tx_tail_ + 1) % txring_len;
    write_reg(e1000_reg::TDT, tx_tail_);

    LTRACEF("TDH %#x TDT %#x\n", read_reg(e1000_reg::TDH), read_reg(e1000_reg::TDT));

    return NO_ERROR;
}

void e1000::add_pktbuf_to_rxring_locked(pktbuf_t *p) {
    DEBUG_ASSERT(p);
    DEBUG_ASSERT(p->dlen == 0);
    DEBUG_ASSERT(p->blen == rxbuffer_len);

    // add it to the next rxring entry at the tail
    rdesc rd = {};
    rd.addr = pktbuf_data_phys(p);
    copy(&rxring_[rx_tail_], &rd);

    // save a copy of the pktbuf in our list
    rx_pktbuf_[rx_tail_] = p;

    // bump tail forward
    rx_tail_ = (rx_tail_ + 1) % rxring_len;
    write_reg(e1000_reg::RDT, rx_tail_);

    LTRACEF("after RDH %#x RDT %#x\n", read_reg(e1000_reg::RDH), read_reg(e1000_reg::RDT));
}

void e1000::add_pktbuf_to_rxring(pktbuf_t *pkt) {
    AutoSpinLock guard(&lock_);

    add_pktbuf_to_rxring_locked(pkt);
}

status_t e1000::init_device(pci_location_t loc, const e1000_id_features *id) {
    loc_ = loc;
    id_feat_ = id;
    char str[32];

    LTRACEF("pci location %s\n", pci_loc_string(loc_, str));

    pci_bar_t bars[6];
    status_t err = pci_bus_mgr_read_bars(loc_, bars);
    if (err != NO_ERROR) {
        return err;
    }

    LTRACEF("e1000 BARS:\n");
    if (LOCAL_TRACE) {
        pci_dump_bars(bars, 6);
    }

    if (!bars[0].valid || bars[0].addr == 0) {
        return ERR_NOT_FOUND;
    }

    // allocate a unit number
    unit_ = atomic_add(&global_count_, 1);

    // map bar 0, main memory mapped register interface, 128KB
    snprintf(str, sizeof(str), "e1000 %d bar0", unit_);
    err = vmm_alloc_physical(vmm_get_kernel_aspace(), str, 128 * 1024, &bar0_regs_, 0, bars[0].addr,
                             /* vmm_flags */ 0, ARCH_MMU_FLAG_UNCACHED_DEVICE);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }

    LTRACEF("bar 0 regs mapped to %p\n", bar0_regs_);

    pci_bus_mgr_enable_device(loc_);

    // read the mac address: try EEPROM first, fall back to RAL0/RAH0
    uint16_t tmp;
    tmp = read_eeprom(0);
    mac_addr_[0] = tmp & 0xff;
    mac_addr_[1] = tmp >> 8;
    tmp = read_eeprom(1);
    mac_addr_[2] = tmp & 0xff;
    mac_addr_[3] = tmp >> 8;
    tmp = read_eeprom(2);
    mac_addr_[4] = tmp & 0xff;
    mac_addr_[5] = tmp >> 8;

    // If EEPROM timed out (all 0xff) fall back to the receive address registers
    // programmed by firmware (common on PCH-integrated i219).
    bool mac_invalid = (mac_addr_[0] == 0xff && mac_addr_[1] == 0xff && mac_addr_[2] == 0xff &&
                        mac_addr_[3] == 0xff && mac_addr_[4] == 0xff && mac_addr_[5] == 0xff);
    if (mac_invalid) {
        // EEPROM not accessible (e.g. PCH-integrated i219): read MAC from receive address
        // registers programmed by firmware.
        uint32_t ral = read_reg(e1000_reg::RAL0);
        uint32_t rah = read_reg(e1000_reg::RAH0);
        if (rah & (1u << 31)) { // valid bit
            mac_addr_[0] = (ral >> 0) & 0xff;
            mac_addr_[1] = (ral >> 8) & 0xff;
            mac_addr_[2] = (ral >> 16) & 0xff;
            mac_addr_[3] = (ral >> 24) & 0xff;
            mac_addr_[4] = (rah >> 0) & 0xff;
            mac_addr_[5] = (rah >> 8) & 0xff;
        } else {
            printf("e1000 %d: unable to read MAC address\n", unit_);
        }
    }

    printf("e1000 %d: mac address %02x:%02x:%02x:%02x:%02x:%02x\n", unit_, mac_addr_[0],
           mac_addr_[1], mac_addr_[2], mac_addr_[3], mac_addr_[4], mac_addr_[5]);

    // allocate and map space for the rx and tx ring
    snprintf(str, sizeof(str), "e1000 %d rxring", unit_);
    err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), str, rxring_len * sizeof(rdesc),
                               reinterpret_cast<void **>(&rxring_), 0, 0, ARCH_MMU_FLAG_UNCACHED);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }
    memset(rxring_, 0, rxring_len * sizeof(rdesc));

    paddr_t rxring_phys = vaddr_to_paddr(rxring_);
    LTRACEF("rx ring at %p, physical %#lx\n", rxring_, rxring_phys);

    snprintf(str, sizeof(str), "e1000 %d txring", unit_);
    err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), str, txring_len * sizeof(tdesc),
                               reinterpret_cast<void **>(&txring_), 0, 0, ARCH_MMU_FLAG_UNCACHED);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }
    memset(txring_, 0, txring_len * sizeof(rdesc));

    paddr_t txring_phys = vaddr_to_paddr(txring_);
    LTRACEF("tx ring at %p, physical %#lx\n", txring_, txring_phys);

    // allocate a large array of contiguous buffers to receive into
    snprintf(str, sizeof(str), "e1000 %d rx buffers", unit_);
    err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), str, rxring_len * rxbuffer_len,
                               reinterpret_cast<void **>(&rx_buf_), 0, 0, 0);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }

    // mask all IRQs
    write_reg(e1000_reg::IMC, 0xffff);

    // qemus 82574 emulation seems to want IAME to be set to auto-clear ICR bits.
    if (is_e1000e()) {
        auto ctrl_ext = read_reg(e1000_reg::CTL_EXT);
        write_reg(e1000_reg::CTL_EXT, ctrl_ext | (1 << 27)); // IAME - interrupt ack auto-mask
        write_reg(e1000_reg::IAM, 0); // set such that no IMS bits are auto cleared
    }

    // set the interrupt treshold reg
    const uint32_t irq_rate = 10000; // max 10k irqs/sec
    write_reg(e1000_reg::ITR, 1000000 / irq_rate * 4);
    if (is_e1000e()) {
        write_reg(e1000_reg::EITR0, 1000000 / irq_rate * 4);
        write_reg(e1000_reg::EITR1, 1000000 / irq_rate * 4);
        write_reg(e1000_reg::EITR2, 1000000 / irq_rate * 4);
        write_reg(e1000_reg::EITR3, 1000000 / irq_rate * 4);
        write_reg(e1000_reg::EITR4, 1000000 / irq_rate * 4);
    }

    // disable tx and rx
    write_reg(e1000_reg::RCTL, 0);
    write_reg(e1000_reg::TCTL, 0);

    // irq handler lambda to get to inner method
    auto irq_handler_wrapper = [](void *arg) -> handler_return {
        e1000 *e = static_cast<e1000 *>(arg);
        return e->irq_handler();
    };

    // allocate a MSI interrupt
    uint irq_base;
    err = pci_bus_mgr_allocate_msi(loc_, 1, &irq_base);
    if (err != NO_ERROR) {
        // fall back to regular IRQs
        err = pci_bus_mgr_allocate_irq(loc_, &irq_base);
        if (err != NO_ERROR) {
            printf("e1000: unable to allocate IRQ\n");
            return err;
        }
        register_int_handler(irq_base, irq_handler_wrapper, this);
    } else {
        register_int_handler_msi(irq_base, irq_handler_wrapper, this, true);
    }
    LTRACEF("IRQ number %#x\n", irq_base);

    unmask_interrupt(irq_base);

    // set up the rx ring
    write_reg(e1000_reg::RDBAL, rxring_phys & 0xffffffff);
#if __INTPTR_WIDTH__ == 64
    write_reg(e1000_reg::RDBAH, rxring_phys >> 32);
#else
    write_reg(e1000_reg::RDBAH, 0);
#endif
    write_reg(e1000_reg::RDLEN, rxring_len * sizeof(rdesc));
    // set head and tail to 0
    write_reg(e1000_reg::RDH, 0);
    write_reg(e1000_reg::RDT, 0);

    // disable receive delay timer and absolute delay timer
    write_reg(e1000_reg::RDTR, 0);
    write_reg(e1000_reg::RADV, 0);
    // disable small packet detect
    write_reg(e1000_reg::RSRPD, 0);

    // set up the flow control thresholds
    write_reg(e1000_reg::FCRTL, 0);
    write_reg(e1000_reg::FCRTH, 0);

    if (is_e1000e()) {
        // Configure RXDCTL:
        // - Bit 25: Queue Enable
        // - Bit 24: Granularity (1 = descriptor granularity, 0 = cache line)
        // - Bits 16-21: Writeback Threshold (WTHRESH = 1 descriptor)
        // - Bits 8-13: Host Threshold (HTHRESH = 1 descriptor)
        // - Bits 0-5: Prefetch Threshold (PTHRESH = 16 descriptors)
        if (write_reg_and_poll_bit(e1000_reg::RXDCTL,
                                   (1 << 25) | (1 << 24) | (1 << 16) | (1 << 8) | 16, 1 << 25,
                                   true) != NO_ERROR) {
            printf("e1000 %d: timeout waiting for RX queue enable\n", unit_);
        }
    }

    // fill the rx ring with pktbufs
    rx_last_head_ = read_reg(e1000_reg::RDH);
    rx_tail_ = read_reg(e1000_reg::RDT);
    for (size_t i = 0; i < rxring_len - 1; i++) {
        // construct a 2K pktbuf, pointing outo our rx_buf_ block of memory
        auto *pkt = pktbuf_alloc_empty();
        if (!pkt) {
            break;
        }
        pktbuf_add_buffer(pkt, rx_buf_ + i * rxbuffer_len, rxbuffer_len, 0, 0, nullptr, nullptr);

        add_pktbuf_to_rxring_locked(pkt);
    }
    // hexdump(rxring_, rxring_len * sizeof(rdesc));

    // start rx worker thread
    auto wrapper_lambda = [](void *arg) -> int {
        e1000 *e = static_cast<e1000 *>(arg);
        return e->rx_worker_routine();
    };
    snprintf(str, sizeof(str), "e1000 %d rx worker", unit_);
    rx_worker_thread_ = thread_create(str, wrapper_lambda, this, HIGH_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(rx_worker_thread_);

    // start receiver
    // enable RX, unicast permiscuous, multicast permiscuous, broadcast accept, BSIZE 2048
    write_reg(e1000_reg::RCTL, (1 << 1) | (1 << 3) | (1 << 4) | (1 << 15) | (0 << 16));

    // unmask receive irq
    auto ims = read_reg(e1000_reg::IMS);
    write_reg(e1000_reg::IMS, ims | E1000_ICR_RXTO | E1000_ICR_RXO);

    // set up the tx path
    write_reg(e1000_reg::TDH, 0);
    write_reg(e1000_reg::TDT, 0);
    tx_last_head_ = 0;
    tx_tail_ = 0;

    // set up the tx ring
    write_reg(e1000_reg::TDBAL, txring_phys & 0xffffffff);
#if __INTPTR_WIDTH__ == 64
    write_reg(e1000_reg::TDBAH, txring_phys >> 32);
#else
    write_reg(e1000_reg::TDBAH, 0);
#endif
    write_reg(e1000_reg::TDLEN, txring_len * sizeof(tdesc));

    if (is_e1000e()) {
        // Configure TXDCTL:
        // - Bit 25: Queue Enable
        // - Bit 24: Granularity (1 = descriptor granularity, 0 = cache line)
        // - Bits 16-21: Writeback Threshold (WTHRESH = 1 descriptor)
        // - Bits 8-13: Host Threshold (HTHRESH = 1 descriptor)
        // - Bits 0-5: Prefetch Threshold (PTHRESH = 16 descriptors)
        if (write_reg_and_poll_bit(e1000_reg::TXDCTL,
                                   (1 << 25) | (1 << 24) | (1 << 16) | (1 << 8) | 16, 1 << 25,
                                   true) != NO_ERROR) {
            printf("e1000 %d: timeout waiting for TX queue enable\n", unit_);
        }
    }

    // enable the transmitter and appropriate irqs
    // TIPG: standard copper gigabit inter-packet gap (IPGR2=6, IPGR1=8, IPGT=8)
    write_reg(e1000_reg::TIPG, (6u << 20) | (8u << 10) | 8u);
    // Configure TCTL:
    // - Bit 1: Transmit Enable (EN)
    // - Bit 3: Pad Short Packets (PSP)
    // - Bits 4-11: Collision Threshold (CT = 15 retries)
    // - Bits 12-21: Collision Distance (COLD = 64 slot time)
    write_reg(e1000_reg::TCTL, (1 << 3) | (1 << 1) | (0x0f << 4) | (0x40 << 12));

    // unmask tx irq
    ims = read_reg(e1000_reg::IMS);
    write_reg(e1000_reg::IMS, ims | E1000_ICR_TXQE | E1000_ICR_TXDW);

    // register this NIC instance with minip's netif layer
    snprintf(str, sizeof(str), "e1000-%d", unit_);
    netif_create(&netif_, str);
    auto tx = [](void *arg, pktbuf_t *p) -> int {
        auto *e = static_cast<e1000 *>(arg);
        DEBUG_ASSERT(e);
        return e->tx(p);
    };

    netif_set_eth(&netif_, tx, this, mac_addr_);
    netif_register(&netif_);

    // add to list of instances
#if LK_DEBUGLEVEL >= 2
    for (auto &inst : instances_) {
        if (!inst) {
            inst = this;
            break;
        }
    }
#endif

    return NO_ERROR;
}

void e1000_init(uint level) {
    LTRACE_ENTRY;

    auto ac = lk::make_auto_call([]() { LTRACE_EXIT; });

    // probe pci to find a device
    for (auto id : e1000_ids) {
        for (size_t i = 0;; i++) {
            pci_location_t loc;
            status_t err = pci_bus_mgr_find_device(&loc, id.id, 0x8086, i);
            if (err != NO_ERROR) {
                break;
            }

            // we maybe found one, create a new device and initialize it
            auto e = new e1000;
            err = e->init_device(loc, &id);
            if (err != NO_ERROR) {
                char str[14];
                printf("e1000: device at %s failed to initialize\n", pci_loc_string(loc, str));
                delete e;
                continue;
            }
        }
    }
}

#if LK_DEBUGLEVEL >= 2
#include <lk/console_cmd.h>

void e1000::dump_status() {
    char loc_str[14];
    printf("e1000 unit %d (%s) at pci %s\n", unit_, is_e1000e() ? "e1000e" : "e1000",
           pci_loc_string(loc_, loc_str));
    printf("\tMAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac_addr_[0], mac_addr_[1],
           mac_addr_[2], mac_addr_[3], mac_addr_[4], mac_addr_[5]);
    printf("\tMapped regs: %p\n", bar0_regs_);

    uint16_t pci_cmd = 0;
    pci_read_config_half(loc_, PCI_CONFIG_COMMAND, &pci_cmd);
    printf("\tPCI Command: %#x\n", pci_cmd);

    uint32_t status = read_reg(e1000_reg::STATUS);
    bool lu = (status & (1 << 1)) != 0;
    uint32_t speed_bits = (status >> 6) & 3;
    const char *speed = "unknown";
    if (speed_bits == 0) {
        speed = "10Mb/s";
    } else if (speed_bits == 1) {
        speed = "100Mb/s";
    } else if (speed_bits == 2 || speed_bits == 3) {
        speed = "1000Mb/s";
    }
    printf("\tLink status: %s, Speed %s (STATUS register %#010x)\n", lu ? "UP" : "DOWN", speed,
           status);

    // Read and print registers
    printf("\tRegisters:\n");
    printf("\t\tCTRL:    %#010x\n", read_reg(e1000_reg::CTRL));
    printf("\t\tEECD:    %#010x\n", read_reg(e1000_reg::EECD));
    printf("\t\tEERD:    %#010x\n", read_reg(e1000_reg::EERD));
    printf("\t\tCTL_EXT: %#010x\n", read_reg(e1000_reg::CTL_EXT));
    printf("\t\tICR:     %#010x\n", read_reg(e1000_reg::ICR));
    printf("\t\tIMS:     %#010x\n", read_reg(e1000_reg::IMS));
    printf("\t\tRCTL:    %#010x\n", read_reg(e1000_reg::RCTL));
    printf("\t\tTCTL:    %#010x\n", read_reg(e1000_reg::TCTL));
    printf("\t\tTIPG:    %#010x\n", read_reg(e1000_reg::TIPG));
    printf("\t\tRXDCTL:  %#010x\n", read_reg(e1000_reg::RXDCTL));
    printf("\t\tTXDCTL:  %#010x\n", read_reg(e1000_reg::TXDCTL));
    printf("\t\tRDH:     %d\n", read_reg(e1000_reg::RDH));
    printf("\t\tRDT:     %d\n", read_reg(e1000_reg::RDT));
    printf("\t\tTDH:     %d\n", read_reg(e1000_reg::TDH));
    printf("\t\tTDT:     %d\n", read_reg(e1000_reg::TDT));
    printf("\t\tRDBAL:   %#010x RDBAH:   %#010x RDLEN: %d\n", read_reg(e1000_reg::RDBAL),
           read_reg(e1000_reg::RDBAH), read_reg(e1000_reg::RDLEN));
    printf("\t\tTDBAL:   %#010x TDBAH:   %#010x TDLEN: %d\n", read_reg(e1000_reg::TDBAL),
           read_reg(e1000_reg::TDBAH), read_reg(e1000_reg::TDLEN));

    printf("\tTX descriptors around TDH/TDT:\n");
    uint32_t tdh = read_reg(e1000_reg::TDH);
    uint32_t tdt = read_reg(e1000_reg::TDT);
    for (size_t i = 0; i < txring_len; i++) {
        if (i == tdh || i == tdt || tx_pktbuf_[i]) {
            printf("\t\t[%zu]%s%s addr: %#llx length: %d cmd: %#x sta_rsv: %#x\n", i,
                   (i == tdh) ? " ->TDH" : "", (i == tdt) ? " ->TDT" : "", txring_[i].addr,
                   txring_[i].length, txring_[i].cmd, txring_[i].sta_rsv);
        }
    }
}

static int e1000_cmd(int argc, const console_cmd_args *argv) {
    if (argc < 2) {
        printf("e1000 commands:\n");
        printf("\t%s status - dump status of all e1000 instances\n", argv[0].str);
        printf("\t%s reg <unit> <reg_offset_hex> [value_hex] - read or write a register\n",
               argv[0].str);
        printf("\t%s set_link <unit> <0|1> - toggle CTRL.SLU (set link up)\n", argv[0].str);
        return 0;
    }

    if (!strcmp(argv[1].str, "status")) {
        e1000::foreach_instance([](e1000 *e) { e->dump_status(); });
        return 0;
    }

    if (argc < 3) {
        printf("missing unit number\n");
        return -1;
    }

    int unit = argv[2].i;
    e1000 *target = nullptr;
    e1000::foreach_instance([unit, &target](e1000 *e) {
        if (e->unit() == unit) {
            target = e;
        }
    });

    if (!target) {
        printf("e1000 unit %d not found\n", unit);
        return -1;
    }

    if (!strcmp(argv[1].str, "set_link")) {
        if (argc < 4) {
            printf("missing link value (0 or 1)\n");
            return -1;
        }
        bool link_up = argv[3].u != 0;
        uint32_t ctrl = target->get_reg(e1000_reg::CTRL);
        if (link_up) {
            ctrl |= (1 << 6); // SLU (Set Link Up)
        } else {
            ctrl &= ~(1 << 6);
        }
        printf("setting CTRL to %#x\n", ctrl);
        target->set_reg(e1000_reg::CTRL, ctrl);
        return 0;
    }

    if (!strcmp(argv[1].str, "reg")) {
        if (argc < 4) {
            printf("missing register offset\n");
            return -1;
        }
        uint32_t offset = argv[3].u;
        if (argc >= 5) {
            uint32_t val = argv[4].u;
            printf("writing %#x to reg offset %#x\n", val, offset);
            target->set_reg(static_cast<e1000_reg>(offset), val);
        } else {
            uint32_t val = target->get_reg(static_cast<e1000_reg>(offset));
            printf("reg offset %#x = %#x\n", offset, val);
        }
        return 0;
    }

    printf("unknown subcommand %s\n", argv[1].str);
    return -1;
}

STATIC_COMMAND_START
STATIC_COMMAND("e1000", "e1000 diagnostics", &e1000_cmd)
STATIC_COMMAND_END(e1000);
#endif

LK_INIT_HOOK(e1000, &e1000_init, LK_INIT_LEVEL_PLATFORM + 1);

} // namespace
