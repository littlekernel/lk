/*
 * Copyright (c) 2021 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include "platform_p.h"

#include <assert.h>
#include <lk/err.h>
#include <lk/debug.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <lib/cbuf.h>
#include <kernel/debug.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/rosco-m68k.h>
#include <platform/timer.h>
#include <platform.h>

#define LOCAL_TRACE 0

// driver for a 68c681 acting as a dual uart and a system timer and a few gpios

// ticks in units of ms
#define TICK_HZ (100U)
#define TICK_MS (1000U / TICK_HZ)
STATIC_ASSERT(TICK_HZ * TICK_MS == 1000U);
static volatile uint32_t ticks;

// periodic timer callback stuff
static platform_timer_callback t_callback;
static void *t_arg;
static lk_time_t t_next_periodic_tick;
static lk_time_t t_periodic_interval;

// uart stuff
#define RXBUF_SIZE 128
static char uart_rx_buf_data[RXBUF_SIZE];
static cbuf_t uart_rx_buf;

static volatile uint8_t * const DUART_BASE = (void *)0xf00001;

// registers, swizzled according to
// https://github.com/rosco-m68k/rosco_m68k/blob/develop/code/shared/rosco_m68k_public.asm
enum {
    DUART_REG_MR1A_RW   = 0x00, // mode 1 register, channel A
    DUART_REG_MR2A_RW   = 0x00, // mode 2 register, channel A
    DUART_REG_SRA_R     = 0x01, // status register, channel A
    DUART_REG_CSRA_W    = 0x01, // clock select register, channel A
    DUART_REG_MISR_R    = 0x02, // masked interrupt status register
    DUART_REG_CRA_W     = 0x02, // command register, channel A
    DUART_REG_RHRA_R    = 0x03, // rx holding register, channel A
    DUART_REG_THRA_W    = 0x03, // tx holding register, channel A
    DUART_REG_IPCR_R    = 0x04, // input port config register
    DUART_REG_ACR_W     = 0x04, // auxillary control register
    DUART_REG_ISR_R     = 0x05, // interrupt status register
    DUART_REG_IMR_W     = 0x05, // interrupt mask register
    DUART_REG_CTU_RW    = 0x06, // counter timer, upper byte
    DUART_REG_CTL_RW    = 0x07, // counter timer, lower byte

    DUART_REG_MR1B_RW   = 0x08, // mode register, channel B
    DUART_REG_SRB_R     = 0x09, // status register, channel B
    DUART_REG_CSRB_W    = 0x09, // clock select register, channel B
    DUART_REG_CRB_W     = 0x0a, // command register, channel B
    DUART_REG_RHRB_R    = 0x0b, // rx holding register, channel B
    DUART_REG_THRB_W    = 0x0b, // tx holding register, channel B

    DUART_REG_IVR_RW    = 0x0c, // interrupt vector register
    DUART_REG_IP_R      = 0x0d, // input port
    DUART_REG_OPCR_W    = 0x0d, // output port configuration register

    DUART_REG_SCC_R     = 0x0e, // start counter/timer command
    DUART_REG_STC_R     = 0x0f, // stop counter/timer command

    DUART_REG_SOPBC_W   = 0x0e, // set output port bits command
    DUART_REG_COPBC_W   = 0x0f, // clear output port bits command
};

// save a copy of whatever IMR was set to before
static uint8_t cached_imr;

static void write_reg(uint reg, uint8_t val) {
    DUART_BASE[reg * 2] = val;
}

static uint8_t read_reg(uint reg) {
    return DUART_BASE[reg * 2];
}

void duart_early_init(void) {
    // clear all IRQs
    cached_imr = 0;
    write_reg(DUART_REG_IMR_W, cached_imr);

    // Set the IRQ vector to 0x45
    write_reg(DUART_REG_IVR_RW, 0x45);

    // set the mode
    // !rxrts, rxrdy int, char error mode, n81
    write_reg(DUART_REG_MR1A_RW, (0b10 << 3) | (0b11 << 0));
    // normal channel, no txrts, no cts, stop bit 1
    write_reg(DUART_REG_MR2A_RW, 0x7);

    // set up a periodic counter at TICK_HZ
    read_reg(DUART_REG_STC_R); // stop the counter

    // compute the counter
    uint16_t count = 3686400 / 2 / TICK_HZ;
    write_reg(DUART_REG_CTL_RW, count & 0xff);
    write_reg(DUART_REG_CTU_RW, (count >> 8) & 0xff);

    // set timer mode
    write_reg(DUART_REG_ACR_W, (0b110 << 4)); // timer mode, X1/CLK

    // start timer
    read_reg(DUART_REG_SCC_R); // start counter

    // unmask irq
    cached_imr = (1<<3); // counter #1 ready
    write_reg(DUART_REG_IMR_W, cached_imr);

    // configure the output ports
    write_reg(DUART_REG_OPCR_W, 0);
}

void duart_init(void) {
    // finish uart init to get rx going
    cbuf_initialize_etc(&uart_rx_buf, RXBUF_SIZE, uart_rx_buf_data);

    // enable uart RX irq
    cached_imr |= (1<<1); // RXRDY/FFULLA
    write_reg(DUART_REG_IMR_W, cached_imr);
}

status_t platform_set_periodic_timer(platform_timer_callback callback, void *arg, lk_time_t interval) {
    LTRACEF("cb %p, arg %p, interval %u\n", callback, arg, interval);
    t_callback = callback;
    t_arg = arg;
    t_periodic_interval = interval;
    t_next_periodic_tick = current_time() + interval;

    return NO_ERROR;
}

enum handler_return duart_irq(void) {
    enum handler_return ret = INT_NO_RESCHEDULE;

    uint8_t isr = read_reg(DUART_REG_ISR_R);

    LTRACEF_LEVEL(2, "isr %#hhx\n", isr);

    if (likely(isr & (1<<3))) { // counter #1 ready
        ticks += TICK_MS;

        // ack the timer hardware
        read_reg(DUART_REG_STC_R);

        // call back the registered timer
        if (likely(t_callback)) {
            lk_time_t now = current_time();
            if (unlikely(now >= t_next_periodic_tick)) {
                ret = t_callback(t_arg, now);
                t_next_periodic_tick += t_periodic_interval;
            }
        }
    }
    if (isr & (1<<1)) { // RXRDY/FFULLA
        uint8_t status = read_reg(DUART_REG_SRA_R);
        LTRACEF("RXRDY/FFULLA, status %#hhx\n", status);
        if (status & (1<<0)) { // RXRDY
            // XXX disable, for some reason the top 3 status bits are always set
#if 0
            if (unlikely(status & (0b111 << 5))) { // any of break, framing, or parity error
                // consume this byte
                __UNUSED volatile uint8_t hole = read_reg(DUART_REG_RHRA_R);
            } else
#endif
            {
                char c = read_reg(DUART_REG_RHRA_R);
                cbuf_write_char(&uart_rx_buf, c, false);
                ret = INT_RESCHEDULE;
            }
        }
    }

    return ret;
}

void platform_dputc(char c) {
    if (c == '\n') {
        platform_dputc('\r');
    }

    // spin while TXRDY is clear
    while ((read_reg(DUART_REG_SRA_R) & (1 << 2)) == 0) // TXRDY
        ;
    write_reg(DUART_REG_THRA_W, c);
}

int platform_dgetc(char *c, bool wait) {
    return cbuf_read_char(&uart_rx_buf, c, wait);
}

int platform_pgetc(char *c, bool wait) {
    for (;;) {
        uint8_t status = read_reg(DUART_REG_SRA_R);
        if (status & (1<<0)) { // RXRDY
            // XXX disable, for some reason the top 3 status bits are always set
#if 0
            if (status & (0b111 << 5)) { // any of break, framing, or parity error
                // consume this byte
                __UNUSED volatile uint8_t hole = read_reg(DUART_REG_RHRA_R);
                continue;
            }
#endif

            *c = read_reg(DUART_REG_RHRA_R);
            return 0;
        }
        if (wait) {
            continue;
        }
        break;
    } while (0);

    return -1;
}

lk_bigtime_t current_time_hires(void) {
    // TODO: look at the current countdown registers
    return ticks * 1000ULL;
}

lk_time_t current_time(void) {
    return ticks;
}

void target_set_debug_led(unsigned int led, bool on) {
    uint8_t bit = 0;
    switch (led) {
        case 0:
            bit = 5; // green LED
            break;
        case 1:
            bit = 3; // red LED
            break;
        default:
            return;
    }

    if (on) {
        write_reg(DUART_REG_SOPBC_W, (1<<bit));
    } else {
        write_reg(DUART_REG_COPBC_W, (1<<bit));
    }
}

