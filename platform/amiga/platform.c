/*
 * Copyright (c) 2025-2026 Josh Cummings
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <kernel/debug.h>
#include <kernel/thread.h>
#include <lib/console.h>
#include <lib/heap.h>
#include <lk/bits.h>
#include <lk/err.h>
#include <lk/init.h>
#include <lk/main.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <platform.h>
#include <platform/display.h>
#include <platform/interrupts.h>
#include <stdbool.h>
#include <stdio.h>
#if WITH_KERNEL_VM
#include <kernel/vm.h>
#else
#include <kernel/novm.h>
#endif

#include "platform_p.h"

static volatile uint16_t *const paula_base = (void *)0xDFF000;

// IRQ handler array
static struct int_handlers {
    int_handler handler;
    void *arg;
} handlers[NUM_IRQS_TOTAL];

static uint8_t cia_a_irqs_enabled, cia_b_irqs_enabled;

static inline bool is_paula_irq(unsigned irq) {
    return irq >= 1 && irq <= NUM_IRQS_PAULA;
}

static inline bool is_valid_irq(unsigned irq) {
    return (irq >= 1 && irq <= NUM_IRQS_TOTAL);
}

static inline bool is_cia_a_irq(unsigned irq) {
    return (irq >= (NUM_IRQS_PAULA + 1) &&
            irq <= (NUM_IRQS_PAULA + NUM_IRQS_CIA));
}

static inline bool is_cia_b_irq(unsigned irq) {
    return (irq >= (NUM_IRQS_TOTAL - NUM_IRQS_CIA) && irq <= NUM_IRQS_TOTAL);
}

static void write_reg(unsigned int reg, uint32_t val) {
    paula_base[reg >> 1] = val;
}

static uint16_t read_reg(unsigned int reg) {
    return paula_base[reg >> 1];
}

status_t mask_interrupt(unsigned int irq) {
    if (!is_valid_irq(irq)) {
        return ERR_INVALID_ARGS;
    }

    if (is_paula_irq(irq)) {
        write_reg(INTENA, (uint16_t)(0x0000 | (1 << (irq - 1))));
        return NO_ERROR;
    }

    volatile uint8_t *cia_base;
    uint16_t cia_icr;
    uint8_t bit;

    if (is_cia_a_irq(irq)) {
        cia_base = (void *)CIA_A_BASE;
        cia_icr = CIA_A_ICR;
        bit = irq - (NUM_IRQS_PAULA + 1);
        cia_a_irqs_enabled &= ~(1 << bit);
    } else if (is_cia_b_irq(irq)) {
        cia_base = (void *)CIA_B_BASE;
        cia_icr = CIA_B_ICR;
        bit = irq - (NUM_IRQS_PAULA + NUM_IRQS_CIA + 1);
        cia_b_irqs_enabled &= ~(1 << bit);
    } else {
        return ERR_INVALID_ARGS;
    }

    cia_base[cia_icr] = (0x00 | (1 << bit));

    return NO_ERROR;
}

status_t unmask_interrupt(unsigned int irq) {
    if (!is_valid_irq(irq)) {
        return ERR_INVALID_ARGS;
    }

    if (is_paula_irq(irq)) {
        write_reg(INTENA, (uint16_t)(0x8000 | (1 << (irq - 1))));
        return NO_ERROR;
    }

    volatile uint8_t *cia_base;
    uint16_t cia_icr;
    uint8_t bit;

    if (is_cia_a_irq(irq)) {
        cia_base = (void *)CIA_A_BASE;
        cia_icr = CIA_A_ICR;
        bit = (irq - (NUM_IRQS_PAULA + 1));
        cia_a_irqs_enabled |= (1 << bit);
    } else if (is_cia_b_irq(irq)) {
        cia_base = (void *)CIA_B_BASE;
        cia_icr = CIA_B_ICR;
        bit = (irq - (NUM_IRQS_PAULA + NUM_IRQS_CIA)) - 1;
        cia_b_irqs_enabled |= (1 << bit);
    } else {
        return ERR_INVALID_ARGS;
    }

    cia_base[cia_icr] = (0x80 | (1 << bit));

    return NO_ERROR;
}

status_t clear_interrupt(unsigned int irq) {
    // TODO: Do we need CIA support here? Reading ICR should clear

    write_reg(INTREQ, (uint16_t)(1 << (irq - 1)));

    return NO_ERROR;
}

void register_int_handler(unsigned int vector, int_handler handler, void *arg) {
    if (vector >= 1 && vector <= NUM_IRQS_TOTAL) {
        handlers[vector - 1].handler = handler;
        handlers[vector - 1].arg = arg;
    }
}

enum handler_return m68k_platform_irq(uint8_t m68k_irq) {
    unsigned int level = m68k_irq - 24;

    // TODO: Possible cleanup here, make naming clearer
    uint16_t paula_pending = *(paula_base + (INTREQR >> 1));
    uint16_t paula_enabled = *(paula_base + (INTENAR >> 1));

    uint16_t paula_all = (paula_pending & paula_enabled);
    uint16_t paula_this_level = (paula_all & irq_level_map[level - 1]);
    uint32_t combined = (uint32_t)(paula_this_level & ~((1 << CIA_A_MUX_LEVEL) |
                                                        (1 << CIA_B_MUX_LEVEL)));

    uint16_t to_clear = 0;

    /*
     * Read from CIA ICRs only when corresponding Paula interrupts have fired (IRQ
     * 4 & 14). This read also causes CIA interrupt state and ICR to be cleared.
     *
     * Bit 7 (set/clear bit) of ICR indicates whether one of the IRQ bits are set.
     *
     * Bits 5 & 6 are unused.
     *
     */
    if (paula_this_level & (1 << CIA_A_MUX_LEVEL)) {
        volatile uint8_t *const cia_a_base = (void *)CIA_A_BASE;
        uint8_t icr = cia_a_base[CIA_A_ICR];
        uint8_t pending = icr & 0x1F;
        uint32_t enabled = pending & cia_a_irqs_enabled;

        if ((icr & 0x80) && enabled) {
            // We're only interested in the lower 5 bits
            combined |= (enabled << NUM_IRQS_PAULA);
        }
        to_clear |= (1 << CIA_A_MUX_LEVEL);
    }

    if (paula_this_level & (1 << CIA_B_MUX_LEVEL)) {
        volatile uint8_t *const cia_b_base = (void *)CIA_B_BASE;
        uint8_t icr = cia_b_base[CIA_B_ICR];
        uint8_t pending = icr & 0x1F;
        uint32_t enabled = pending & cia_b_irqs_enabled;

        if ((icr & 0x80) && enabled) {
            combined |= (enabled << (NUM_IRQS_PAULA + NUM_IRQS_CIA));
        }

        to_clear |= (1 << CIA_B_MUX_LEVEL);
    }

    THREAD_STATS_INC(interrupts);
    KEVLOG_IRQ_ENTER(m68k_irq);

    if (combined == 0) {
        if (to_clear) {
            write_reg(INTREQ, to_clear);
        }
        return INT_NO_RESCHEDULE;
    }

    enum handler_return ret = INT_NO_RESCHEDULE;

    while (combined) {
        int irq_bit = ctz(combined);
        combined &= ~(1 << irq_bit); // Clear IRQ bit

        if (handlers[irq_bit].handler) {
            ret = handlers[irq_bit].handler(handlers[irq_bit].arg);
        }
    }

    write_reg(INTREQ, to_clear);

    KEVLOG_IRQ_EXIT(m68k_irq);

    return ret;
}

void platform_early_init(void) {
    // Start with a clean interrupt slate, we'll selectively enable/unmask as needed. 
    // Disable and clear all Paula interrupts initially.
    write_reg(INTENA, 0x7FFF);
    write_reg(INTREQ, 0x7FFF);

    // Enable Paula master interrupt bit
    write_reg(INTENA, 0xC000);

    // Enable Paula 'EXTER' interrupts, needed for CIA-B timers
    unmask_interrupt(INTERRUPT_EXTER);

    // Chip mem size is passed in by the bootloader. novm automatically 
    // moves an arena to avoid overlapping with the kernel itself.
    novm_add_arena("mem", MEMBASE, lk_boot_args[0]);

    platform_init_display();
    platform_serial_init();
    cia_timer_init();
}

void platform_dputc(char c) {
    if (c == '\n') {
        platform_dputc('\r');
    }

    uart_putc(c); // TODO: is it possible to have multiple consoles?
}

int platform_dgetc(char *c, bool wait) {
    return cbuf_read_char(&console_input_cbuf, c, wait);
}

void platform_init(void) {
    platform_keyboard_init(&console_input_cbuf);
}
