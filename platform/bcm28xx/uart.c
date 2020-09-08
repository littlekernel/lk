/*
 * Copyright (c) 2015 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/reg.h>
#include <stdio.h>
#include <lk/trace.h>
#include <lk/console_cmd.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/interrupts.h>
#include <platform/debug.h>
#include <platform/bcm28xx.h>
#include <platform/bcm28xx/pll_read.h>
#include <assert.h>
#include <dev/gpio.h>

/* TODO: extract this into a generic PL011 driver */

/* PL011 implementation */
#define UART_DR    (0x00)
#define UART_RSR   (0x04)
#define UART_TFR   (0x18)
#define UART_ILPR  (0x20)
#define UART_IBRD  (0x24)
#define UART_FBRD  (0x28)
#define UART_LCRH  (0x2c)
#define UART_CR    (0x30)
#define UART_IFLS  (0x34)
#define UART_IMSC  (0x38)
#define UART_TRIS  (0x3c)
#define UART_TMIS  (0x40)
#define UART_ICR   (0x44)
#define UART_DMACR (0x48)

#define UARTREG(base, reg)  (*REG32((base)  + (reg)))

#define RXBUF_SIZE 16
#define NUM_UART 1

static cbuf_t uart_rx_buf[NUM_UART];

static int cmd_uart_dump(int argc, const cmd_args *argv);

//STATIC_COMMAND_START
//STATIC_COMMAND("dump_uart_state", "print uart state relating to baud", &cmd_uart_dump)
//STATIC_COMMAND_END(uart);

static inline uintptr_t uart_to_ptr(unsigned int n) {
    switch (n) {
        default:
        case 0:
            return UART0_BASE;
    }
}

static uint32_t calculate_baud_divisor(uint32_t baud) {
  uint32_t uart_freq = get_uart_base_freq();
  uint32_t divisor = (uart_freq << 6) / baud / 16;
  return divisor;
}

static enum handler_return uart_irq(void *arg) {
    bool resched = false;
    uint port = (uint)arg;
    uintptr_t base = uart_to_ptr(port);

    /* read interrupt status and mask */
    uint32_t isr = UARTREG(base, UART_TMIS);

    if (isr & ((1<<6) | (1<<4))) { // rtmis, rxmis
        UARTREG(base, UART_ICR) = (1<<4);
        cbuf_t *rxbuf = &uart_rx_buf[port];

        /* while fifo is not empty, read chars out of it */
        while ((UARTREG(base, UART_TFR) & (1<<4)) == 0) {
            uint32_t data = UARTREG(base, UART_DR);
            char c = data & 0xff;
            if (data & 0x400) {
              dprintf(INFO, "UART break detected\n");
            } else if (data & 0x100) {
              dprintf(INFO, "UART framing error\n");
            } else {
              if (data & 0x800) {
                dprintf(INFO, "UART input overflow\n");
              }
              cbuf_write_char(rxbuf, c, false);

              resched = true;
            }
        }
    }

    return resched ? INT_RESCHEDULE : INT_NO_RESCHEDULE;
}

static void uart_flush(int port) {
  uintptr_t base = uart_to_ptr(port);
  while (UARTREG(base, UART_TFR) & 0x20);
}

