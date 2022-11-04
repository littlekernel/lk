//
// Copyright (c) 2021 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <arch/atomic.h>
#include <lk/init.h>
#include <lk/err.h>
#include <lk/cpp.h>
#include <lk/trace.h>
#include <lk/list.h>
#include <dev/bus/pci.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <lib/minip.h>
#include <lib/pktbuf.h>
#include <string.h>
#include <platform/interrupts.h>
#include <type_traits>

#include "e1000_hw.h"

#define LOCAL_TRACE 0

class e1000;
static e1000 *the_e; // XXX hack to remember the first e1000 seen and use for minip

// list of known 8086:x e1000 devices to match against
struct e1000_id_features {
    uint16_t id;
    bool e1000e;
};

const e1000_id_features e1000_ids[] = {
    { 0x100c, false }, // 82544GC QEMU 'e1000-82544gc'
    { 0x100e, false }, // 82540EM QEMU 'e1000'
    { 0x100f, false }, // 82545EM QEMU 'e1000-82544em'
    { 0x10d3, true }, // 82574L  QEMU 'e1000e'
    { 0x1533, true }, // i210
};

// i210 ids
// 0x1533
// 0x1536
// 0x1537
// 0x1538

// i219 ids
// 0x156f
// 0x1570
//    soc integrated versions?
// 0x1a1c // i219-LM (17)
// 0x1a1d // i219-V  (17)
// 0x1a1e // i219-LM (16)
// 0x1a1f // i219-V  (16)


class e1000 {
public:
    e1000();
    ~e1000();

    status_t init_device(pci_location_t loc, const e1000_id_features *id);

    int tx(pktbuf_t *p);

    bool is_e1000e() const { return id_feat_->e1000e; }

    const uint8_t *mac_addr() const { return mac_addr_; }

private:
    static const size_t rxring_len = 64;
    static const size_t txring_len = 64;
    static const size_t rxbuffer_len = 2048;

    uint32_t read_reg(e1000_reg reg);
    void write_reg(e1000_reg reg, uint32_t val);
    uint16_t read_eeprom(uint8_t offset);

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

