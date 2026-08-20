/*
 * Copyright (c) 2026 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <arch/arm/cm.h>
#include <assert.h>
#include <dev/uart.h>
#include <lib/cbuf.h>
#include <lib/io.h>
#include <platform/rcc.h>
#include <stdint.h>
#include <stm32h5xx.h>
#include <target/debugconfig.h>

typedef USART_TypeDef stm32_usart_t;

#define RXBUF_SIZE 16

#ifdef ENABLE_UART1
static cbuf_t uart1_rx_buf;
#endif

#ifdef ENABLE_UART3
static cbuf_t uart3_rx_buf;
#endif

/*
 * All of the uarts come out of reset clocked from PCLK, and SystemInit() does
 * not touch the bus prescalers, so the uart clock tracks the system clock.
 */
static uint32_t stm32_usart_clock(void) {
    return SystemCoreClock;
}

static void stm32_usart_init1_early(stm32_usart_t *usart, int irqn) {
    const uint32_t baud_rate = 115200;

    // Ensure USART is disabled before configuring it.
    usart->CR1 = 0;

    // Set stop bits to 1 (CR2[13:12] = 00b)
    usart->CR2 = 0;
    usart->CR3 = 0;

    // Go with the defaults of:
    //   word length: 8 bits
    //   parity: disabled
    //   oversampling: by 16
    // Enable TX and RX.
    usart->CR1 = USART_CR1_TE | USART_CR1_RE;

    const uint32_t clock = stm32_usart_clock();
    usart->BRR = (clock + baud_rate / 2) / baud_rate;

    // Leave IRQs disabled until init.
    NVIC_DisableIRQ(irqn);

    // Enable UART.
    usart->CR1 |= USART_CR1_UE;
}

static void stm32_usart_init1(stm32_usart_t *usart, int irqn, cbuf_t *rxbuf) {
    cbuf_initialize(rxbuf, RXBUF_SIZE);
    // Enable RX not empty interrupt.
    usart->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
    NVIC_EnableIRQ(irqn);
}

void uart_init_early(void) {
#ifdef ENABLE_UART1
    stm32_rcc_set_enable(STM32_RCC_CLK_USART1, true);
    stm32_usart_init1_early(USART1, USART1_IRQn);
#endif
#ifdef ENABLE_UART3
    stm32_rcc_set_enable(STM32_RCC_CLK_USART3, true);
    stm32_usart_init1_early(USART3, USART3_IRQn);
#endif
}

void uart_init(void) {
#ifdef ENABLE_UART1
    stm32_usart_init1(USART1, USART1_IRQn, &uart1_rx_buf);
#endif
#ifdef ENABLE_UART3
    stm32_usart_init1(USART3, USART3_IRQn, &uart3_rx_buf);
#endif
}

static cbuf_t *stm32_get_rxbuf(int port) {
#if CONSOLE_HAS_INPUT_BUFFER
    if (DEBUG_UART == port) {
        return &console_input_cbuf;
    }
#endif
    switch (port) {
#ifdef ENABLE_UART1
        case 1:
            return &uart1_rx_buf;
#endif
#ifdef ENABLE_UART3
        case 3:
            return &uart3_rx_buf;
#endif
        default:
            ASSERT(false);
            return 0;
    }
}

static void stm32_uart_rx_irq(stm32_usart_t *usart, cbuf_t *rxbuf) {
    arm_cm_irq_entry();

    bool resched = false;
    while (usart->ISR & USART_ISR_RXNE_RXFNE) {
        if (!cbuf_space_avail(rxbuf)) {
            // Overflow - let flow control do its thing by not
            // reading from the FIFO.
            usart->CR1 &= ~USART_CR1_RXNEIE_RXFNEIE;
            break;
        }

        char c = usart->RDR;
        cbuf_write_char(rxbuf, c, false);
        resched = true;
    }

    arm_cm_irq_exit(resched);
}

#ifdef ENABLE_UART1
void stm32_USART1_IRQ(void) {
    stm32_uart_rx_irq(USART1, stm32_get_rxbuf(1));
}
#endif

#ifdef ENABLE_UART3
void stm32_USART3_IRQ(void) {
    stm32_uart_rx_irq(USART3, stm32_get_rxbuf(3));
}
#endif

static void stm32_usart_putc(stm32_usart_t *usart, char c) {
    while ((usart->ISR & USART_ISR_TXE_TXFNF) == 0);
    usart->TDR = c;
    while ((usart->ISR & USART_ISR_TC) == 0);
}

static int stm32_usart_getc(stm32_usart_t *usart, cbuf_t *rxbuf, bool wait) {
    char c;
    if (cbuf_read_char(rxbuf, &c, wait) == 0) {
        return -1;
    }
    if (cbuf_space_avail(rxbuf) > RXBUF_SIZE/2)
        usart->CR1 |= USART_CR1_RXNEIE_RXFNEIE;

    return c;
}

static stm32_usart_t *stm32_get_usart(int port) {
    switch (port) {
#ifdef ENABLE_UART1
        case 1:
            return USART1;
#endif
#ifdef ENABLE_UART3
        case 3:
            return USART3;
#endif
        default:
            ASSERT(false);
            return 0;
    }
}

int uart_putc(int port, char c) {
    stm32_usart_t *usart = stm32_get_usart(port);
    stm32_usart_putc(usart, c);
    return 1;
}

int uart_getc(int port, bool wait) {
    cbuf_t *rxbuf = stm32_get_rxbuf(port);
    stm32_usart_t *usart = stm32_get_usart(port);

    return stm32_usart_getc(usart, rxbuf, wait);
}

void uart_flush_tx(int port) {}

void uart_flush_rx(int port) {}

void uart_init_port(int port, uint baud) {
    // TODO - later
    PANIC_UNIMPLEMENTED;
}
