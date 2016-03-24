/*
 * Copyright (c) 2012 Kent Ryhorchuk
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
#include <stdarg.h>
#include <reg.h>
#include <debug.h>
#include <stdio.h>
#include <assert.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <arch/ops.h>
#include <dev/uart.h>
#include <target/debugconfig.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_usart.h>
#include <arch/arm/cm.h>

#ifdef ENABLE_UART1
cbuf_t uart1_rx_buf;
#ifndef UART1_FLOWCONTROL
#define UART1_FLOWCONTROL USART_HardwareFlowControl_None
#endif
#ifndef UART1_BAUDRATE
#define UART1_BAUDRATE 115200
#endif
#ifndef UART1_RXBUF_SIZE
#define UART1_RXBUF_SIZE 64
#endif
#endif

#ifdef ENABLE_UART2
cbuf_t uart2_rx_buf;
#ifndef UART2_FLOWCONTROL
#define UART2_FLOWCONTROL USART_HardwareFlowControl_None
#endif
#ifndef UART2_BAUDRATE
#define UART2_BAUDRATE 115200
#endif
#ifndef UART2_RXBUF_SIZE
#define UART2_RXBUF_SIZE 64
#endif
#endif

#ifdef ENABLE_UART3
cbuf_t uart3_rx_buf;
#ifndef UART3_FLOWCONTROL
#define UART3_FLOWCONTROL USART_HardwareFlowControl_None
#endif
#ifndef UART3_BAUDRATE
#define UART3_BAUDRATE 115200
#endif
#ifndef UART3_RXBUF_SIZE
#define UART3_RXBUF_SIZE 64
#endif
#endif

#ifdef ENABLE_UART6
cbuf_t uart6_rx_buf;
#ifndef UART6_FLOWCONTROL
#define UART6_FLOWCONTROL USART_HardwareFlowControl_None
#endif
#ifndef UART6_BAUDRATE
#define UART6_BAUDRATE 115200
#endif
#ifndef UART6_RXBUF_SIZE
#define UART6_RXBUF_SIZE 64
#endif
#endif

static void usart_init1_early(USART_TypeDef *usart, uint32_t baud, uint16_t flowcontrol, int irqn)
{
    USART_InitTypeDef init;

    init.USART_BaudRate = baud;
    init.USART_WordLength = USART_WordLength_8b;
    init.USART_StopBits = USART_StopBits_1;
    init.USART_Parity = USART_Parity_No;
    init.USART_Mode = USART_Mode_Tx|USART_Mode_Rx;
    init.USART_HardwareFlowControl = flowcontrol;

    USART_Init(usart, &init);
    USART_ITConfig(usart, USART_IT_RXNE, DISABLE);
    NVIC_DisableIRQ(irqn);
    USART_Cmd(usart, ENABLE);
}

static void usart_init1(USART_TypeDef *usart, int irqn, cbuf_t *rxbuf, size_t rxsize)
{
    cbuf_initialize(rxbuf, rxsize);
    USART_ITConfig(usart, USART_IT_RXNE, ENABLE);
    NVIC_EnableIRQ(irqn);
    USART_Cmd(usart, ENABLE);
}

void uart_init_early(void)
{
#ifdef ENABLE_UART1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    usart_init1_early(USART1, UART1_BAUDRATE, UART1_FLOWCONTROL, USART1_IRQn);
#endif
#ifdef ENABLE_UART2
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    usart_init1_early(USART2, UART2_BAUDRATE, UART2_FLOWCONTROL, USART2_IRQn);
#endif
#ifdef ENABLE_UART3
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    usart_init1_early(USART3, UART3_BAUDRATE, UART3_FLOWCONTROL, USART3_IRQn);
#endif
#ifdef ENABLE_UART6
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6, ENABLE);
    usart_init1_early(USART6, UART6_BAUDRATE, UART6_FLOWCONTROL, USART6_IRQn);
#endif
}

void uart_init(void)
{
#ifdef ENABLE_UART1
    usart_init1(USART1, USART1_IRQn, &uart1_rx_buf, UART1_RXBUF_SIZE);
#endif
#ifdef ENABLE_UART2
    usart_init1(USART2, USART2_IRQn, &uart2_rx_buf, UART2_RXBUF_SIZE);
#endif
#ifdef ENABLE_UART3
    usart_init1(USART3, USART3_IRQn, &uart3_rx_buf, UART3_RXBUF_SIZE);
#endif
#ifdef ENABLE_UART6
    usart_init1(USART6, USART6_IRQn, &uart6_rx_buf, UART6_RXBUF_SIZE);
#endif
}

void uart_rx_irq(USART_TypeDef *usart, cbuf_t *rxbuf)
{
    arm_cm_irq_entry();

    bool resched = false;
    while (USART_GetFlagStatus(usart, USART_FLAG_RXNE)) {
        if (!cbuf_space_avail(rxbuf)) {
            // Overflow - let flow control do its thing by not
            // reading the from the FIFO.
            USART_ITConfig(usart, USART_IT_RXNE, DISABLE);
            break;
        }

        char c = USART_ReceiveData(usart);
        cbuf_write_char(rxbuf, c, false);
        resched = true;
    }

    arm_cm_irq_exit(resched);
}

#ifdef ENABLE_UART1
void stm32_USART1_IRQ(void)
{
    uart_rx_irq(USART1, &uart1_rx_buf);
}
#endif

#ifdef ENABLE_UART2
void stm32_USART2_IRQ(void)
{
    uart_rx_irq(USART2, &uart2_rx_buf);
}
#endif

#ifdef ENABLE_UART3
void stm32_USART3_IRQ(void)
{
    uart_rx_irq(USART3, &uart3_rx_buf);
}
#endif

#ifdef ENABLE_UART6
void stm32_USART6_IRQ(void)
{
    uart_rx_irq(USART6, &uart6_rx_buf);
}
#endif

static void usart_putc(USART_TypeDef *usart, char c)
{
    while (USART_GetFlagStatus(usart, USART_FLAG_TXE) == 0);
    USART_SendData(usart, c);
    while (USART_GetFlagStatus(usart, USART_FLAG_TC) == 0);
}

static int usart_getc(USART_TypeDef *usart, cbuf_t *rxbuf, bool wait)
{
    unsigned char c;
    if (cbuf_read_char(rxbuf, (char *) &c, wait) == 0)
        return -1;
    if (cbuf_space_avail(rxbuf) > cbuf_size(rxbuf))
        USART_ITConfig(usart, USART_IT_RXNE, ENABLE);

    return c;
}

static USART_TypeDef *get_usart(int port)
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
#ifdef ENABLE_UART6
        case 6:
            return USART6;
#endif
        default:
            ASSERT(false);
            return 0;
    }
}

static cbuf_t *get_rxbuf(int port)
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
#ifdef ENABLE_UART6
        case 6:
            return &uart6_rx_buf;
#endif
        default:
            ASSERT(false);
            return 0;
    }
}

int uart_putc(int port, char c)
{
    USART_TypeDef *usart = get_usart(port);
    usart_putc(usart, c);
    return 1;
}

int uart_getc(int port, bool wait)
{
    cbuf_t *rxbuf = get_rxbuf(port);
    USART_TypeDef *usart = get_usart(port);

    return usart_getc(usart, rxbuf, wait);
}

void uart_flush_tx(int port) {}

void uart_flush_rx(int port) {}

void uart_init_port(int port, uint baud)
{
    USART_TypeDef *usart = get_usart(port);
    uint32_t treg = 0;
    uint32_t apbclk = 0;
    uint32_t intdiv = 0;
    uint32_t fracdiv = 0;
    RCC_ClocksTypeDef RCC_ClksStat;

    RCC_GetClocksFreq(&RCC_ClksStat);

    if ((usart == USART1) || (usart == USART6)) {
        apbclk = RCC_ClksStat.PCLK2_Frequency;
    } else {
        apbclk = RCC_ClksStat.PCLK1_Frequency;
    }

    if ((usart->CR1 & USART_CR1_OVER8) != 0) {
        intdiv = ((25 * apbclk) / (2 * (baud)));
    } else {
        intdiv = ((25 * apbclk) / (4 * (baud)));
    }
    treg = (intdiv / 100) << 4;

    fracdiv = intdiv - (100 * (treg >> 4));

    if ((usart->CR1 & USART_CR1_OVER8) != 0) {
        treg |= ((((fracdiv * 8) + 50) / 100)) & ((uint8_t) 0x07);
    } else {
        treg |= ((((fracdiv * 16) + 50) / 100)) & ((uint8_t) 0x0F);
    }

    usart->BRR = (uint16_t) treg;
}

// inject a character into the console uart rx buffer
void __debugger_console_putc(char c)
{
    cbuf_t *rxbuf = get_rxbuf(DEBUG_UART);
    if (rxbuf && cbuf_space_avail(rxbuf)) {
        cbuf_write_char(rxbuf, c, false);
    }
}

