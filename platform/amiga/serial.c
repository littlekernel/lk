/*
 * Copyright (c) 2025-2026 Josh Cummings
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include "platform_p.h"

#include <kernel/event.h>
#include <kernel/spinlock.h>
#include <kernel/thread.h>
#include <lib/cbuf.h>
#include <platform.h>
#include <platform/interrupts.h>
#include <stdbool.h>
#include <stdint.h>

#define TBE_STATUS (1 << 13)
#define RBF_STATUS (1 << 14)

#define RXBUF_SIZE 32

static event_t tx_ev;
static thread_t *tx_thread = NULL;
static spin_lock_t tx_lock = SPIN_LOCK_INITIAL_VALUE;

static volatile bool tx_active = false;
static volatile bool tx_kick_pending = false;
static volatile bool tx_writer_started = false;

static cbuf_t rx_buf;
static cbuf_t tx_buf;
static char rx_buf_data[RXBUF_SIZE];

// Serial register offsets, relative to Paula base register
// TODO: Move this somewhere else? (header?)
#define SERDAT  0x030
#define SERDATR 0x018
#define SERPER  0x032

static volatile uint16_t *serial_reg = (volatile uint16_t *)0xDFF000;

static void write_reg(unsigned int reg, uint32_t val) {
    serial_reg[reg >> 1] = (uint16_t)val;
}

static uint16_t read_reg(unsigned int reg) {
    return serial_reg[reg >> 1];
}

// ACK and wake thread/event
static enum handler_return uart_irq_tx_handler(void *arg) {
    clear_interrupt(INTERRUPT_TBE);
    event_signal(&tx_ev, false);
    return INT_NO_RESCHEDULE;
}

static enum handler_return uart_irq_rx_handler(void *arg) {
    bool resched = false;

    uint16_t reg = read_reg(SERDATR);
    clear_interrupt(INTERRUPT_RBF);

    if ((reg & RBF_STATUS) > 0) {
        char c = reg & 0xFF;

#if CONSOLE_HAS_INPUT_BUFFER
        cbuf_t *target_buf = &console_input_cbuf;
#else
        cbuf_t *target_buf = &rx_buf;
#endif

        cbuf_write_char(target_buf, c, false);
        resched = true;
    }

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

void platform_serial_init(void) {
    cbuf_initialize(&tx_buf, 256);
    cbuf_initialize_etc(&rx_buf, RXBUF_SIZE, rx_buf_data);

    register_int_handler(INTERRUPT_TBE, uart_irq_tx_handler, NULL);
    register_int_handler(INTERRUPT_RBF, uart_irq_rx_handler, NULL);
    unmask_interrupt(INTERRUPT_RBF);

    event_init(&tx_ev, false, EVENT_FLAG_AUTOUNSIGNAL);
}

// Drain all queued bytes, mask TX IRQ when empty
static int uart_write_thread(void *arg) {
    for (;;) {
        event_wait(&tx_ev);

        for (;;) {
            // Retrieve bytes from cbuf, mask and idle if none
            char c;
            spin_lock(&tx_lock);
            if (cbuf_read_char(&tx_buf, &c, false) != 1) {
                tx_kick_pending = false;
                tx_active = false;
                mask_interrupt(INTERRUPT_TBE); // Prevent TBE storm
                spin_unlock(&tx_lock);
                break;
            }

            tx_active = true;
            spin_unlock(&tx_lock);

            // Wait until transmitter is ready
            while ((read_reg(SERDATR) & TBE_STATUS) == 0) {
            }
            write_reg(SERDAT, c | 0x200);

            // Keep interrupt enabled while there's data
            unmask_interrupt(INTERRUPT_TBE);
        }
    }

    return 0;
}

void uart_putc(char c) {
    bool irqs_disabled = arch_ints_disabled();

    if (!tx_writer_started && !irqs_disabled) {
        tx_thread = thread_create("[uart writer]", uart_write_thread, NULL,
                                  DEFAULT_PRIORITY, DEFAULT_STACK_SIZE);
        thread_resume(tx_thread);
        tx_writer_started = true;
    }

    spin_lock(&tx_lock);

    // Drain queue -> Send current byte -> Drain again -> mask TX
    if (irqs_disabled) {
        char q;

        // Drain any queued bytes first
        while (cbuf_read_char(&tx_buf, &q, false) == 1) {
            while ((read_reg(SERDATR) & TBE_STATUS) == 0) {
            }
            write_reg(SERDAT, q | 0x200);
        }

        while ((read_reg(SERDATR) & TBE_STATUS) == 0) {
        }
        write_reg(SERDAT, c | 0x200);

        // Drain anything that came along in the meantime
        while (cbuf_read_char(&tx_buf, &q, false) == 1) {
            while ((read_reg(SERDATR) & TBE_STATUS) == 0) {
            }
            write_reg(SERDAT, q | 0x200);
        }

        // Queue should be empty now...
        tx_active = false;
        mask_interrupt(INTERRUPT_TBE);

        spin_unlock(&tx_lock);

        return;
    }

    /* normal path: enqueue and let worker/IRQ drain */
    size_t written = cbuf_write_char(&tx_buf, c, false);

    // Arm TX and wake writer thread
    if (!tx_active) {
        tx_active = true;
        unmask_interrupt(INTERRUPT_TBE);
        event_signal(&tx_ev, false);
    }

    // Handle buffer being full, write when we can
    while (written == 0) {
        spin_unlock(&tx_lock);
        event_signal(&tx_ev, false);
        spin_lock(&tx_lock);
        written = cbuf_write_char(&tx_buf, c, false);
    }

    spin_unlock(&tx_lock);
}

int uart_getc(char *c, bool wait) {
    return cbuf_read_char(&rx_buf, c, wait);
}

// TODO: Implement panic stuff properly
int platform_pgetc(char *c, bool wait) {
    uint16_t reg = read_reg(SERDATR);

    if ((reg & (1 << 14)) > 0) {
        *c = reg & 0xFF;
        return 0;
    }

    return -1;
}

void platform_pputc(char c) {
    uart_putc(c);
}
