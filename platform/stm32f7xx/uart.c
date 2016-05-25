/*
 * Copyright (c) 2012 Kent Ryhorchuk
 * Copyright (c) 2015 Travis Geiselbrecht
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
#include <err.h>
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <arch/ops.h>
#include <dev/uart.h>
#include <target/debugconfig.h>
#include <platform/stm32.h>
#include <arch/arm/cm.h>

#define DEFAULT_FLOWCONTROL UART_HWCONTROL_NONE
#define DEFAULT_BAUDRATE 115200
#define DEFAULT_RXBUF_SIZE 16

#define NUM_UARTS 8

struct uart_instance {
    UART_HandleTypeDef handle;
    cbuf_t rx_buf;
};

#if ENABLE_UART1
static struct uart_instance uart1;
#ifndef UART1_FLOWCONTROL
#define UART1_FLOWCONTROL DEFAULT_FLOWCONTROL
#endif
#ifndef UART1_BAUDRATE
#define UART1_BAUDRATE DEFAULT_BAUDRATE
#endif
#ifndef UART1_RXBUF_SIZE
#define UART1_RXBUF_SIZE DEFAULT_RXBUF_SIZE
#endif
#endif

#if ENABLE_UART3
static struct uart_instance uart3;
#ifndef UART3_FLOWCONTROL
#define UART3_FLOWCONTROL DEFAULT_FLOWCONTROL
#endif
#ifndef UART3_BAUDRATE
#define UART3_BAUDRATE DEFAULT_BAUDRATE
#endif
#ifndef UART3_RXBUF_SIZE
#define UART3_RXBUF_SIZE DEFAULT_RXBUF_SIZE
#endif
#endif

#if ENABLE_UART2 || ENABLE_UART4 || ENABLE_UART5 || ENABLE_UART6 || ENABLE_UART7 || ENABLE_UART8
#error add support for additional uarts
#endif

static struct uart_instance *const uart[NUM_UARTS + 1] = {
#if ENABLE_UART1
    [1] = &uart1,
#endif
#if ENABLE_UART3
    [3] = &uart3,
#endif
};

// This function is called by HAL_UART_Init().
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;

    /* Select SysClk as source of UART clocks */
    switch ((uintptr_t)huart->Instance) {
        case (uintptr_t)USART1:
            RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
            RCC_PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_SYSCLK;
            HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

            __HAL_RCC_USART1_CLK_ENABLE();
            break;
        case (uintptr_t)USART3:
            RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART3;
            RCC_PeriphClkInit.Usart3ClockSelection = RCC_USART3CLKSOURCE_SYSCLK;
            HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

            __HAL_RCC_USART3_CLK_ENABLE();
            break;
        default:
            panic("unimplemented clock set up for uart\n");
    }
}

static void usart_init_early(struct uart_instance *u, USART_TypeDef *usart, uint32_t baud, uint16_t flowcontrol)
{
    u->handle.Instance = usart;
    u->handle.Init.BaudRate = baud;
    u->handle.Init.WordLength = UART_WORDLENGTH_8B;
    u->handle.Init.StopBits = UART_STOPBITS_1;
    u->handle.Init.Parity = UART_PARITY_NONE;
    u->handle.Init.Mode = UART_MODE_TX_RX;
    u->handle.Init.HwFlowCtl  = flowcontrol;
    u->handle.Init.OverSampling = UART_OVERSAMPLING_8;
    HAL_UART_Init(&u->handle);
}

static void usart_init(struct uart_instance *u, USART_TypeDef *usart, uint irqn, size_t rxsize)
{
    cbuf_initialize(&u->rx_buf, rxsize);

    /* Enable the UART Parity Error Interrupt */
    __HAL_UART_ENABLE_IT(&u->handle, UART_IT_PE);

    /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
    __HAL_UART_ENABLE_IT(&u->handle, UART_IT_ERR);

    /* Enable the UART Data Register not empty Interrupt */
    __HAL_UART_ENABLE_IT(&u->handle, UART_IT_RXNE);

    HAL_NVIC_EnableIRQ(irqn);
}

void uart_init_early(void)
{
#if ENABLE_UART1
    usart_init_early(uart[1], USART1, UART1_BAUDRATE, UART1_FLOWCONTROL);
#endif
#if ENABLE_UART3
    usart_init_early(uart[3], USART3, UART3_BAUDRATE, UART3_FLOWCONTROL);
#endif
}

void uart_init(void)
{
#ifdef ENABLE_UART1
    usart_init(uart[1], USART1, USART1_IRQn, UART1_RXBUF_SIZE);
#endif
#ifdef ENABLE_UART3
    usart_init(uart[3], USART3, USART3_IRQn, UART3_RXBUF_SIZE);
#endif
}

