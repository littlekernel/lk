/*
 * Copyright (c) 2012 Kent Ryhorchuk
 * Copyright (c) 2016 Erik Gilling
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <arch/arm/cm.h>
#include <assert.h>
#include <dev/uart.h>
#include <lib/cbuf.h>
#include <platform/rcc.h>
#include <stdint.h>
#include <stm32f0xx.h>

typedef USART_TypeDef stm32_usart_t;

#define RXBUF_SIZE 16

#ifdef ENABLE_UART1
cbuf_t uart1_rx_buf;
#ifndef UART1_FLOWCONTROL
#define UART1_FLOWCONTROL 0x0
#endif
#endif

#ifdef ENABLE_UART2
cbuf_t uart2_rx_buf;
#ifndef UART2_FLOWCONTROL
#define UART2_FLOWCONTROL 0x0
#endif
#endif

#ifdef ENABLE_UART3
cbuf_t uart3_rx_buf;
#ifndef UART3_FLOWCONTROL
#define UART3_FLOWCONTROL 0x0
#endif
#endif

#ifdef ENABLE_UART1
#endif
#ifdef ENABLE_UART2
#endif
#ifdef ENABLE_UART3
#endif

static void stm32_usart_init1_early(stm32_usart_t *usart,
                                    uint16_t flow_control, int irqn)
{
    uint32_t baud_rate = 115200;

    // Ensure USART is disabled before configuring it.
    usart->CR1 = 0;

    // Set stop bits to 1 (CR2[13:12] = 00b)
    usart->CR2 = 0;


    // Go with the defaults of:
    //   word length: 8 bits
    //   parity: disabled
    // Enable TX and RX.
    usart->CR1 = USART_CR1_TE | USART_CR1_RE;

    usart->CR3 = flow_control;

    // TODO(konkers): Add rcc API for querying clock freq.
    uint32_t apb_clock = 48000000;

    usart->BRR = (apb_clock + baud_rate / 2) / baud_rate;

    // Leave IRQs disabled until init.
    NVIC_DisableIRQ(irqn);

    // Enable UART.
    usart->CR1 |= USART_CR1_UE;
}

static void stm32_usart_init1(stm32_usart_t *usart, int irqn, cbuf_t *rxbuf)
{
    cbuf_initialize(rxbuf, RXBUF_SIZE);
    // Enable RX not empty interrupt.
    usart->CR1 |= USART_CR1_RXNEIE;
    NVIC_EnableIRQ(irqn);
}

void uart_init_early(void)
{
#ifdef ENABLE_UART1
    stm32_rcc_set_enable(STM32_RCC_CLK_USART1, true);
#endif
#ifdef ENABLE_UART2
    stm32_rcc_set_enable(STM32_RCC_CLK_USART2, true);
#endif
#ifdef ENABLE_UART3
    stm32_rcc_set_enable(STM32_RCC_CLK_USART3, true);
#endif

#ifdef ENABLE_UART1
    stm32_usart_init1_early(USART1, UART1_FLOWCONTROL, USART1_IRQn);
#endif
#ifdef ENABLE_UART2
    stm32_usart_init1_early(USART2, UART2_FLOWCONTROL, USART2_IRQn);
#endif
#ifdef ENABLE_UART3
    stm32_usart_init1_early(USART3, UART3_FLOWCONTROL, USART3_IRQn);
#endif
}

void uart_init(void)
{
#ifdef ENABLE_UART1
    stm32_usart_init1(USART1, USART1_IRQn, &uart1_rx_buf);
#endif
#ifdef ENABLE_UART2
    stm32_usart_init1(USART2, USART2_IRQn, &uart2_rx_buf);
#endif
#ifdef ENABLE_UART3
    stm32_usart_init1(USART3, USART3_IRQn, &uart3_rx_buf);
#endif
}

// I'm seeing a weird issue with my nucleo-f072rb board where rx interrupts
// don't fire right after a reset by the on board programmer.  If I hit
// the reset button, rx works fine.  I can live with this. YMMV.
static void stm32_uart_rx_irq(stm32_usart_t *usart, cbuf_t *rxbuf)
{
    arm_cm_irq_entry();

    bool resched = false;
    while (usart->ISR & USART_ISR_RXNE) {
        if (!cbuf_space_avail(rxbuf)) {
            // Overflow - let flow control do its thing by not
            // reading the from the FIFO.
            usart->CR1 &= ~USART_CR1_RXNEIE;
            break;
        }

        char c = usart->RDR;
        cbuf_write_char(rxbuf, c, false);
        resched = true;
    }

    arm_cm_irq_exit(resched);
}

#ifdef ENABLE_UART1
void stm32_USART1_IRQ(void)
{
    stm32_uart_rx_irq(USART1, &uart1_rx_buf);
}
#endif

#ifdef ENABLE_UART2
void stm32_USART2_IRQ(void)
{
    stm32_uart_rx_irq(USART2, &uart2_rx_buf);
}
#endif

#ifdef ENABLE_UART3
void stm32_USART3_IRQ(void)
{
    stm32_uart_rx_irq(USART3, &uart3_rx_buf);
}
#endif


static void stm32_usart_putc(stm32_usart_t *usart, char c)
{
    while ((usart->ISR & USART_ISR_TXE) == 0);
    usart->TDR = c;
    while ((usart->ISR & USART_ISR_TC) == 0);
}

static int stm32_usart_getc(stm32_usart_t *usart, cbuf_t *rxbuf, bool wait)
{
    char c;
    cbuf_read_char(rxbuf, &c, wait);
    if (cbuf_space_avail(rxbuf) > RXBUF_SIZE/2)
        usart->CR1 |= USART_CR1_RXNEIE;

    return c;
}

static stm32_usart_t *stm32_get_usart(int port)
{
    switch (port) {
#ifdef ENABLE_UART1
        case 1:
            return USART1;
#endif
#ifdef ENABLE_UART2
        case 2:
            return USART2;
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

static cbuf_t *stm32_get_rxbuf(int port)
{
    switch (port) {
#ifdef ENABLE_UART1
        case 1:
            return &uart1_rx_buf;
#endif
#ifdef ENABLE_UART2
        case 2:
            return &uart2_rx_buf;
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

int uart_putc(int port, char c)
{
    stm32_usart_t *usart = stm32_get_usart(port);
    stm32_usart_putc(usart, c);
    return 1;
}

int uart_getc(int port, bool wait)
{
    cbuf_t *rxbuf = stm32_get_rxbuf(port);
    stm32_usart_t *usart = stm32_get_usart(port);

    return stm32_usart_getc(usart, rxbuf, wait);
}

void uart_flush_tx(int port) {}

void uart_flush_rx(int port) {}

void uart_init_port(int port, uint baud)
{
    // TODO - later
    PANIC_UNIMPLEMENTED;
}
