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


#define USART1_TX_PIN                    GPIO_PIN_9
#define USART1_TX_GPIO_PORT              GPIOA
#define USART1_TX_AF                     GPIO_AF7_USART1
#define USART1_RX_PIN                    GPIO_PIN_10
#define USART1_RX_GPIO_PORT              GPIOA
#define USART1_RX_AF                     GPIO_AF7_USART1

static UART_HandleTypeDef handle;

// This function is called by HAL_UART_Init().
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  if (huart != &handle)
  {
    //  !! harcoded only for USART1, like the rest of this file.
    return;
  }

  GPIO_InitTypeDef  GPIO_InitStruct;

  RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;

  /*##-1- Enable peripherals and GPIO Clocks #################################*/
  /* Enable GPIO TX/RX clock */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* Select SysClk as source of USART1 clocks */
  RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  RCC_PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_SYSCLK;
  HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

  /* Enable USARTx clock */
  __HAL_RCC_USART1_CLK_ENABLE();

  /*##-2- Configure peripheral GPIO ##########################################*/
  /* UART TX GPIO pin configuration  */
  GPIO_InitStruct.Pin       = USART1_TX_PIN;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_PULLUP;
  GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
  GPIO_InitStruct.Alternate = USART1_TX_AF;

  HAL_GPIO_Init(USART1_TX_GPIO_PORT, &GPIO_InitStruct);

  /* UART RX GPIO pin configuration  */
  GPIO_InitStruct.Pin = USART1_RX_PIN;
  GPIO_InitStruct.Alternate = USART1_RX_AF;

  HAL_GPIO_Init(USART1_RX_GPIO_PORT, &GPIO_InitStruct);

  /*##-3- Configure the NVIC for UART ########################################*/   
  /* NVIC for USARTx */
  HAL_NVIC_SetPriority(USART1_IRQn, 0, 1);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  ITM_SendChar('@');
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

#ifdef ENABLE_UART1
void stm32_USART1_IRQ(void)
{
    arm_cm_irq_entry();
    ITM_SendChar('x');
    HAL_UART_IRQHandler(&handle);
    ITM_SendChar('y');
    arm_cm_irq_exit(true);
}
#endif

static char icc;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    cbuf_write_char(&uart1_rx_buf, icc, false);
    ITM_SendChar(icc);
}

int uart_putc(int port, char c)
{ 
    HAL_StatusTypeDef hs = HAL_UART_Transmit(&handle, (uint8_t *)&c, 1, HAL_MAX_DELAY);
    if (hs != HAL_OK)
      return -1;
    ITM_SendChar('.');
    return 1;
}

int uart_getc(int port, bool wait)
{
    char c;
    HAL_StatusTypeDef hs =  HAL_UART_Receive_IT(&handle, &icc, 1);
    if (hs != HAL_OK)
      return -1;
    ITM_SendChar('g');
    if (cbuf_read_char(&uart1_rx_buf, (char*) &c, wait) == 0)
        return -1;
    ITM_SendChar('h');
    return c;
}

void uart_flush_tx(int port) {}

void uart_flush_rx(int port) {}

void uart_init_port(int port, uint baud)
{
    // TODO - later
    PANIC_UNIMPLEMENTED;
}
