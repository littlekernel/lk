/*
 * Copyright (c) 2018 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <dev/interrupt/riscv_plic.h>

#include <assert.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>

// Driver for simplic PLIC implementations for various RISC-V machines.

#define LOCAL_TRACE 0

// Preallocate space for up to 256 vectors.
// If more are needed will need to bump this up or switch to a dynamic scheme.
#define MAX_IRQS 256
static struct int_handlers {
    int_handler handler;
    void *arg;
} handlers[MAX_IRQS];

static uintptr_t plic_base_virt = 0;
static size_t num_irqs = 0;
static bool hart0_m_only = false;

#define PLIC_PRIORITY(irq)          (plic_base_virt + 4 * (irq))
#define PLIC_PENDING(irq)           (plic_base_virt + 0x1000 + (4 * ((irq) / 32)))
#define PLIC_ENABLE(irq, hart)      (plic_base_virt + 0x2000 + (0x80 * plic_hart_index(hart)) + (4 * ((irq) / 32)))
#define PLIC_THRESHOLD(hart)        (plic_base_virt + 0x200000 + (0x1000 * plic_hart_index(hart)))
#define PLIC_COMPLETE(hart)         (plic_base_virt + 0x200004 + (0x1000 * plic_hart_index(hart)))
#define PLIC_CLAIM(hart)            PLIC_COMPLETE(hart)

// Mapping of HART to interrupt target is annoyingly complex. Switch between two modes
// based on the hart0_m_only bool:
//
// On the JH7110 (like other sifive socs) the first HART only has one mode, machine
// and the subsequent harts have both machine and supervisor. The interrupt targets
// are thus indexed:
// HART 0 machine mode = 0
// HART 1 machine mode = 1
// HART 1 supervisor mode = 2
// HART 2 machine mode = 3
// HART 2 supervisor mode = 4
// ...
//
// On flatter designs, such as qemu's 'virt' machine, all harts are equal and 0 indexed,
// so the mapping is simpler:
// HART 0 machine mode = 0
// HART 0 supervisor mode = 1
// HART 1 machine mode = 2
// HART 1 supervisor mode = 3
// HART 2 machine mode = 4
// HART 2 supervisor mode = 5
// ...
//
// This routine maps harts to the current mode's interrupt target
static unsigned int plic_hart_index(unsigned int hart) {
    unsigned int index;
    if (hart0_m_only) {
#if RISCV_M_MODE
        index = (hart == 0) ? 0 : (2 * hart - 1);
#elif RISCV_S_MODE
        DEBUG_ASSERT(hart != 0);
        index = 2 * hart;
#else
#error undefined
#endif
    } else {
#if RISCV_M_MODE
        index = 2 * hart;
#elif RISCV_S_MODE
        index = 2 * hart + 1;
#else
#error undefined
#endif
    }
    return index;
}

void plic_early_init(uintptr_t base, size_t num_irqs_, bool hart0_m_only_) {
    plic_base_virt = base;
    DEBUG_ASSERT(num_irqs_ <= MAX_IRQS);
    num_irqs = num_irqs_;
    hart0_m_only = hart0_m_only_;

    // mask all irqs and set their priority to 1
    // TODO: mask on all the other cpus too
    for (size_t i = 1; i < num_irqs; i++) {
        *REG32(PLIC_ENABLE(i, riscv_current_hart())) &= ~(1 << (i % 32));
        *REG32(PLIC_PRIORITY(i)) = 1;
    }

    // set global priority threshold to 0
    *REG32(PLIC_THRESHOLD(riscv_current_hart())) = 0;
}

void plic_init(void) {}

status_t mask_interrupt(unsigned int vector) {
    LTRACEF("vector %u, current hart %u\n", vector, riscv_current_hart());
    *REG32(PLIC_ENABLE(vector, riscv_current_hart())) &= ~(1 << (vector % 32));
    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int vector) {
    LTRACEF("vector %u, current hart %u\n", vector, riscv_current_hart());
    *REG32(PLIC_ENABLE(vector, riscv_current_hart())) |= (1 << (vector % 32));

    return NO_ERROR;
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg) {
    LTRACEF("vector %u handler %p arg %p, hart %u\n", vector, handler, arg, riscv_current_hart());

    DEBUG_ASSERT(vector < num_irqs);

    handlers[vector].handler = handler;
    handlers[vector].arg = arg;
}

void register_int_handler_msi(unsigned int vector, int_handler handler, void *arg, bool edge) {
    PANIC_UNIMPLEMENTED;
}

enum handler_return riscv_platform_irq(void) {
    // see what irq triggered it
    uint32_t vector = *REG32(PLIC_CLAIM(riscv_current_hart()));
    LTRACEF("vector %u\n", vector);

    if (unlikely(vector == 0)) {
        // nothing pending
        return INT_NO_RESCHEDULE;
    }

    THREAD_STATS_INC(interrupts);
    KEVLOG_IRQ_ENTER(vector);

    enum handler_return ret = INT_NO_RESCHEDULE;
    if (handlers[vector].handler) {
        ret = handlers[vector].handler(handlers[vector].arg);
    }

    // ack the interrupt
    *REG32(PLIC_COMPLETE(riscv_current_hart())) = vector;

    KEVLOG_IRQ_EXIT(vector);

    return ret;
}

