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
#include <stm32f7xx_hal.h>
#include <stm32f7xx_hal_rcc.h>
#include <stm32f7xx_hal_rcc_ex.h> 
#include <stm32f7xx_hal_dma.h>
#include <stm32f7xx_hal_uart.h>
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


#define USARTx                           USART1

#define USARTx_TX_PIN                    GPIO_PIN_9
#define USARTx_TX_GPIO_PORT              GPIOA
#define USARTx_TX_AF                     GPIO_AF7_USART1
#define USARTx_RX_PIN                    GPIO_PIN_10
#define USARTx_RX_GPIO_PORT              GPIOA
#define USARTx_RX_AF                     GPIO_AF7_USART1

static UART_HandleTypeDef handle;

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef  GPIO_InitStruct;

  RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Select SysClk as source of USART1 clocks */
  RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  RCC_PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_HSI;
  HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

  /* Enable USARTx clock */
  __HAL_RCC_USART1_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* UART TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = USARTx_TX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = USARTx_TX_AF;

  HAL_GPIO_Init(USARTx_TX_GPIO_PORT, &GPIO_InitStruct);

  /* UART RX GPIO pin configuration  */
  GPIO_InitStruct.Pin = USARTx_RX_PIN;
  GPIO_InitStruct.Alternate = USARTx_RX_AF;

  HAL_GPIO_Init(USARTx_RX_GPIO_PORT, &GPIO_InitStruct);
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
#if 0
    HAL_USART_ITConfig(usart, USART_IT_RXNE, DISABLE);
    NVIC_DisableIRQ(irqn);
    HAL_USART_Cmd(usart, ENABLE);
#endif
}

static void usart_init1(USART_TypeDef *usart, int irqn, cbuf_t *rxbuf, size_t rxsize)
{
    cbuf_initialize(rxbuf, rxsize);
#if 0
    USART_ITConfig(usart, USART_IT_RXNE, ENABLE);
    NVIC_EnableIRQ(irqn);
    USART_Cmd(usart, ENABLE);
#endif
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

void uart_rx_irq(USART_TypeDef *usart, cbuf_t *rxbuf)
{
    arm_cm_irq_entry();

    bool resched = false;
#if 0
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
#endif

    arm_cm_irq_exit(resched);
}

#ifdef ENABLE_UART1
void stm32_USART1_IRQ(void)
{
    uart_rx_irq(USART1, &uart1_rx_buf);
}
#endif

static void usart_putc(USART_TypeDef *usart, char c)
{
    //HAL_StatusTypeDef HAL_USART_Transmit(USART_HandleTypeDef *husart, uint8_t *pTxData, uint16_t Size, uint32_t Timeout);

    HAL_UART_Transmit(&handle, (uint8_t *)&c, 1, HAL_MAX_DELAY);
}

static int usart_getc(USART_TypeDef *usart, cbuf_t *rxbuf, bool wait)
{
    //HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)

    uint8_t c;
    HAL_StatusTypeDef ret = HAL_UART_Receive(&handle, &c, 1, 0);
    if (ret == HAL_OK)
        return c;

    return -1;
#if 0
    unsigned char c;
    if (cbuf_read_char(rxbuf, (char*) &c, wait) == 0)
        return -1;

    if (cbuf_space_avail(rxbuf) > cbuf_size(rxbuf))
        USART_ITConfig(usart, USART_IT_RXNE, ENABLE);

    return c;
#endif
}

static USART_TypeDef *get_usart(int port)
{
    switch (port) {
#ifdef ENABLE_UART1
    case 1: return USART1;
#endif
#ifdef ENABLE_UART2
    case 2: return USART2;
#endif
#ifdef ENABLE_UART3
    case 3: return USART3;
#endif
#ifdef ENABLE_UART6
    case 6: return USART6;
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
    case 1: return &uart1_rx_buf;
#endif
#ifdef ENABLE_UART2
    case 2: return &uart2_rx_buf;
#endif
#ifdef ENABLE_UART3
    case 3: return &uart3_rx_buf;
#endif
#ifdef ENABLE_UART6
    case 6: return &uart6_rx_buf;
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
    // TODO - later
    PANIC_UNIMPLEMENTED;
}