void uart_init(void) {
    gpio_config(14, 4);
    gpio_config(15, 4);
    for (size_t i = 0; i < NUM_UART; i++) {
        uintptr_t base = uart_to_ptr(i);

        // create circular buffer to hold received data
        cbuf_initialize(&uart_rx_buf[i], RXBUF_SIZE);
        DEBUG_ASSERT(uart_rx_buf[i].event.magic == EVENT_MAGIC);

        // assumes interrupts are contiguous
        register_int_handler(INTERRUPT_VC_UART + i, &uart_irq, (void *)i);
        uint32_t divisor = calculate_baud_divisor(115200);

        uart_flush(i);

        UARTREG(base, UART_CR) = 0; // shutdown the entire uart

        // clear all irqs
        UARTREG(uart_to_ptr(i), UART_ICR) = 0x3ff;

        UARTREG(base, UART_LCRH) = 0x70; // fifo enable, 8bit mode

        if (divisor > 0) {
          UARTREG(base, UART_IBRD) = (divisor >> 6) & 0xffff;
          UARTREG(base, UART_FBRD) = divisor & 0x3f;
        }

        // set fifo trigger level
        UARTREG(uart_to_ptr(i), UART_IFLS) = 0; // 1/8 rxfifo, 1/8 txfifo

        // enable rx interrupt
        UARTREG(uart_to_ptr(i), UART_IMSC) = (1<<6)|(1<<4); // rtim, rxim

        // enable receive
        UARTREG(uart_to_ptr(i), UART_CR) |= (1<<9) | (1<<8) | (1<<0); // rxen, tx_enable, uarten

        // enable interrupt
        unmask_interrupt(INTERRUPT_VC_UART + i);
    }
}

int uart_putc(int port, char c);
void udelay(uint32_t t);

void uart_init_early(void) {
    if (*REG32(CM_UARTDIV) == 0) {
      // CM_UARTDIV can range from 0 to 1023 with a fractional resolution of 1/4096th
      // on the rpi1-3, this sets the freq to 19.2 / (0x3900 / 0x1000) == ~5.3mhz
      // TODO, have a better default for other models?
      *REG32(CM_UARTDIV) = CM_PASSWORD | 0x3900;
      *REG32(CM_UARTCTL) = CM_PASSWORD | CM_SRC_OSC | CM_UARTCTL_FRAC_SET | CM_UARTCTL_ENAB_SET;
    }
    udelay(100);

    for (size_t i = 0; i < NUM_UART; i++) {
        uintptr_t base = uart_to_ptr(i);
        uint32_t divisor = calculate_baud_divisor(115200);
        UARTREG(base, UART_CR) = 0; // shutdown the entire uart
        UARTREG(base, UART_LCRH) = 0x70; // fifo enable, 8bit mode
        if (divisor > 0) {
          UARTREG(base, UART_IBRD) = (divisor >> 6) & 0xffff;
          UARTREG(base, UART_FBRD) = divisor & 0x3f;
        }
        UARTREG(base, UART_CR) = (1<<8)|(1<<0); // tx_enable, uarten
    }
    gpio_config(14, 4);
    printf("uart early init done\n");
}

int uart_putc(int port, char c) {
    uintptr_t base = uart_to_ptr(port);

    /* spin while fifo is full */
    while (UARTREG(base, UART_TFR) & (1<<5))
        ;
    UARTREG(base, UART_DR) = c;

    return 1;
}

int uart_getc(int port, bool wait) {
    cbuf_t *rxbuf = &uart_rx_buf[port];

    char c;
    assert(port < NUM_UART);
    if (cbuf_read_char(rxbuf, &c, wait) == 1)
        return c;

    return -1;
}

void uart_flush_tx(int port) {
}

void uart_flush_rx(int port) {
}

void uart_init_port(int port, uint baud) {
}

static int cmd_uart_dump(int argc, const cmd_args *argv) {
  uintptr_t base = uart_to_ptr(0);
  uint32_t baud = 115200;

  if (argc >= 2) {
    baud = argv[1].u;
  }

  dprintf(INFO, "CM_UARTDIV: 0x%x\nCM_UARTCTL: 0x%x\n", *REG32(CM_UARTDIV), *REG32(CM_UARTCTL));
  dprintf(INFO, "UART_IBRD: 0x%x\nUART_FBRD: 0x%x\n", UARTREG(base, UART_IBRD), UARTREG(base, UART_FBRD));
  uint32_t uart_freq = get_uart_base_freq();
  dprintf(INFO, "uart input clock is %d\n", uart_freq);
  uint32_t divisor = calculate_baud_divisor(baud);

  dprintf(INFO, "want a uart divisor of 0x%x / 64\n", divisor);
  return 0;
}
