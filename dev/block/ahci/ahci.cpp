//
// Copyright (c) 2022 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
// NOTE ON MEMORY ATTRIBUTES AND BARRIERS
//
// The command list / FIS / command table region this driver shares with the controller is
// mapped ARCH_MMU_FLAG_UNCACHED_DEVICE (see ahci_port::init()), and the driver has no memory
// barriers: it relies on that mapping to order its descriptor writes against the register
// writes that hand a command to the controller.
//
// That works on x86, which is where this driver is used in practice: uncached accesses stay
// coherent with the caches, and KVM forces guest memory to write-back regardless of what the
// guest asks for.
//
// It does not generalize. The driver is reachable on any PCI capable target, and on arm64
// under qemu it already binds and works under TCG but takes a data abort under KVM, because on
// a core without FEAT_S2FWB (ARMv8.4) the guest's uncached accesses and the VMM's cached view
// of the same page never see each other. A genuinely non-coherent DMA architecture would break
// it the other way around.
//
// Making this driver work on either would mean giving it the same treatment as
// virtio_device::virtio_alloc_ring() and dev/net/e1000: map the shared region cached and add
// explicit wmb()/rmb() around the doorbell and completion registers, plus real cache
// maintenance if the platform is not coherent. Nobody has needed that yet, so it has not been
// done.

#include "ahci.h"

#include <arch/atomic.h>
#include <dev/bus/pci.h>
#include <kernel/event.h>
#include <kernel/thread.h>
#include <kernel/vm.h>
#include <lk/bits.h>
#include <lk/cpp.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/list.h>
#include <lk/trace.h>
#include <platform/interrupts.h>
#include <string.h>

#include "ahci_hw.h"
#include "disk.h"
#include "port.h"

#define LOCAL_TRACE 0

int ahci::global_count_ = 0;

constexpr bool use_threaded_probe = false;

status_t ahci::init_device(pci_location_t loc) {
    char str[32];
    loc_ = loc;

    LTRACEF("pci location %s\n", pci_loc_string(loc_, str));

    pci_bar_t bars[6];
    status_t err = pci_bus_mgr_read_bars(loc_, bars);
    if (err != NO_ERROR) {
        return err;
    }

    LTRACEF("ahci BARS:\n");
    if (LOCAL_TRACE) {
        pci_dump_bars(bars, 6);
    }

    if (!bars[5].valid || !bars[5].addr) {
        return ERR_NOT_FOUND;
    }

    // allocate a unit number
    unit_ = atomic_add(&global_count_, 1);
    dprintf(INFO, "ahci%d: found at %s\n", unit_, pci_loc_string(loc_, str));

    // map bar 5, main memory mapped register interface, 4K
    snprintf(str, sizeof(str), "ahci%d abar", unit_);
    void *regs;
    err = vmm_alloc_physical(vmm_get_kernel_aspace(), str, PAGE_ALIGN(bars[5].size), &regs, 0,
                             bars[5].addr, /* vmm_flags */ 0,
                             ARCH_MMU_FLAG_PERM_READ | ARCH_MMU_FLAG_PERM_WRITE | ARCH_MMU_FLAG_UNCACHED_DEVICE);
    if (err != NO_ERROR) {
        return ERR_NOT_FOUND;
    }
    abar_regs_ = reinterpret_cast<volatile uint32_t *>(regs);

    LTRACEF("ABAR mapped to %p\n", abar_regs_);

    pci_bus_mgr_enable_device(loc_);

    capabilities_ = read_reg(ahci_reg::CAP);
    LTRACEF("CAP %#x\n", capabilities_);
    LTRACEF("PI %#x\n", read_reg(ahci_reg::PI));

    // mask all irqs
    write_reg(ahci_reg::GHC, read_reg(ahci_reg::GHC) & ~(1U << 1)); // clear GHC.IE

    static auto irq_handler_wrapper = [](void *arg) -> handler_return {
        ahci *a = static_cast<ahci *>(arg);
        return a->irq_handler();
    };

    // allocate an MSI interrupt
    uint irq_base;
    err = pci_bus_mgr_allocate_msi(loc_, 1, &irq_base);
    if (err != NO_ERROR) {
        // fall back to regular IRQs
        err = pci_bus_mgr_allocate_irq(loc_, &irq_base);
        if (err != NO_ERROR) {
            printf("ahci: unable to allocate IRQ\n");
            return err;
        }
        register_int_handler(irq_base, irq_handler_wrapper, this);
        LTRACEF("IRQ number %#x\n", irq_base);
    } else {
        register_int_handler_msi(irq_base, irq_handler_wrapper, this, true);
        LTRACEF("MSI IRQ number %#x\n", irq_base);
    }

    unmask_interrupt(irq_base);

    // clear any pending interrupts
    LTRACEF("IS %#x\n", read_reg(ahci_reg::IS));
    write_reg(ahci_reg::IS, read_reg(ahci_reg::IS));

    // enable interrupts
    write_reg(ahci_reg::GHC, read_reg(ahci_reg::GHC) | (1U << 1)); // set GHC.IE

    LTRACEF("GHC %#x\n", read_reg(ahci_reg::GHC));
    LTRACEF("BOHC %#x\n", read_reg(ahci_reg::BOHC));

    // probe every port marked implemented
    uint32_t port_bitmap = read_reg(ahci_reg::PI);
    num_ports_ = BITS(capabilities_, 4, 0) + 1;
    for (size_t port = 0; port < num_ports_; port++) {
        if ((port_bitmap & (1U << port)) == 0) {
            // skip port not implemented
            break;
        }

        ports_[port] = new ahci_port(*this, port);
        auto *p = ports_[port];

        ahci_disk *disk = nullptr;
        err = p->probe(&disk);
        if (err != NO_ERROR) {
            continue;
        }

        DEBUG_ASSERT(disk);

        // add the disk to a list for further processing
        disk_list_.push_back(disk);
    }

    return NO_ERROR;
}

