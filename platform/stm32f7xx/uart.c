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
#include <lib/cbuf.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <arch/ops.h>
#include <dev/uart.h>
#include <target/debugconfig.h>
#include <platform/stm32.h>
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
#define UART1_RXBUF_SIZE 16
#endif
#endif

static UART_HandleTypeDef handle;

// This function is called by HAL_UART_Init().
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    if (huart != &handle) {
        //  !! harcoded only for USART1, like the rest of this file.
        ITM_SendChar('!');
        return;
    }

    RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;

    /*##-1- Enable Clocks #################################*/

    /* Select SysClk as source of USART1 clocks */
    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    RCC_PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_SYSCLK;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    /* Enable USARTx clock */
    __HAL_RCC_USART1_CLK_ENABLE();

    /*##-2- Make sure the irq handler is disabled for now */
    /* NVIC for USARTx */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
}

static void usart_init1_early(USART_TypeDef *usart, uint32_t baud, uint16_t flowcontrol, int irqn)
{
    handle.Instance = usart;
    handle.Init.BaudRate = baud;
    handle.Init.WordLength = UART_WORDLENGTH_8B;
    handle.Init.StopBits = UART_STOPBITS_1;
    handle.Init.Parity = UART_PARITY_NONE;
    handle.Init.Mode = UART_MODE_TX_RX;
    handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    handle.Init.OverSampling = UART_OVERSAMPLING_8;
    HAL_UART_Init(&handle);
}

static void usart_init1(USART_TypeDef *usart, int irqn, cbuf_t *rxbuf, size_t rxsize)
{
    cbuf_initialize(rxbuf, rxsize);

    /* Enable the UART Parity Error Interrupt */
    __HAL_UART_ENABLE_IT(&handle, UART_IT_PE);

    /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
    __HAL_UART_ENABLE_IT(&handle, UART_IT_ERR);

    /* Enable the UART Data Register not empty Interrupt */
    __HAL_UART_ENABLE_IT(&handle, UART_IT_RXNE);

    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void uart_init_early(void)
{
#if ENABLE_UART1
    usart_init1_early(USART1, UART1_BAUDRATE, 0, USART1_IRQn);
#endif
}

void uart_init(void)
{
#ifdef ENABLE_UART1
    usart_init1(USART1, USART1_IRQn, &uart1_rx_buf, UART1_RXBUF_SIZE);
#endif
}

#if 0
#ifdef ENABLE_UART1
void stm32_USART1_IRQ(void)
{
    arm_cm_irq_entry();
    HAL_UART_IRQHandler(&handle);
    arm_cm_irq_exit(true);
}
#endif

// TODO: remove icc hack.
static char icc;

// Thi function is called from HAL_UART_IRQHandler().
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    cbuf_write_char(&uart1_rx_buf, icc, false);
}
#endif

void stm32_USART1_IRQ(void)
{
    bool resched = false;

    arm_cm_irq_entry();

    /* UART parity error interrupt occurred -------------------------------------*/
    if ((__HAL_UART_GET_IT(&handle, UART_IT_PE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&handle, UART_IT_PE) != RESET)) {
        __HAL_UART_CLEAR_PEFLAG(&handle);

        printf("PARITY ERROR\n");
    }

    /* UART frame error interrupt occurred --------------------------------------*/
    if ((__HAL_UART_GET_IT(&handle, UART_IT_FE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&handle, UART_IT_ERR) != RESET)) {
        __HAL_UART_CLEAR_FEFLAG(&handle);

        printf("FRAME ERROR\n");
    }

    /* UART noise error interrupt occurred --------------------------------------*/
    if ((__HAL_UART_GET_IT(&handle, UART_IT_NE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&handle, UART_IT_ERR) != RESET)) {
        __HAL_UART_CLEAR_NEFLAG(&handle);

        printf("UART NOISE ERROR\n");
    }

    /* UART Over-Run interrupt occurred -----------------------------------------*/
    if ((__HAL_UART_GET_IT(&handle, UART_IT_ORE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&handle, UART_IT_ERR) != RESET)) {
        __HAL_UART_CLEAR_OREFLAG(&handle);

        printf("OVERRUN ERROR\n");
    }

    /* UART in mode Receiver ---------------------------------------------------*/
    if ((__HAL_UART_GET_IT(&handle, UART_IT_RXNE) != RESET) && (__HAL_UART_GET_IT_SOURCE(&handle, UART_IT_RXNE) != RESET)) {

        /* we got a character */
        uint8_t c = (uint8_t)(handle.Instance->RDR & 0xff);
        cbuf_write_char(&uart1_rx_buf, c, false);
        resched = true;

        /* Clear RXNE interrupt flag */
        __HAL_UART_SEND_REQ(&handle, UART_RXDATA_FLUSH_REQUEST);
    }

    /* UART in mode Transmitter ------------------------------------------------*/
    if ((__HAL_UART_GET_IT(&handle, UART_IT_TXE) != RESET) &&(__HAL_UART_GET_IT_SOURCE(&handle, UART_IT_TXE) != RESET)) {
        ;
    }

    /* UART in mode Transmitter (transmission end) -----------------------------*/
    if ((__HAL_UART_GET_IT(&handle, UART_IT_TC) != RESET) &&(__HAL_UART_GET_IT_SOURCE(&handle, UART_IT_TC) != RESET)) {
        ;
    }

    arm_cm_irq_exit(resched);
}

int uart_putc(int port, char c)
{
#if 0
    HAL_StatusTypeDef hs = HAL_UART_Transmit(&handle, (uint8_t *)&c, 1, HAL_MAX_DELAY);
    if (hs != HAL_OK)
        return -1;
    return 1;
#else
    while (__HAL_UART_GET_FLAG(&handle, UART_FLAG_TXE) == RESET)
        ;
    handle.Instance->TDR = (c & (uint8_t)0xFF);

    return 1;
#endif
}

int uart_getc(int port, bool wait)
{
#if 0
    HAL_StatusTypeDef hs =  HAL_UART_Receive_IT(&handle, (uint8_t *)&icc, 1);
    if (hs != HAL_OK)
        return -1;
#endif
    char c;
    if (cbuf_read_char(&uart1_rx_buf, &c, wait) == 0)
        return -1;
    return c;
}

void uart_flush_tx(int port) {}

void uart_flush_rx(int port) {}

void uart_init_port(int port, uint baud)
{
    // TODO - later
    PANIC_UNIMPLEMENTED;
}