static void stm32_usart_shared_irq(struct uart_instance *u, const unsigned int id)
{
    bool resched = false;

    arm_cm_irq_entry();

    /* UART parity error interrupt occurred -------------------------------------*/
    if ((__HAL_UART_GET_IT(&u->handle, UART_IT_PE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&u->handle, UART_IT_PE) != RESET)) {
        __HAL_UART_CLEAR_PEFLAG(&u->handle);

        printf("UART PARITY ERROR\n");
    }

    /* UART frame error interrupt occurred --------------------------------------*/
    if ((__HAL_UART_GET_IT(&u->handle, UART_IT_FE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&u->handle, UART_IT_ERR) != RESET)) {
        __HAL_UART_CLEAR_FEFLAG(&u->handle);

        printf("UART FRAME ERROR\n");
    }

    /* UART noise error interrupt occurred --------------------------------------*/
    if ((__HAL_UART_GET_IT(&u->handle, UART_IT_NE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&u->handle, UART_IT_ERR) != RESET)) {
        __HAL_UART_CLEAR_NEFLAG(&u->handle);

        printf("UART NOISE ERROR\n");
    }

    /* UART Over-Run interrupt occurred -----------------------------------------*/
    if ((__HAL_UART_GET_IT(&u->handle, UART_IT_ORE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&u->handle, UART_IT_ERR) != RESET)) {
        __HAL_UART_CLEAR_OREFLAG(&u->handle);

        printf("UART OVERRUN ERROR\n");
    }

    /* UART in mode Receiver ---------------------------------------------------*/
    if ((__HAL_UART_GET_IT(&u->handle, UART_IT_RXNE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&u->handle, UART_IT_RXNE) != RESET)) {

        /* we got a character */
        uint8_t c = (uint8_t)(u->handle.Instance->RDR & 0xff);

        cbuf_t *target_buf = &u->rx_buf;

#if CONSOLE_HAS_INPUT_BUFFER
        if (id == DEBUG_UART) {
            target_buf = &console_input_cbuf;
        }
#endif

        if (cbuf_write_char(target_buf, c, false) != 1) {
            printf("WARNING: uart cbuf overrun!\n");
        }
        resched = true;

        /* Clear RXNE interrupt flag */
        __HAL_UART_SEND_REQ(&u->handle, UART_RXDATA_FLUSH_REQUEST);
    }

    /* UART in mode Transmitter ------------------------------------------------*/
    if ((__HAL_UART_GET_IT(&u->handle, UART_IT_TXE) != RESET) &&(__HAL_UART_GET_IT_SOURCE(&u->handle, UART_IT_TXE) != RESET)) {
        ;
    }

    /* UART in mode Transmitter (transmission end) -----------------------------*/
    if ((__HAL_UART_GET_IT(&u->handle, UART_IT_TC) != RESET) &&(__HAL_UART_GET_IT_SOURCE(&u->handle, UART_IT_TC) != RESET)) {
        ;
    }

    arm_cm_irq_exit(resched);
}

#if ENABLE_UART1
void stm32_USART1_IRQ(void)
{
    stm32_usart_shared_irq(uart[1], 1);
}
#endif

#if ENABLE_UART3
void stm32_USART3_IRQ(void)
{
    stm32_usart_shared_irq(uart[3], 3);
}
#endif

int uart_putc(int port, char c)
{
    struct uart_instance *u = uart[port];
    if (port < 0 || port > NUM_UARTS || !u)
        return ERR_BAD_HANDLE;

    while (__HAL_UART_GET_FLAG(&u->handle, UART_FLAG_TXE) == RESET)
        ;
    u->handle.Instance->TDR = (c & (uint8_t)0xFF);

    return 1;
}

int uart_getc(int port, bool wait)
{
    struct uart_instance *u = uart[port];
    if (port < 0 || port > NUM_UARTS || !u)
        return ERR_BAD_HANDLE;

    char c;
    if (cbuf_read_char(&u->rx_buf, &c, wait) == 0)
        return ERR_IO;
    return c;
}

int uart_pputc(int port, char c)
{
    return uart_putc(port, c);
}

int uart_pgetc(int port)
{
    struct uart_instance *u = uart[port];
    if (port < 0 || port > NUM_UARTS || !u)
        return ERR_BAD_HANDLE;

    if ((__HAL_UART_GET_IT(&u->handle, UART_IT_RXNE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&u->handle, UART_IT_RXNE) != RESET)) {
        uint8_t c = (uint8_t)(u->handle.Instance->RDR & 0xff);
        return c;
    }
    return ERR_IO;
}

void uart_flush_tx(int port) {}

void uart_flush_rx(int port) {}

void uart_init_port(int port, uint baud)
{
    // TODO - later
    PANIC_UNIMPLEMENTED;
}
