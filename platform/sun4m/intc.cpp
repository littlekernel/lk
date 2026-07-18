//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/trace.h>
#include <platform/interrupts.h>
#include <sys/types.h>

#include "platform_p.h"

#define LOCAL_TRACE 0

// slavio interrupt controller

// Table of interrupt sources taken from qemu emulation of sun4m:
// Master bit refers to the bit in the INTC_GLOBAL_PEN_REG.
// PIL is the processor interrupt priority level.
// Master Bit      | CPU PIL        | Mapped Hardware Device in QEMU
// ----------------|----------------|------------------------------------------------------------
// 0               | PIL 2          | NVRAM / Real-Time Clock (sysbus-m48t08)
// 1               | PIL 3          | Unused
// 2               | PIL 5          | Unused
// 3               | PIL 7          | Unused
// 4               | PIL 9          | Unused
// 5               | PIL 11         | Audio Codec (sun-CS4231)
// 6               | PIL 13         | Unused
// 7               | PIL 2          | Unused
// 8               | PIL 3          | Unused
// 9               | PIL 5          | Unused
// 10              | PIL 7          | Unused
// 11              | PIL 9          | Framebuffer Graphic Card (cg3 or tcx)
// 12              | PIL 11         | Unused
// 13              | PIL 13         | Unused
// 14              | PIL 12         | Keyboard & Mouse (ESCC Serial Ports OR'd)
// 15              | PIL 12         | Serial Console (ESCC Serial Ports OR'd)
// 16              | PIL 6          | Ethernet Controller (le / LANCE DMA)
// 17              | PIL 13         | Unused
// 18              | PIL 4          | SCSI Controller (esp / ESP DMA)
// 19              | PIL 10         | System Timer / Counter
// 20              | PIL 8          | Unused
// 21              | PIL 9          | Unused
// 22              | PIL 11         | Floppy Disk Controller (sun4m_fdctrl)
// 23–26           | PIL 0          | Unused / Reserved
// 27              | PIL 15         | Unused / Reserved
// 28              | PIL 15         | ECC Memory Controller Error (eccmemctl)
// 29              | PIL 15         | Unused / Reserved
// 30              | PIL 15         | IOMMU Error & SLAVIO Miscellaneous System Controller Error
// 31              | PIL 0          | Unused / Reserved

// Bit 31 in the INTC_GLOBAL_PEN_REG is the master enable bit,
// which must be set to 1 to allow any interrupts to be delivered to the CPU.

namespace {

// each cpu offset is 0x1000, only handle UP for now
constexpr uint64_t INTC_PERCPU_PHYS = 0xff1400000ULL;
constexpr uint64_t INTC_GLOBAL_PHYS = 0xff1410000ULL;
constexpr uint32_t INTC_NUM_IRQS = 32;

struct int_handler_struct {
    int_handler handler;
    void *arg;
};

int_handler_struct int_handler_table[INTC_NUM_IRQS] = {};
SpinLock lock;

enum : uint8_t {
    INTC_PERCPU_PEND_REG = 0,
    INTC_PERCPU_CLR_PEND = 4,
    INTC_PERCPU_SET_SOFT_INT = 8,
};

enum : uint8_t {
    INTC_GLOBAL_PEN_REG = 0,
    INTC_GLOBAL_TARGET_MASK_REG = 4,
    INTC_GLOBAL_TARGET_CLR_REG = 8,
    INTC_GLOBAL_TARGET_SET_REG = 0xc,
    INTC_GLOBAL_TARGET_REG = 0x10,
};

void intc_dump() {
    printf("PERCPU PEND REG: 0x%x\n",
           sparc_read_physical_32(INTC_PERCPU_PHYS + INTC_PERCPU_PEND_REG));
    printf("PERCPU CLR PEND: 0x%x\n",
           sparc_read_physical_32(INTC_PERCPU_PHYS + INTC_PERCPU_CLR_PEND));
    printf("PERCPU SET SOFT INT: 0x%x\n",
           sparc_read_physical_32(INTC_PERCPU_PHYS + INTC_PERCPU_SET_SOFT_INT));
    printf("GLOBAL PEN REG: 0x%x\n",
           sparc_read_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_PEN_REG));
    printf("GLOBAL TARGET MASK REG: 0x%x\n",
           sparc_read_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_MASK_REG));
    printf("GLOBAL TARGET REG: 0x%x\n",
           sparc_read_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_REG));
}

} // namespace

void sun4m_intc_early_init() {
    // intc_dump();

    // start with all interrupts masked, and the master enable bit set
    sparc_write_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_SET_REG, 0x7fffffff);

    // cpu 0 gets all of the external irqs
    sparc_write_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_REG, 0);
}

void sun4m_intc_init() {}

status_t mask_interrupt(unsigned int vector) {
    LTRACEF("vector %u\n", vector);
    if (vector >= INTC_NUM_IRQS) {
        return ERR_INVALID_ARGS;
    }

    AutoSpinLock guard(&lock);
    sparc_write_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_SET_REG, 1u << vector);

    if (LOCAL_TRACE) {
        intc_dump();
    }

    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector) {
    LTRACEF("vector %u\n", vector);
    if (vector >= INTC_NUM_IRQS) {
        return ERR_INVALID_ARGS;
    }

    AutoSpinLock guard(&lock);
    sparc_write_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_CLR_REG, 1u << vector);

    if (LOCAL_TRACE) {
        intc_dump();
    }

    return NO_ERROR;
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg) {
    LTRACEF("vector %u, handler %p, arg %p\n", vector, handler, arg);
    if (vector >= INTC_NUM_IRQS) {
        panic("register_int_handler: vector out of range %u\n", vector);
    }

    AutoSpinLock guard(&lock);
    int_handler_table[vector].handler = handler;
    int_handler_table[vector].arg = arg;
}

extern "C" enum handler_return platform_irq(uint32_t irq) {
    LTRACEF("PIL %u\n", irq);
    // intc_dump();

    THREAD_STATS_INC(interrupts);

    // If we came in at PIL 14 it's a per cpu timer interrupt, handle it directly.
    if (irq == SUN4M_TIMER_PERCPU_IRQ_LEVEL) {
        return sun4m_timer_irq(nullptr);
    }

    uint32_t pending = sparc_read_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_PEN_REG);
    uint32_t mask = sparc_read_physical_32(INTC_GLOBAL_PHYS + INTC_GLOBAL_TARGET_MASK_REG);

    LTRACEF_LEVEL(2, "pending pre mask: 0x%x\n", pending);
    pending &= ~mask;

    LTRACEF_LEVEL(2, "pending post mask: 0x%x\n", pending);
    handler_return ret = INT_NO_RESCHEDULE;
    while (pending != 0) {
        uint32_t vector = static_cast<uint32_t>(__builtin_ctz(pending));
        pending &= ~(1u << vector);

        if (likely(int_handler_table[vector].handler)) {
            if (int_handler_table[vector].handler(int_handler_table[vector].arg) ==
                INT_RESCHEDULE) {
                ret = INT_RESCHEDULE;
            }
        } else {
            TRACEF("unhandled vector %u\n", vector);
        }
    }

    return ret;
}