    // rx ring
    rdesc *rxring_ = nullptr;
    uint32_t rx_last_head_ = 0;
    uint32_t rx_tail_ = 0;
    pktbuf_t *rx_pktbuf_[rxring_len] = {};
    uint8_t *rx_buf_ = nullptr; // rxbuffer_len * rxring_len byte buffer that rx_pktbuf[] points to

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

uint32_t e1000::read_reg(e1000_reg reg) {
    volatile uint32_t *r = (volatile uint32_t *)((uintptr_t)bar0_regs_ + (size_t)reg);

    return *r;
}

void e1000::write_reg(e1000_reg reg, uint32_t val) {
    volatile uint32_t *r = (volatile uint32_t *)((uintptr_t)bar0_regs_ + (size_t)reg);

    *r = val;
}

uint16_t e1000::read_eeprom(uint8_t offset) {
    // 8257x+ seems to have a different EERD layout
    uint32_t val;
    if (is_e1000e()) {
        write_reg(e1000_reg::EERD, (offset << 2) | 0x1); // data + start bit

        // spin while bit 1 (DONE) is clear
        while (((val = read_reg(e1000_reg::EERD)) & (1<<1)) == 0)
            ;
    } else {
        write_reg(e1000_reg::EERD, (offset << 8) | 0x1); // data + start bit

        // spin while bit 4 (DONE) is clear
        while (((val = read_reg(e1000_reg::EERD)) & (1<<4)) == 0)
            ;
    }
    return val >> 16;
}

volatile int e1000::global_count_ = 0;

e1000::e1000() = default;
e1000::~e1000() {
    // TODO: free resources
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

    if (icr & (1<<0)) { // TXDW - transmit descriptor written back
        PANIC_UNIMPLEMENTED;
    }
    if (icr & (1<<1)) { // TXQE - transmit queue empty
        //PANIC_UNIMPLEMENTED;
        // nothing to really do here
    }
    if (icr & (1<<6)) {
        printf("e1000: RX OVERRUN\n");
    }
    if (icr & (1<<7)) { // RXTO - rx timer interrupt
        // rx timer fired, packets are probably ready
        auto rdh = read_reg(e1000_reg::RDH);
        auto rdt = read_reg(e1000_reg::RDT);

        while (rx_last_head_ != rdh) {
            // copy the current rx descriptor locally for better cache performance
            rdesc rxd;
            copy(&rxd, rxring_ + rx_last_head_);

            LTRACEF("last_head %#x RDH %#x RDT %#x\n", rx_last_head_, rdh, rdt);
            if (LOCAL_TRACE) rxd.dump();

            // recover the pktbuf we queued in this spot
            DEBUG_ASSERT(rx_pktbuf_[rx_last_head_]);
            DEBUG_ASSERT(pktbuf_data_phys(rx_pktbuf_[rx_last_head_]) == rxd.addr);
            pktbuf_t *pkt = rx_pktbuf_[rx_last_head_];

            bool consumed_pkt = false;
            if (rxd.status & (1 << 0)) { // descriptor done, we own it now
                if (rxd.status & (1<<1)) { // end of packet
                    if (rxd.errors == 0) {
                        // good packet, trim data len according to the rx descriptor
                        pkt->dlen = rxd.length;
                        pkt->flags |= PKTBUF_FLAG_EOF; // just to make sure

                        // queue it in the rx queue
                        list_add_tail(&rx_queue_, &pkt->list);

                        // wake up the rx worker
                        event_signal(&rx_event_, false);
                        ret = INT_RESCHEDULE;
                        consumed_pkt = true;
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
            minip_rx_driver_callback(p);

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
    td.cmd = (1<<0); // end of packet (EOP)
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
    if (err != NO_ERROR) return err;

    LTRACEF("e1000 BARS:\n");
    if (LOCAL_TRACE) pci_dump_bars(bars, 6);

    if (!bars[0].valid || bars[0].addr == 0) {
        return ERR_NOT_FOUND;
    }

    // allocate a unit number
    unit_ = atomic_add(&global_count_, 1);

    // map bar 0, main memory mapped register interface, 128KB
    snprintf(str, sizeof(str), "e1000 %d bar0", unit_);
    err = vmm_alloc_physical(vmm_get_kernel_aspace(), str, 128*1024, &bar0_regs_, 0,
                             bars[0].addr, /* vmm_flags */ 0, ARCH_MMU_FLAG_UNCACHED_DEVICE);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }

    LTRACEF("bar 0 regs mapped to %p\n", bar0_regs_);

    pci_bus_mgr_enable_device(loc_);

    // read the mac address out of the eeprom
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

    printf("e1000 %d: mac address %02x:%02x:%02x:%02x:%02x:%02x\n", unit_, mac_addr_[0], mac_addr_[1], mac_addr_[2],
           mac_addr_[3], mac_addr_[4], mac_addr_[5]);

    // allocate and map space for the rx and tx ring
    snprintf(str, sizeof(str), "e1000 %d rxring", unit_);
    err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), str, rxring_len * sizeof(rdesc), (void **)&rxring_, 0, 0, ARCH_MMU_FLAG_UNCACHED);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }
    memset(rxring_, 0, rxring_len * sizeof(rdesc));

    paddr_t rxring_phys = vaddr_to_paddr(rxring_);
    LTRACEF("rx ring at %p, physical %#lx\n", rxring_, rxring_phys);

    snprintf(str, sizeof(str), "e1000 %d txring", unit_);
    err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), str, txring_len * sizeof(tdesc), (void **)&txring_, 0, 0, ARCH_MMU_FLAG_UNCACHED);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }
    memset(txring_, 0, txring_len * sizeof(rdesc));

    paddr_t txring_phys = vaddr_to_paddr(txring_);
    LTRACEF("tx ring at %p, physical %#lx\n", txring_, txring_phys);

    // allocate a large array of contiguous buffers to receive into
    snprintf(str, sizeof(str), "e1000 %d rx buffers", unit_);
    err = vmm_alloc_contiguous(vmm_get_kernel_aspace(), str, rxring_len * rxbuffer_len, (void **)&rx_buf_, 0, 0, 0);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }

    // mask all IRQs
    write_reg(e1000_reg::IMC, 0xffff);

    // qemus 82574 emulation seems to want IAME to be set to auto-clear ICR bits.
    if (is_e1000e()) {
        auto ctrl_ext = read_reg(e1000_reg::CTL_EXT);
        write_reg(e1000_reg::CTL_EXT, ctrl_ext | (1<<27)); // IAME - interrupt ack auto-mask
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
        e1000 *e = (e1000 *)arg;
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
    //hexdump(rxring_, rxring_len * sizeof(rdesc));

    // start rx worker thread
    auto wrapper_lambda = [](void *arg) -> int {
        e1000 *e = (e1000 *)arg;
        return e->rx_worker_routine();
    };
    snprintf(str, sizeof(str), "e1000 %d rx worker", unit_);
    rx_worker_thread_ = thread_create(str, wrapper_lambda, this, HIGH_PRIORITY, DEFAULT_STACK_SIZE);
    thread_resume(rx_worker_thread_);

    // start receiver
    // enable RX, unicast permiscuous, multicast permiscuous, broadcast accept, BSIZE 2048
    write_reg(e1000_reg::RCTL, (1<<1) | (1<<3) | (1<<4) | (1<<15) | (0<<16));

    // unmask receive irq
    auto ims = read_reg(e1000_reg::IMS);
    write_reg(e1000_reg::IMS, ims | (1<<7) | (1<<6)); // RXO, RXTO

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

    // enable the transmitter and appropriate irqs
    write_reg(e1000_reg::TCTL, (1<<3) | (1<<1)); // short packet pad, tx enable

    // unmask tx irq
    ims = read_reg(e1000_reg::IMS);
    write_reg(e1000_reg::IMS, ims | (1<<1) | (1<<0)); // transmit queue empty, tx descriptor write back

    return NO_ERROR;
}

extern "C"
status_t e1000_register_with_minip() {
    auto tx_routine = [](void *arg, pktbuf_t *p) {
        auto *e = static_cast<e1000 *>(arg);
        return e->tx(p);
    };

    if (the_e) {
        minip_set_eth(tx_routine, the_e, the_e->mac_addr());
        return NO_ERROR;
    }

    return ERR_NOT_FOUND;
}

static void e1000_init(uint level) {
    LTRACE_ENTRY;

    auto ac = lk::make_auto_call([]() { LTRACE_EXIT; });

    // probe pci to find a device
    for (auto id:  e1000_ids) {
        for (size_t i = 0; ; i++) {
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

            // XXX first e1000 found is remembered
            the_e = e;
        }
    }
}

LK_INIT_HOOK(e1000, &e1000_init, LK_INIT_LEVEL_PLATFORM + 1);