void ahci::disk_probe_worker() {
    LTRACE_ENTRY;

    for (ahci_disk &disk : disk_list_) {
        disk.identify();
    }

    LTRACE_EXIT;
}

status_t ahci::start_disk_probe() {
    if (disk_list_.is_empty()) {
        return NO_ERROR;
    }

    auto probe_worker = [](void *arg) -> int {
        ahci *a = static_cast<ahci *>(arg);

        a->disk_probe_worker();

        return 0;
    };

    if (use_threaded_probe) {
        thread_t *disk_probe_thread = thread_create("ahci disk probe", probe_worker, this, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        thread_detach_and_resume(disk_probe_thread);
    } else {
        probe_worker(this);
    }

    return NO_ERROR;
}

handler_return ahci::irq_handler() {
    LTRACE_ENTRY;

    const auto orig_is = read_reg(ahci_reg::IS);
    auto is = orig_is;
    LTRACEF("is %#x\n", is);

    // cycle through the ports that have interrupts queued
    handler_return ret = INT_NO_RESCHEDULE;
    while (is != 0) {
        int port = __builtin_ctz(is);

        LTRACEF("interrupt on port %d\n", port);

        DEBUG_ASSERT(ports_[port] != nullptr);

        if (ports_[port]->irq_handler() == INT_RESCHEDULE) {
            ret = INT_RESCHEDULE;
        }

        is &= ~(1U << port);
    }

    // clear the interrupt status
    write_reg(ahci_reg::IS, orig_is);

    LTRACE_EXIT;

    return ret;
}

namespace {

// hook called at init time to iterate through pci bus and find all of the ahci devices
void ahci_init(uint level) {
    LTRACE_ENTRY;

    // probe pci to find a device
    for (size_t i = 0;; i++) {
        // look for AHCI controllers by class
        pci_location_t loc;
        status_t err = pci_bus_mgr_find_device_by_class(&loc, 0x1, 0x6, 0x1, i);
        if (err != NO_ERROR) {
            break;
        }

        // we maybe found one, create a new device and initialize it
        auto a = new ahci;
        err = a->init_device(loc);
        if (err != NO_ERROR) {
            char str[14];
            printf("ahci: device at %s failed to initialize\n", pci_loc_string(loc, str));
            delete a;
            continue;
        }

        // set up any disks we've found
        a->start_disk_probe();
    }
}
LK_INIT_HOOK(ahci, &ahci_init, LK_INIT_LEVEL_PLATFORM + 1);

} // namespace
